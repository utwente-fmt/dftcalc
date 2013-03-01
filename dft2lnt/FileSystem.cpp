/*
 * FileSystem.cpp
 * 
 * Part of a general library.
 * 
 * Adapted by Gerjan Stokkink to support Mac OS X.
 *
 * @author Freark van der Berg
 */

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <assert.h>
#include <iostream>
#include <fstream>
#include "FileSystem.h"

#ifdef WIN32
	int dir_make(const char* path, int mode) {
		(void)mode;
		return mkdir(path);
	}
#else
	int dir_make(const char* path, mode_t mode) {
		return mkdir(path,mode);
	}
#endif

#ifdef WIN32

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * realpath() Win32 implementation, supports non standard glibc extension
 * This function has no copyright assigned and is placed in the Public Domain.
 * Written by Nach M. S. September 8, 2005
 */
char *realpath(const char *path, char resolved_path[PATH_MAX]) {
	char *return_path = 0;

	if (path) { //Else EINVAL
		if (resolved_path) {
			return_path = resolved_path;
		} else {
			//Non standard extension that glibc uses
			return_path = (char*)malloc(PATH_MAX);
		}

		if (return_path) { //Else EINVAL
			//This is a Win32 API function similar to what realpath() is supposed to do
			size_t size = GetFullPathNameA(path, PATH_MAX, return_path, 0);

			//GetFullPathNameA() returns a size larger than buffer if buffer is too small
			if (size > PATH_MAX) {
				if (return_path != resolved_path) { //Malloc'd buffer - Unstandard extension retry
					size_t new_size;

					free(return_path);
					return_path = (char*)malloc(size);

					if (return_path) {
						new_size = GetFullPathNameA(path, size, return_path, 0); //Try again

						if (new_size > size) { //If it's still too large, we have a problem, don't try again
							free(return_path);
							return_path = 0;
							errno = ENAMETOOLONG;
						} else {
							size = new_size;
						}
					} else {
						//I wasn't sure what to return here, but the standard does say to return EINVAL
						//if resolved_path is null, and in this case we couldn't malloc large enough buffer
						errno = EINVAL;
					}
				} else { //resolved_path buffer isn't big enough
					return_path = 0;
					errno = ENAMETOOLONG;
				}
			}

			//GetFullPathNameA() returns 0 if some path resolve problem occured
			if (!size) {
				if (return_path != resolved_path) { //Malloc'd buffer
					free(return_path);
				}

				return_path = 0;

				//Convert MS errors into standard errors
				switch (GetLastError()) {
				case ERROR_FILE_NOT_FOUND:
					errno = ENOENT;
					break;

				case ERROR_PATH_NOT_FOUND:
				case ERROR_INVALID_DRIVE:
					errno = ENOTDIR;
					break;

				case ERROR_ACCESS_DENIED:
					errno = EACCES;
					break;

				default: //Unknown Error
					errno = EIO;
					break;
				}
			}

			//If we get to here with a valid return_path, we're still doing good
			if (return_path) {
				struct stat stat_buffer;

				//Make sure path exists, stat() returns 0 on success
				if (stat(return_path, &stat_buffer)) {
					if (return_path != resolved_path) {
						free(return_path);
					}

					return_path = 0;
					//stat() will set the correct errno for us
				}
				//else we succeeded!
			}
		} else {
			errno = EINVAL;
		}
	} else {
		errno = EINVAL;
	}

	return return_path;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WIN32

#ifdef __cplusplus
extern "C" {
#endif

char* cwd_realpath(const char* path, char resolved_path[PATH_MAX]) {
	char* relative_path = (char*)malloc(PATH_MAX);
	getcwd(relative_path,PATH_MAX);
	//std::cerr << "cwd_realpath: path='" << path << "' relative_path='" << relative_path << "'" << std::endl;
	int len = strlen(relative_path);
	relative_path[len++] = '/';
	strncpy(relative_path+len,path,PATH_MAX-len);

	realpath(relative_path,resolved_path);
	//std::cerr << "cwd_realpath: path='" << path << "' ext-relative_path='" << relative_path << "' resolved_path='" << resolved_path << "'" << std::endl;

	free(relative_path);
	return resolved_path;
}

char* path_basename(const char* path) {
	const char *base = path;

	while (*path) {
		if (*path++ == '/') {
			base = path;
		}
	}
	return (char *) base;
}

#ifdef __cplusplus
} // extern "C"
#endif

std::string FileSystem::getRealPath(const std::string& filePath) {
	char path[PATH_MAX];
	realpath(filePath.c_str(),path);
	return std::string(path);
}

std::string FileSystem::getDirName(const std::string& filePath) {
	char* fp = strdup(filePath.c_str());
	if(!fp) return "";
	char* result = dirname(fp);
	std::string p(result);
	free(fp);
	return p;
}

std::string FileSystem::getBaseName(const std::string& filePath) {
	char* fp = strdup(filePath.c_str());
	if(!fp) return "";
	char* result = basename(fp);
	std::string p(result);
	free(fp);
	return p;
}

std::string FileSystem::getFileExtension(const std::string& filePath) {
	char* fp = strdup(filePath.c_str());
	if(!fp) return "";
	char* result = basename(fp);
	char* extension = NULL;
	while (*result) {
		if (*result++ == '.') {
			extension = result;
		}
	}
	std::string p("");
	if(extension) {
		p = std::string(extension);
	}
	free(fp);
	return p;
}
std::string FileSystem::getFileBase(const std::string& filePath) {
	char* fp = strdup(filePath.c_str());
	if(!fp) return "";
	char* result = basename(fp);
	char* extension = NULL;
	while (*result) {
		if (*result++ == '.') {
			extension = result;
		}
	}
	std::string p("");
	if(extension) {
		*extension = '\0';
		p = std::string(result);
	}
	free(fp);
	return p;
}
void FileSystem::getFileBaseAndExtension(std::string& fileBase, std::string& fileExtension, const std::string& filePath) {
	char* fp = strdup(filePath.c_str());
	if(!fp) return;
	char* result = fp;
	char* fileName = fp;
	while (*result) {
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
	return ::chdir(dir.getFileRealPath().c_str());
}

int FileSystem::isDir(const File& file) {
	struct stat fileStat;
	stat(file.getFileRealPath().c_str(),&fileStat);
	return S_ISDIR(fileStat.st_mode);
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
	char buffer[PATH_MAX];
	char* res = getcwd(buffer,PATH_MAX);
	if(res) {
		dirStack.push_back( std::string(buffer) );
		chdir(dir.c_str());
		return 0;
	} else {
		return 1;
	}
}

int PushD::pushd(const File& dir) {
	return pushd(dir.getFileRealPath());
}

int PushD::popd() {
	if(dirStack.size()>0) {
		chdir(dirStack.back().c_str());
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
