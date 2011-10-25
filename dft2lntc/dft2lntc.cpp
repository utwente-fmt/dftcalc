#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <libgen.h>
#include <getopt.h>

#include "dft_parser.h"
#include "dft_ast.h"
#include "ASTValidator.h"
#include "ASTDFTBuilder.h"
#include "realpath.h"
#include "dft2lnt.h"
#include "DFTree.h"
#include "DFTreeValidator.h"
#include "DFTreePrinter.h"
#include "DFTreeSVLAndLNTBuilder.h"

FILE* pp_outputFile = stdout;

void print_help() {
	printf("dft2lnt [INPUTFILE.dft] [options]\n");
	printf(
"Compiles the inputfile to Lotos NT files and SVL script. If no inputfile was specified, stdin is used.\n"
"\n"
"Options:\n"
	);
}

int main(int argc, char** argv) {

	/* Command line arguments and their default settings */
	string inputFileName     = "";
	int    inputFileSet      = 0;
	string outputFileName    = "a.out";
	int    outputFileSet     = 0;
	string outputASTFileName = "";
	int    outputASTFileSet  = 0;
	string outputASMFileName = "";
	int    outputASMFileSet  = 0;
	string outputRUNFileName = "";
	int    outputRUNFileSet  = 0;

	int         optimize          = 1;
	int         execute           = 0;
	int			stopAfterPreproc  = 0;

	/* Parse command line arguments */
	char c;
	while( (c = getopt(argc,argv,"o:a:s:gEr:h-:")) >= 0 ) {
		switch(c) {

			// -o FILE
			case 'o':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					outputFileName = "";
					outputFileSet = 1;
				} else {
					printf("-o: '%s'\n",optarg);
					outputFileName = string(optarg);
					outputFileSet = 1;
					FILE* f = fopen(optarg,"a");
					if(f) {
						fclose(f);
					} else {
						fprintf(stderr,"unable to open outputfile %s\n",optarg);
						return 1;
					}
				}
				break;

			// -a FILE
			case 'a':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					outputASTFileName = "";
					outputASTFileSet = 1;
					printf("outputASTFileSet");
				} else {
					outputASTFileName = string(optarg);
					outputASTFileSet = 1;
					FILE* f = fopen(optarg,"a");
					if(f) {
						fclose(f);
					} else {
						fprintf(stderr,"unable to open outputASTfile %s\n",optarg);
						return 1;
					}
				}
				break;

			// -s FILE
			case 's':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					outputASMFileName = "";
					outputASMFileSet = 1;
				} else {
					outputASMFileName = string(optarg);
					outputASMFileSet = 1;
					FILE* f = fopen(optarg,"a");
					if(f) {
						fclose(f);
					} else {
						fprintf(stderr,"unable to open outputASMfile %s\n",optarg);
						return 1;
					}
				}
				break;


			// -g
			case 'g':
				optimize = 0;
				break;

			// -E
			case 'E':
				stopAfterPreproc = 1;
				break;

			// -r
			case 'r':
				execute = 1;
				if(optarg) {
					if(strlen(optarg)==1 && optarg[0]=='-') {
						outputRUNFileName = "";
						outputRUNFileSet = 1;
					} else {
						outputRUNFileName = string(optarg);
						outputRUNFileSet = 1;
						FILE* f = fopen(optarg,"a");
						if(f) {
							fclose(f);
						} else {
							fprintf(stderr,"unable to open input file for execution %s\n",optarg);
							return 1;
						}
					}
				}
				break;

			// -h
			case 'h':
				print_help();
				exit(0);

			// --
			case '-':
				if(!strcmp(optarg,"help")) {
					print_help();
					exit(0);
				}
		}
	}

//	printf("args:\n");
//	for(unsigned int i=0; i<(unsigned int)argc; i++) {
//		printf("  %s\n",argv[i]);
//	}

	/* Parse command line arguments without a -X.
	 * These specify the input files. Currently it will only allow
	 * one input file.
	 */
	if(optind<argc) {
		int isSet = 0;
		for(unsigned int i=optind; i<(unsigned int)argc; ++i) {
			if(isSet) {
				fprintf(stderr,"too many input files: %s\n",argv[i]);
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
					fprintf(stderr,"unable to open inputfile %s\n",argv[i]);
					return 1;
				}
			}
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
	if(inputFileName.empty()) {
		printf("No inputfile specified. See codolc --help\n");
		return 0;
	}

	/* Create a new compiler context */
	DFT::CompilerContext* compilerContext = new DFT::CompilerContext(inputFileName);
	
	char* real_path = new char[PATH_MAX];
	cwd_realpath(inputFileName.c_str(),real_path);
	std::string parserInputFilePath(real_path);
	delete[] real_path;

	std::string parserInputFileName(path_basename(inputFileName.c_str()));
	
	/* Parse input file */
	printf(":: Checking syntax...\n"); fflush(stdout);
	Parser* parser = new Parser(inputFile,parserInputFilePath,compilerContext);
	std::vector<DFT::AST::ASTNode*>* ast = parser->parse();
	if(!ast) {
		printf(":error:Could not build AST\n");
		return 1;
	}
	/* Validate input */
	printf(":: Validating input...\n"); fflush(stdout);
	int astValid = false;
	{
		DFT::ASTValidator validator(ast,compilerContext);
		astValid = validator.validate();
	}
	if(!astValid) {
		printf(":error:AST invalid\n");
		return 1;
	}
	
	/* Create DFT */
	DFT::DFTree* dft = NULL;
	if(astValid) {
		printf(":: Building DFT...\n"); fflush(stdout);
		DFT::ASTDFTBuilder builder(ast,compilerContext);
		dft = builder.build();
	}
	if(!dft) {
		printf(":error:DFT invalid\n");
		return 1;
	}

	/* Validate DFT */
	int dftValid = false;
	if(dft) {
		printf(":: Validating DFT...\n"); fflush(stdout);
		DFT::DFTreeValidator validator(dft,compilerContext);
		dftValid = validator.validate();
		if(dftValid) {
			printf(":: DFT determined valid\n");
		} else {
			printf(":: DFT determined invalid\n");
		}
	}
	
	/* Printing DFT */
	if(dft) {
		printf(":: Printing DFT...\n"); fflush(stdout);
		DFT::DFTreePrinter printer(dft,compilerContext);
		printer.print(std::cout);
	}

	/* Building SVL and LNT out of DFT */
	if(dft) {
		printf(":: Building SVL and LNT...\n"); fflush(stdout);
		DFT::DFTreeSVLAndLNTBuilder builder(".","try",dft,compilerContext);
		builder.build();
	}
	delete dft;
	
}