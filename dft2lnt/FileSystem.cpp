/*
 * FileSystem.cpp
 * 
 * Part of a general library.
 * 
 * Adapted by Gerjan Stokkink to support Mac OS X.
 * Adapted by Enno Ruijters to support Win32
 *
 * @author Freark van der Berg
 */

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef WIN32
#	define WIN32_LEAN_AND_MEAN // Omit rarely-used and architecture-specific stuff from WIN32
#	include <windows.h>
#	include <PathCch.h>
#	include <io.h>
#else
#	include <unistd.h>
#	include <libgen.h>
#endif
#include <string>
#include <assert.h>
#include <iostream>
#include <fstream>
#include "FileSystem.h"

#ifdef WIN32
	int dir_make(const char* path, int mode) {
		(void)mode;
		return !CreateDirectoryA(path, NULL);
	}
#else
	int dir_make(const char* path, mode_t mode) {
		return mkdir(path,mode);
	}
#endif

char* path_basename(const char* path) {
	const char *base = path;

	while (*path) {
		if (*path++ == '/') {
			base = path;
		}
	}
	return (char *) base;
}

#ifndef WIN32
	std::string FileSystem::getRealPath(const std::string& filePath) {
		char path[PATH_MAX];
		realpath(filePath.c_str(),path);
		return std::string(path);
	}

	std::string FileSystem::getDirName(const std::string& filePath) {
		char* fp = strdup(filePath.c_str());
		if (!fp) return "";
		char* result = dirname(fp);
		std::string p(result);
		free(fp);
		return p;
	}

	std::string FileSystem::getBaseName(const std::string& filePath) {
		char* fp = strdup(filePath.c_str());
		if (!fp) return "";
		char* result = basename(fp);
		std::string p(result);
		free(fp);
		return p;
	}
#else
	std::string FileSystem::getRealPath(const std::string& filePath) {
		char path[MAX_PATH];
		GetFullPathNameA(filePath.c_str(), sizeof(path), path, NULL);
		return std::string(path);
	}

	std::string FileSystem::getDirName(const std::string& filePath) {
		wchar_t path[MAX_PATH];
		wchar_t fullPath[MAX_PATH];
		char utf8Path[MAX_PATH];
		if (!MultiByteToWideChar(CP_ACP, 0, filePath.c_str(), -1, path, sizeof(path)))
			throw std::runtime_error("Error converting path to wide string: " + filePath);
		GetFullPathNameW(path, sizeof(fullPath) / sizeof(*fullPath), fullPath, NULL);
		if (PathCchRemoveFileSpec(fullPath, sizeof(fullPath) / sizeof(*fullPath)) != S_OK)
			throw std::runtime_error("Error getting directory of: " + filePath);
		WideCharToMultiByte(CP_ACP, 0, fullPath, -1, utf8Path, sizeof(utf8Path), NULL, NULL);
		return std::string(utf8Path);
	}

	std::string FileSystem::getBaseName(const std::string& filePath) {
		char path[MAX_PATH];
		char *file;
		GetFullPathNameA(filePath.c_str(), sizeof(path), path, &file);
		return std::string(file);
	}
#endif



std::string FileSystem::getFileExtension(const std::string& filePath) {
	std::string base = FileSystem::getBaseName(filePath);
	const char *result = base.c_str();
	const char *extension = NULL;
	while (*result) {
		if (*result++ == '.') {
			extension = result;
		}
	}
	std::string p("");
	if(extension) {
		p = std::string(extension);
	}
	return p;
}
std::string FileSystem::getFileBase(const std::string& filePath) {
	std::string base = FileSystem::getBaseName(filePath);
	const char* result = base.c_str();
	const char* extension = NULL;
	while (*result) {
		if (*result++ == '.') {
			extension = result;
		}
	}
	std::string p("");
	if(extension)
		p = base.substr(0, extension - result);
	return p;
}
void FileSystem::getFileBaseAndExtension(std::string& fileBase, std::string& fileExtension, const std::string& filePath) {
	char* fp = strdup(filePath.c_str());
	if(!fp) return;
	char* result = fp;
	char* fileName = fp;
	while (*result) {
#ifdef WIN32
		if (*result == '\\')
			fileName = result + 1;
#endif
		if (*result++ == '/') {
			fileName = result;
		}

	}
	char* extension = NULL;
	result = fileName;
	while (*result) {
		if (*result++ == '.') {
			extension = result;
		}
	}
	if(extension) {
		*(extension-1) = '\0';
		fileBase = std::string(fileName);
		fileExtension = std::string(extension);
	} else {
		fileBase = std::string(fileName);
		fileExtension = "";
	}
	free(fp);
}

int FileSystem::exists(const File& file) {
	struct stat fileStat;
	return !stat(file.getFileRealPath().c_str(),&fileStat);
}

int FileSystem::move(const File& from, const File& to, bool interactive, bool safe) {
	if(!exists(from)) return 1;
	if(exists(to)) {
		if(interactive) {
			assert(0 && "implement interactive move");
		} else if(safe) {
			return 1;
		}
	}
	return rename(from.getFileRealPath().c_str(),to.getFileRealPath().c_str());
}

int FileSystem::copy(const File& from, const File& to, bool interactive, bool safe) {
	if(!exists(from)) return 1;
	if(exists(to)) {
		if(interactive) {
			assert(0 && "implement interactive copy");
		} else if(safe) {
			return 1;
		}
	}
	FILE *fromFile, *toFile;
	char ch;
	
	/* open source file */
	if((fromFile = fopen(from.getFileRealPath().c_str(), "rb"))==NULL) {
		return 1;
	}
	
	/* open destination file */
	if((toFile = fopen(to.getFileRealPath().c_str(), "wb"))==NULL) {
		return 1;
	}
	
	/* copy the file */
	while(!feof(fromFile)) {
		ch = fgetc(fromFile);
		if(ferror(fromFile)) {
			return 1;
		}
		if(!feof(fromFile)) fputc(ch, toFile);
		if(ferror(toFile)) {
			return 1;
		}
	}
	
	if(fclose(fromFile)==EOF) {
		return 1;
	}
	
	if(fclose(toFile)==EOF) {
		return 1;
	}
	
	return 0;
}

int FileSystem::mkdir(const File& dir, int mode) {
	//if(exists(dir)) return 0;
	return dir_make(dir.getFileRealPath().c_str(),mode);
}

int FileSystem::chdir(const File& dir) {
#ifndef WIN32
	return ::chdir(dir.getFileRealPath().c_str());
#else
	return SetCurrentDirectory(dir.getFileRealPath().c_str());
#endif
}

int FileSystem::isDir(const File& file) {
#ifndef WIN32
	struct stat fileStat;
	stat(file.getFileRealPath().c_str(),&fileStat);
	return S_ISDIR(fileStat.st_mode);
#else
	DWORD attributes = GetFileAttributesA(file.getFileRealPath().c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES)
		return false;
	return !!(attributes & FILE_ATTRIBUTE_DIRECTORY);
#endif
}

int FileSystem::remove(const File& file) {
	return ::remove(file.getFileRealPath().c_str());
}

time_t FileSystem::getLastAccessTime(const File& file) {
	struct stat fileStat;
	stat(file.getFileRealPath().c_str(),&fileStat);
	return fileStat.st_atime;
}

time_t FileSystem::getLastModificationTime(const File& file) {
	struct stat fileStat;
	stat(file.getFileRealPath().c_str(),&fileStat);
	return fileStat.st_mtime;
}

time_t FileSystem::getLastStatusChangeTime(const File& file) {
	struct stat fileStat;
	stat(file.getFileRealPath().c_str(),&fileStat);
	return fileStat.st_ctime;
}

bool FileSystem::canCreateOrModify(const File& file) {
	if(hasAccessTo(file,W_OK)) return true;
	if(!exists(file) && hasAccessTo(File(file.getPathTo()),W_OK|X_OK)) return true;
	return false;
}

bool FileSystem::hasAccessTo(const File& file, int mode) {
	return !access(file.getFileRealPath().c_str(),mode);
}

bool File::isWritableDirectory(void) {
#ifndef WIN32
	return !access(getFileRealPath().c_str(), W_OK);
#else
	/* Windows has no convenient way to test this, other than to try to create a file. */
	if (!FileSystem::isDir(*this))
		return false;
	wchar_t path[MAX_PATH];
	wchar_t tempPath[MAX_PATH];
	if (!MultiByteToWideChar(CP_ACP, 0, filePath.c_str(), -1, path, sizeof(path)))
		throw std::runtime_error("Error converting path to wide string: " + filePath);
	if (!GetTempFileNameW(path, L"d2l", 0, tempPath))
		return false;
	std::cerr << "Good\n";
	DeleteFileW(tempPath);
	return true;
#endif
}

int FileSystem::findInPath(std::vector<File>& result, const File& file) {
#if 0
	char foundFile[PATH_MAX];
	int r = (int)SearchPath(NULL,file.getFileName().c_str(),NULL,PATH_MAX,foundFile,NULL);
	if(r<PATH_MAX) {
		result.push_back(std::string(foundFile));
		return 1;
	} else if(r==0) {
		return 0;
	} else {
		return -1;
	}
#else
	const char* path = getenv("PATH");
	if(!path) return 0;
	return findInPath(result,file,path);
#endif
}

int FileSystem::findInPath(std::vector<File>& result, const File& file, const char* path) {
	int n = 0;
	const char* pathEntryStart = path;
	const char* pathEntryEnd = path;
	//std::cerr << "Search path: " << std::string(path) << std::endl;
	for(;;) {
		switch(*pathEntryEnd) {
			case '\0':
			case ':': {
				std::string pathEntry = std::string(pathEntryStart,pathEntryEnd);
				//std::cerr << "Trying path: " << pathEntry << std::endl;
				File hit = file.newWithPathTo(pathEntry);
				if(FileSystem::exists(hit)) {
					result.push_back(hit);
					++n;
				}
				//std::cerr << "Trying: " << hit.getFilePath() << std::endl;
				if(*pathEntryEnd=='\0') {
					goto pathend;
				} else {
					++pathEntryEnd;
					pathEntryStart = pathEntryEnd;
				}
				break;
			}
			default:
				++pathEntryEnd;
		}
	}
pathend:
	return n;
}

std::string* FileSystem::load(const File& file) {
//	FILE* fp;
//	long len;
//
//	std::string* str = new std::string();
//
//	fp = fopen(file.getFileRealPath().c_str(),"rb");
//	fseek(fp,0,SEEK_END);
//	len = ftell(fp)+1;
//	fseek(fp,0,SEEK_SET);
//
//	str->reserve(len+1);
//	fread((char*)str->c_str(),len,1,fp); //read into buffer
//	fclose(fp);
	std::string* str = new std::string();
	size_t filesize;
	
	std::ifstream fileStream (file.getFileRealPath());
	if(fileStream.is_open()) {
		filesize=fileStream.tellg();
		
		str->reserve(filesize);
		
		fileStream.seekg(0);
		while (!fileStream.eof()) {
			int c = fileStream.get();
			*str += c;
		}
		str->resize((str->size()-1));
		return str;
	} else {
		delete str;
		return NULL;
	}
}

// ----------------------------------------

PushD::PushD() {
}
PushD::PushD(const std::string& path) {
	pushd(path);
}
PushD::PushD(const File& path) {
	pushd(path.getFileRealPath());
}

PushD::~PushD() {
	popd();
}

int PushD::pushd(const std::string& dir) {
#ifndef WIN32
	char buffer[PATH_MAX];
	char* res = getcwd(buffer,PATH_MAX);
	if (!res)
		return 1;
#else
	char buffer[MAX_PATH];
	if (!GetCurrentDirectoryA(sizeof(buffer), buffer))
		return 1;
#endif
	dirStack.push_back( std::string(buffer) );
	FileSystem::chdir(dir);
	return 0;
}

int PushD::pushd(const File& dir) {
	return pushd(dir.getFileRealPath());
}

int PushD::popd() {
	if(dirStack.size()>0) {
		std::string dir = dirStack.back();
		FileSystem::chdir(dir);
		dirStack.pop_back();
		return 0;
	} else {
		return 1;
	}
}

// ----------------------------------------

File::File(const std::string& filePath) {
	this->pathTo = FileSystem::getDirName(filePath);
	FileSystem::getFileBaseAndExtension(fileBase,fileExtension,filePath);
	updateExtra();
}
File::File(const std::string& pathTo, const std::string& fileName) {
	this->pathTo = pathTo;
	FileSystem::getFileBaseAndExtension(fileBase,fileExtension,fileName);
	updateExtra();
}
File::File(const std::string& pathTo, const std::string& fileBase, const std::string& fileExtension) {
	this->pathTo = pathTo;
	this->fileBase = fileBase;
	this->fileExtension = fileExtension;
	updateExtra();
}

File& File::setPathTo(const std::string& pathTo) {
	this->pathTo = pathTo;
	updateExtra();
	return *this;
}

File& File::insertPathToPrefix(const std::string& pathTo) {
	if(!this->isAbsolute()) {
		this->pathTo = pathTo + "/" + this->pathTo;
		updateExtra();
	}
	return *this;
}

File& File::setFileExtension(const std::string& fileExtension ) {
	this->fileExtension = fileExtension;
	updateExtra();
	return *this;
}

File File::newWithExtension(const std::string& fileExtension) const {
	assert(this);
	File f(*this);
	f.fileExtension = fileExtension;
	f.updateExtra();
	return f;
}

File File::newWithName(const std::string& fileBase, const std::string& fileExtension) const {
	assert(this);
	File f(*this);
	f.fileBase = fileBase;
	f.fileExtension = fileExtension;
	f.updateExtra();
	return f;
}

File File::newWithName(const std::string& fileName) const {
	assert(this);
	File f(*this);
	FileSystem::getFileBaseAndExtension(f.fileBase,f.fileExtension,fileName);
	f.updateExtra();
	return f;
}

File File::newWithPathTo(const std::string& pathTo) const {
	assert(this);
	File f(*this);
	f.pathTo = pathTo;
	f.updateExtra();
	return f;
}

File& File::fix() {
	pathTo = FileSystem::getRealPath(pathTo); 
	updateExtra();
	return *this;
}

File& File::fixWithOrigin(const std::string& pathTo) {
	File absolute = this->newFixed();
	if(this->getFilePath()!=absolute.getFilePath()) {
		this->pathTo = pathTo + "/" + this->pathTo;
		fix();
	}
	return *this;
}

File File::newFixed() const {
	File file = *this;
	return file.fix();
}

File File::newFixedWithOrigin(const std::string& pathTo) const {
	File file = *this;
	return file.fixWithOrigin(pathTo);
}

std::ostream& operator<<(std::ostream& stream, const File& file) {
	stream << file.getFilePath();
	return stream;
}
