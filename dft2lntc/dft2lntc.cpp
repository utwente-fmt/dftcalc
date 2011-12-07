#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <fstream>
#include <libgen.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef WIN32
#include <io.h>
#endif

#ifdef WIN32
	int dir_make(const char* path, int mode) {
		(void)mode;
		return mkdir(path);
	}
#else
	int dir_make(const char* path, __mode_t mode) {
		return mkdir(path,mode);
	}
#endif

#include "dft_parser.h"
#include "dft_ast.h"
#include "ASTPrinter.h"
#include "ASTValidator.h"
#include "ASTDFTBuilder.h"
#include "realpath.h"
#include "dft2lnt.h"
#include "DFTree.h"
#include "DFTreeValidator.h"
#include "DFTreePrinter.h"
#include "DFTreeBCGNodeBuilder.h"
#include "DFTreeSVLAndLNTBuilder.h"
#include "DFTreeEXPBuilder.h"
#include "compiletime.h"

FILE* pp_outputFile = stdout;

void print_help() {
	printf("dft2lnt [INPUTFILE.dft] [options]\n");
	printf(
"Compiles the inputfile to EXP and SVL script. If no inputfile was specified, stdin is used.\n"
"If no outputfile was specified, 'a.svl' and 'a.exp' are used.\n"
"\n"
"Options:\n"
"  -a FILE     Output AST to file. '-' for stdout.\n"
"  -t FILE     Output DFT to file. '-' for stdout.\n"
"  -h, --help  Show this help.\n"
"  -o FILE     Output EXP to <FILE>.exp and SVL to <FILE>.svl.\n"
"  -s FILE     Output EXP to file. '-' for stdout. Overrules -o.\n"
"  -x FILE     Output SVL to file. '-' for stdout. Overrules -o.\n"
"  -b FILE     Output of SVL to this BCG file.\n"
"  --color     Use colored messages.\n"
	);
}

void print_version() {
	printf("dft2lntc");
	printf("  built on %s\n",COMPILETIME_DATE);
	printf("  git revision `%s'",COMPILETIME_GITREV);
	if(COMPILETIME_GITCHANGED)
		printf(" + uncommited changes\n");
	else
		printf("\n");
	printf("Copyright statement.\n");
}

std::string getRoot(CompilerContext* compilerContext) {
	char* root = getenv((const char*)"DFT2LNTROOT");
	std::string dft2lntRoot = root?string(root):"";

	if(dft2lntRoot=="") {
		compilerContext->reportError("Environment variable `DFT2LNTROOT' not set. Please set it to where lntnodes/ can be found.");
		goto end;
	}
	
	// \ to /
	{
		char buf[dft2lntRoot.length()+1];
		for(int i=dft2lntRoot.length();i--;) {
			if(dft2lntRoot[i]=='\\')
				buf[i] = '/';
			else
				buf[i] = dft2lntRoot[i];
		}
		buf[dft2lntRoot.length()] = '\0';
		if(buf[dft2lntRoot.length()-1]=='/') {
			buf[dft2lntRoot.length()-1] = '\0';
		}
		dft2lntRoot = string(buf);
	}
	
	struct stat rootStat;
	if(stat((dft2lntRoot).c_str(),&rootStat)) {
		// report error
		compilerContext->reportError("Could not stat DFT2LNTROOT (`" + dft2lntRoot + "')");
		dft2lntRoot = "";
		goto end;
	}
	
	if(stat((dft2lntRoot+DFT::DFTreeBCGNodeBuilder::LNTROOT).c_str(),&rootStat)) {
		if(dir_make((dft2lntRoot+DFT::DFTreeBCGNodeBuilder::LNTROOT).c_str(),0755)) {
			compilerContext->reportError("Could not create LNT Nodes directory (`" + dft2lntRoot+DFT::DFTreeBCGNodeBuilder::LNTROOT + "')");
			dft2lntRoot = "";
			goto end;
		}
	}

	if(stat((dft2lntRoot+DFT::DFTreeBCGNodeBuilder::BCGROOT).c_str(),&rootStat)) {
		if(dir_make((dft2lntRoot+DFT::DFTreeBCGNodeBuilder::BCGROOT).c_str(),0755)) {
			compilerContext->reportError("Could not create BCG Nodes directory (`" + dft2lntRoot+DFT::DFTreeBCGNodeBuilder::BCGROOT + "')");
			dft2lntRoot = "";
			goto end;
		}
	}
	
	compilerContext->message("DFT2LNTROOT is: " + dft2lntRoot);
end:
	return dft2lntRoot;
}

int main(int argc, char** argv) {

	/* Command line arguments and their default settings */
	string inputFileName     = "";
	int    inputFileSet      = 0;
	string outputFileName    = "a";
	int    outputFileSet     = 0;
	string outputASTFileName = "";
	int    outputASTFileSet  = 0;
	string outputDFTFileName = "";
	int    outputDFTFileSet  = 0;
	string outputSVLFileName = "";
	int    outputSVLFileSet  = 0;
	string outputEXPFileName = "";
	int    outputEXPFileSet  = 0;
	string outputBCGFileName = "";
	int    outputBCGFileSet  = 0;

	int optimize             = 1;
	int execute              = 0;
	int stopAfterPreproc     = 0;
	int useColoredMessages   = 0;

	/* Parse command line arguments */
	char c;
	while( (c = getopt(argc,argv,"o:a:t:s:x:h-:")) >= 0 ) {
		switch(c) {

			// -o FILE
			case 'o':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					outputFileName = "";
					outputFileSet = 1;
				} else {
					outputFileName = string(optarg);
					outputFileSet = 1;
				}
				break;

			// -a FILE
			case 'a':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					outputASTFileName = "";
					outputASTFileSet = 1;
				} else {
					outputASTFileName = string(optarg);
					outputASTFileSet = 1;
				}
				break;

			// -t FILE
			case 't':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					outputDFTFileName = "";
					outputDFTFileSet = 1;
				} else {
					outputDFTFileName = string(optarg);
					outputDFTFileSet = 1;
				}
				break;

			// -s FILE
			case 's':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					outputSVLFileName = "";
					outputSVLFileSet = 1;
				} else {
					outputSVLFileName = string(optarg);
					outputSVLFileSet = 1;
				}
				break;

			// -x FILE
			case 'x':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					outputEXPFileName = "";
					outputEXPFileSet = 1;
				} else {
					outputEXPFileName = string(optarg);
					outputEXPFileSet = 1;
				}
				break;

			// -b FILE
			case 'b':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					outputBCGFileName = "";
					outputBCGFileSet = 1;
				} else {
					outputBCGFileName = string(optarg);
					outputBCGFileSet = 1;
				}
				break;

			// -h
			case 'h':
				print_help();
				exit(0);

			// --
			case '-':
				if(!strcmp("help",optarg)) {
					print_help();
					exit(0);
				} else if(!strcmp("version",optarg)) {
					print_version();
					exit(0);
				} else if(!strcmp("color",optarg)) {
					useColoredMessages = true;
				} else if(!strcmp("no-color",optarg)) {
					useColoredMessages = false;
				}
		}
	}

//	printf("args:\n");
//	for(unsigned int i=0; i<(unsigned int)argc; i++) {
//		printf("  %s\n",argv[i]);
//	}

	/* Create a new compiler context */
	CompilerContext* compilerContext = new CompilerContext(std::cerr);
	compilerContext->useColoredMessages(useColoredMessages);

	/* Parse command line arguments without a -X.
	 * These specify the input files. Currently it will only allow
	 * one input file.
	 */
	if(optind<argc) {
		int isSet = 0;
		for(unsigned int i=optind; i<(unsigned int)argc; ++i) {
			if(isSet) {
				compilerContext->reportError("too many input files: "+string(argv[i]));
				continue;
			}
			isSet = 1;
			if(strlen(argv[i])==1 && argv[i][0]=='-') {
				inputFileName = "";
			} else {
				inputFileName = string(argv[i]);
				inputFileSet = 1;
				FILE* f = fopen(argv[i],"r");
				if(f) {
					fclose(f);
				} else {
					compilerContext->reportError("unable to open inputfile " + string(argv[i]));
					return 1;
				}
			}
		}
	}
	
	/*
	 * Handle arguments
	 */
	{
		// Set SVL and EXP output file names
		outputSVLFileName = outputSVLFileSet ? outputSVLFileName : outputFileName + "." + DFT::FileExtensions::SVL;
		outputEXPFileName = outputEXPFileSet ? outputEXPFileName : outputFileName + "." + DFT::FileExtensions::EXP;
		outputBCGFileName = outputBCGFileSet ? outputBCGFileName : outputFileName + "." + DFT::FileExtensions::BCG;
		outputFileSet = outputFileSet || outputSVLFileSet || outputEXPFileSet;
		
		// Test all the files that need to be written if they are writable
		bool ok = true;
		ok = outputSVLFileName=="" ? ok : compilerContext->testWritable(outputSVLFileName) ? ok : false;
		ok = outputEXPFileName=="" ? ok : compilerContext->testWritable(outputEXPFileName) ? ok : false;
		ok = outputDFTFileName=="" ? ok : compilerContext->testWritable(outputDFTFileName) ? ok : false;
		ok = outputASTFileName=="" ? ok : compilerContext->testWritable(outputASTFileName) ? ok : false;
		if(!ok) {
		}
	}

	#if 0

		/* If an outputFile was set on command line, use that for the precompiler
		 * output. If there was no outputFile set, use a temporary file.
		 */
		string pp_outputFileName;

		bool useTmpFile = outputFileName.empty();

		if(useTmpFile) {
			pp_outputFileName = "";
			pp_outputFile = tmpfile();
		} else {
			pp_outputFileName = outputFileName;
			pp_outputFile = fopen(outputFileName.c_str(),"wb+");
		}

		/* Check if the output file could be opened */
		if(!pp_outputFile) {
			printf("DID NOT WRITE\n");
			exit(-1);
		}

		/* Here a preprocessor would go... */

		// Close the outputFile if it was set on the commandline.
		// If the file is temporary, reset it to start
		// This means the temporary file is kept open for later use!
		if(useTmpFile) fseek(pp_outputFile,0,SEEK_SET);
		else fclose(pp_outputFile);

		if(stopAfterPreproc) return 0;
	//	return 0;

		// Set the correct filename for the inputFile
		inputFileName = inputFileSet ? pp_outputFileName : "stdin";
		inputFileSet = 1;
		FILE* inputFile = useTmpFile   ? pp_outputFile :
		                  inputFileSet ? fopen(pp_outputFileName.c_str(),"rb") :
		                  stdin ;
	#else
		FILE* inputFile = inputFileSet ? fopen(inputFileName.c_str(),"rb") : stdin ;
	#endif

/*	for(int i=0;i<100;++i) {
		char c;
		fread(&c,1,1,inputFile);
		if(c==0 || c==EOF) {
			break;
		}
		putchar(c);
	}
*/
//	if(inputFileName.empty()) {
//		printf("No inputfile specified. See dft2lntc --help\n");
//		return 0;
//	}
//
	
	char* real_path = new char[PATH_MAX];
	cwd_realpath(inputFileName.c_str(),real_path);
	std::string parserInputFilePath(real_path);
	delete[] real_path;

	std::string parserInputFileName(path_basename(inputFileName.c_str()));
	
	std::string dft2lntRoot = getRoot(compilerContext);
	bool rootValid = dft2lntRoot!="";

	/* Parse input file */
	compilerContext->notify("Checking syntax...");
	Parser* parser = new Parser(inputFile,parserInputFilePath,compilerContext);
	std::vector<DFT::AST::ASTNode*>* ast = parser->parse();
	compilerContext->flush();
	if(!ast) {
		compilerContext->reportError("could not build AST");
		compilerContext->flush();
		return 1;
	}

	/* Print AST */
	if(ast && outputASTFileSet) {
		compilerContext->notify("Printing AST...");
		compilerContext->flush();
		DFT::ASTPrinter printer(ast,compilerContext);
		if(outputASTFileName!="") {
			std::ofstream astFile (outputASTFileName);
			astFile << printer.print();
		} else {
			std::cerr << printer.print();
		}
	}
	compilerContext->flush();
	
	/* Validate input */
	compilerContext->notify("Validating input...");
	compilerContext->flush();
	int astValid = false;
	{
		DFT::ASTValidator validator(ast,compilerContext);
		astValid = validator.validate();
	}
	compilerContext->flush();
	if(!astValid) {
		printf(":error:AST invalid\n");
		return 1;
	}
	
	/* Create DFT */
	DFT::DFTree* dft = NULL;
	if(astValid) {
		compilerContext->notify("Building DFT...");
		compilerContext->flush();
		DFT::ASTDFTBuilder builder(ast,compilerContext);
		dft = builder.build();
	}
	compilerContext->flush();
	if(!dft) {
		printf(":error:DFT invalid\n");
		return 1;
	}

	/* Validate DFT */
	int dftValid = false;
	if(dft) {
		compilerContext->notify("Validating DFT...");
		compilerContext->flush();
		DFT::DFTreeValidator validator(dft,compilerContext);
		dftValid = validator.validate();
		if(!dftValid) {
			compilerContext->message("DFT determined invalid",CompilerContext::MessageType::Error);
//		} else {
//			printf(":: DFT determined valid\n");
		}
	}
	compilerContext->flush();
	
	/* Printing DFT */
	if(dft && outputDFTFileSet) {
		compilerContext->notify("Printing DFT...");
		compilerContext->flush();
		DFT::DFTreePrinter printer(dft,compilerContext);
		if(outputFileName!="") {
			std::ofstream dftFile (outputDFTFileName);
			printer.print(dftFile);
		} else {
			printer.print(std::cout);
		}
		
	}
	compilerContext->flush();

	/* Building SVL and LNT out of DFT */
//	if(dftValid) {
//		compilerContext->notify("Building SVL and LNT...");
//		DFT::DFTreeSVLAndLNTBuilder builder(dft2lntRoot,".","try",dft,compilerContext);
//		builder.build();
//	}

	/* Building needed BCG files for DFT */
	if(rootValid && dftValid) {
		DFT::DFTreeBCGNodeBuilder builder(dft2lntRoot,dft,compilerContext);
		builder.generate();
	}

	/* Building EXP out of DFT */
	if(rootValid && dftValid && outputFileSet) {
		compilerContext->notify("Building EXP...");
		compilerContext->flush();
		DFT::DFTreeEXPBuilder builder(dft2lntRoot,".",outputBCGFileName,outputEXPFileName,dft,compilerContext);
		builder.build();
		
		if(outputSVLFileName!="") {
			std::ofstream svlFile (outputSVLFileName);
			builder.printSVL(svlFile);
		} else {
			builder.printSVL(std::cout);
		}
		if(outputEXPFileName!="") {
			std::ofstream expFile (outputEXPFileName);
			builder.printEXP(expFile);
		} else {
			builder.printEXP(std::cout);
		}
		
	}
	compilerContext->flush();

	if(compilerContext->getErrors() > 0 || compilerContext->getWarnings() > 0) {
		compilerContext->reportErrors();
	}
	compilerContext->flush();

	delete dft;
	
}
