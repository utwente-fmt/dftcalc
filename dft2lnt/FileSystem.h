#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <unistd.h>
#include <limits.h>

class FileSystem {
private:
	vector<string> dirStack;
public:
	int pushd(std::string dir) {
		char buffer[PATH_MAX];
		char* res = getcwd(buffer,PATH_MAX);
		if(res) {
			dirStack.push_back( string(buffer) );
			chdir(dir.c_str());
			return 0;
		} else {
			return 1;
		}
	}
	int popd() {
		if(dirStack.size()>0) {
			chdir(dirStack.back().c_str());
			dirStack.pop_back();
			return 0;
		} else {
			return 1;
		}
	}
};

class PushD {
private:
	vector<string> dirStack;
public:
	PushD() {
	}
	PushD(std::string path) {
		pushd(path);
	}
	
	virtual ~PushD() {
		popd();
	}
	
	int pushd(std::string dir) {
		char buffer[PATH_MAX];
		char* res = getcwd(buffer,PATH_MAX);
		if(res) {
			dirStack.push_back( string(buffer) );
			chdir(dir.c_str());
			return 0;
		} else {
			return 1;
		}
	}
	int popd() {
		if(dirStack.size()>0) {
			chdir(dirStack.back().c_str());
			dirStack.pop_back();
			return 0;
		} else {
			return 1;
		}
	}
};

#endif // FILESYSTEM_H