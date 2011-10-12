#ifndef DFT2LNT_H
#define DFT2LNT_H

// Maximum number of nested files using include statements
#define MAX_FILE_NESTING 100

#include <iostream>
#include <vector>
#include <map>

#include "dft_ast.h"
#include "DFTree.h"

namespace DFT {

class DFT2LNT {
public:
	static int validateCommands(const std::vector<DFT::AST::ASTNode*>* ast) {
		int valid = true;
		for(int i=0; i<ast->size();++i) {
//			valid = ast->at(i)->validate() ? valid : false;
		}
		return valid;
	}
	static DFTree* createDFT(const std::vector<DFT::AST::ASTNode*>* ast) {
		DFTree* dft = new DFTree();
		for(int i=0; i<ast->size();++i) {
//			ast->at(i)->handleCommand(dft);
		}
		return dft;
	}
};

/**
 * Every file is associated with an instance of this struct
 */
typedef struct FileContext {
	FILE* fileHandle;
	std::string filename;
	Location loc;
	
	FileContext():
		fileHandle(NULL),
		filename("") {
	}
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
class CompilerContext {
private:
	unsigned int errors;
	unsigned int warnings;
public:
	int indentLevel;
	std::string name;
	map<string,string> types;
	FileContext fileContext[MAX_FILE_NESTING]; // max file nesting of MAX_FILE_NESTING allowed
	int fileContexts;

		/**
		 * Creates a new compiler context with a specific name.
		 */
		CompilerContext(std::string name): errors(0), warnings(0), name(name) {
		}

	~CompilerContext() {
	}

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
	bool popFileContext() {
		--fileContexts;
		return false;
	}
	int getFileContexts() const {
		return fileContexts;
	}
	FileContext& getFileContext(int i) {
		assert(0<=i && i < MAX_FILE_NESTING && "getFileContext(i): invalid i");
		return fileContext[i];
	}

		void reportErrorAt(Location loc, std::string str);

		void reportWarningAt(Location loc, std::string str);

		void reportError(std::string str);
		void reportWarning(std::string str);

		unsigned int getErrors() {
			return errors;
		}

		unsigned int getWarnings() {
			return warnings;
		}
};

} //Namespace: DFT

#endif // DFT2LNT_H