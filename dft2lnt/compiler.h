/*
 * compiler.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#ifndef COMPILER_H
#define COMPILER_H

// Maximum number of nested files using include statements
#define MAX_FILE_NESTING 100

#include <iostream>
#include <vector>
#include <set>
#include <stack>
#include <map>
#include "ConsoleWriter.h"
#include "dft_parser_location.h"
#include "MessageFormatter.h"

/**
 * Every file is associated with an instance of this struct
 */
typedef struct FileContext {
	FILE* fileHandle;
	std::string filename;
	Location loc;
	
	/**
	 * Creates a new FileContext object.
	 */
	FileContext():
		fileHandle(NULL),
		filename("") {
	}

	/**
	 * Creates a new FileContext object with the specified settings.
	 */
	FileContext(FILE* fileHandle, std::string& filename, Location& loc):
		fileHandle(fileHandle),
		filename(filename),
		loc(loc) {
	}
} FileContext;

/**
 * The compiler context contains settings and data related to the compilation
 * of a file and its dependencies.
 */
class CompilerContext: public MessageFormatter {
private:
	std::string name;
public:
	map<string,string> types;
	FileContext fileContext[MAX_FILE_NESTING]; // max file nesting of MAX_FILE_NESTING allowed
	int fileContexts;

	/**
	 * Creates a new compiler context with a specific name.
	 */
	CompilerContext(std::ostream& out): MessageFormatter(out), name("") {
	}

	virtual ~CompilerContext() {
	}

	/**
	 * Push the specified FileContext on the stack.
	 * @param The FileContext to be pushed on the stack.
	 * @return false: success, true: error
	 */
	bool pushFileContext(FileContext context) {
		if(fileContexts>=MAX_FILE_NESTING) {
			reportError("Max file nesting (100) surpassed");
			return true;
		} else {
			fileContext[fileContexts] = context;
			++fileContexts;
		}
		return false;
	}
	
	/**
	 * Pop the top FileContext from the stack.
	 * @return false: success, true: error
	 */
	bool popFileContext() {
		--fileContexts;
		return false;
	}
	
	/**
	 * Returns the current number of file contexts.
	 * @return The current number of file contexts.
	 */
	int getFileContexts() const {
		return fileContexts;
	}
	
	/**
	 * Returns a reference to the FileContext at the specified location
	 * on the stack.
	 * @param i The position in the stack of the FileContext that will be
	 *          returned.
	 * @return The FileContext at the specified location.
	 */
	FileContext& getFileContext(int i) {
		assert(0<=i && i < MAX_FILE_NESTING && "getFileContext(i): invalid i");
		return fileContext[i];
	}
	
	/**
	 * Tests if the specified file is allowed to be written to.
	 * If using a relative path, take the working directory into account.
	 * @param fileName The path+name of the file to test.
	 * @return true: writable, false: not writable.
	 */
	virtual bool testWritable(std::string fileName);

};

#endif // COMPILER_H
