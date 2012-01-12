#include <stdio.h>

#include "compiler.h"
#include "FileSystem.h"





	}






}

bool CompilerContext::testWritable(std::string fileName) {
	File file(fileName);
	File folder(file.getPathTo());
	
	if(FileSystem::exists(File(fileName))) {
		if(FileSystem::hasAccessTo(file,W_OK)) {
			return true;
		} else {
			reportError("no permission to open outputfile '" + fileName + "' for writing");
			return false;
		}
	} else if(FileSystem::hasAccessTo(folder,W_OK)) {
		return true;
	} else {
		reportError("unable to open outputfile '" + fileName + "' for writing");
		return false;
	}
}
