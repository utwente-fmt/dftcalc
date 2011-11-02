#ifndef REALPATH_H
#define REALPATH_H

#include <limits.h>
#include <string>

#ifdef __cplusplus

class Path {
	static std::string realPath(std::string path) {
		return "";
	}
};

extern "C" {
#endif

char *realpath(const char *path, char resolved_path[PATH_MAX]);
char *cwd_realpath(const char *path, char resolved_path[PATH_MAX]);
char* path_basename(const char* path);

#ifdef __cplusplus
} // extern "C"
#endif

#endif