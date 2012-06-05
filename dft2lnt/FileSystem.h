/*
 * CompilerContext.h
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

class File;

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <vector>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

char *realpath(const char *path, char resolved_path[PATH_MAX]);
char *cwd_realpath(const char *path, char resolved_path[PATH_MAX]);
char* path_basename(const char* path);

#ifdef __cplusplus
} // extern "C"
#endif

class FileSystem {
private:
	std::vector<std::string> dirStack;
public:
	static std::string getRealPath(const std::string& filePath);
	static std::string getDirName(const std::string& filePath);
	static std::string getBaseName(const std::string& filePath);
	static std::string getFileExtension(const std::string& filePath);
	static std::string getFileBase(const std::string& filePath);
	static void getFileBaseAndExtension(std::string& fileBase, std::string& fileExtension, const std::string& filePath);
	static int exists(const File& file);
	static int move(const File& from, const File& to, bool interactive=false, bool safe=false);
	static int copy(const File& from, const File& to, bool interactive=false, bool safe=false);
	static int mkdir(const File& dir, int mode=0755);
	static int chdir(const File& dir);
	static int isDir(const File& file);
	static int remove(const File& file);
	static time_t getLastAccessTime(const File& file);
	static time_t getLastModificationTime(const File& file);
	static time_t getLastStatusChangeTime(const File& file);
	
	static bool canCreateOrModify(const File& file);
	static bool hasAccessTo(const File& file, int mode);
	static int findInPath(std::vector<File>& result, const File& file);
	static int findInPath(std::vector<File>& result, const File& file, const char* path);

	static std::string* load(const File& file);
};

class PushD {
private:
	std::vector<std::string> dirStack;
public:
	PushD();
	PushD(const std::string& path);
	PushD(const File& path);
	
	virtual ~PushD();
	
	int pushd(const std::string& dir);
	int pushd(const File& dir);
	
	int popd();
};

class File {
private:
	std::string pathTo;
	std::string fileBase;
	std::string fileExtension;

	std::string fileName;
	std::string filePath;

	void updateExtra() {
		this->fileName = fileExtension == "" ? fileBase : fileBase + "." + fileExtension;
		this->filePath = pathTo        == "" ? fileName : pathTo + "/" + fileName;
	}
public:
	File() {};
	File(const std::string& filePath);
	File(const std::string& pathTo, const std::string& fileName);
	File(const std::string& pathTo, const std::string& fileBase, const std::string& fileExtension);
	
	const std::string& getPathTo() const { return pathTo; }
	const std::string& getFileName() const { return fileName; }
	const std::string& getFileBase() const { return fileBase; }
	const std::string& getFileExtension() const { return fileExtension; }
	const std::string& getFilePath() const { return filePath; }
	std::string getFileRealPath() const { return FileSystem::getRealPath(filePath); }
	
	File& setPathTo(const std::string& pathTo);
	File& insertPathToPrefix(const std::string& pathTo);
	File& setFileExtension(const std::string& fileExtension );
	File& fix();
	File& fixWithOrigin(const std::string& path);
	
	File newFixed() const;
	File newFixedWithOrigin(const std::string& pathTo) const;
	File newWithExtension(const std::string& fileExtension) const;
	File newWithName(const std::string& fileBase, const std::string& fileExtension) const;
	File newWithName(const std::string& fileName) const;
	File newWithPathTo(const std::string& filePath) const;
	
	inline bool operator==(const File& other) const {
		return this->getFileRealPath() == other.getFileRealPath();
	}
	
	inline bool operator!=(const File& other) const {
		return !(*this == other);
	}
	
	inline bool operator<(const File& other) const {
		return this->getFileRealPath() < other.getFileRealPath();
	}
	
	inline bool isModifiedLaterThan(const File& other) const {
		return FileSystem::getLastModificationTime(*this) > FileSystem::getLastModificationTime(other);
	}
	inline bool isAccessedLaterThan(const File& other) const {
		return FileSystem::getLastAccessTime(*this) > FileSystem::getLastAccessTime(other);
	}
	
	inline bool isEmpty() {
		return fileBase.empty() && fileExtension.empty();
	}

	inline bool isAbsolute() {
		return !pathTo.empty() && pathTo[0] == '/';
	}
	
};

std::ostream& operator<<(std::ostream& stream, const File& file);

#endif // FILESYSTEM_H
