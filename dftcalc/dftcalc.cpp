/*
 * dftcalc.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <limits.h>
#include <CADP.h>

#ifdef WIN32
#include <io.h>
#endif

using namespace std;

#include "Shell.h"
#include "FileSystem.h"
#include "MessageFormatter.h"
#include "DFTreeBCGNodeBuilder.h"
#include "dftcalc.h"
#include "compiletime.h"
#include "yaml-cpp/yaml.h"

const int DFT::DFTCalc::VERBOSITY_SEARCHING = 2;

const int VERBOSITY_FLOW = 1;
const int VERBOSITY_DATA = 1;
const int VERBOSITY_EXECUTIONS = 2;

void print_help(MessageFormatter* messageFormatter, string topic="") {
	if(topic.empty()) {
		messageFormatter->notify ("dftcalc [INPUTFILE.dft] [options]");
		messageFormatter->message("  Calculates the failure probability for the specified DFT file, given the");
		messageFormatter->message("  specified time constraints. Result is written to the specified output file.");
		messageFormatter->message("  Check dftcalc --help=output for more details regarding the output.");
		messageFormatter->message("");
		messageFormatter->notify ("General Options:");
		messageFormatter->message("  -h, --help      Show this help.");
		messageFormatter->message("  --color         Use colored messages.");
		messageFormatter->message("  --no-color      Do not use colored messages.");
		messageFormatter->message("  --version       Print version info and quit.");
		messageFormatter->message("  -O<s>=<v>       Sets settings <s> to value <v>. (see --help=settings)");
		messageFormatter->message("");
		messageFormatter->notify ("Debug Options:");
		messageFormatter->message("  --verbose=x     Set verbosity to x, -1 <= x <= 5.");
		messageFormatter->message("  -v, --verbose   Increase verbosity. Up to 5 levels.");
		messageFormatter->message("  -q              Decrease verbosity.");
		messageFormatter->message("");
		messageFormatter->notify ("Output Options:");
		messageFormatter->message("  -r FILE         Output result to this file. (see --help=output)");
		messageFormatter->message("  -p              Print result to stdout.");
		messageFormatter->message("  -t x            Calculate P(DFT fails in x time units), default is 1");
		messageFormatter->message("  -m <command>    Raw MRMC Calculation command. Overrules -t.");
		messageFormatter->message("  -C DIR          Temporary output files will be in this directory");
		messageFormatter->flush();
	} else if(topic=="output") {
		messageFormatter->notify ("Output");
		messageFormatter->message("  The output file specified with -r uses YAML syntax.");
		messageFormatter->message("  The top node is a map, containing one element, a mapping containing various");
		messageFormatter->message("  information regarding the DFT. E.g. it looks like this:");
		messageFormatter->message("  b.dft:");
		messageFormatter->message("    dft: b.dft");
		messageFormatter->message("    failprob: 0.3934693");
		messageFormatter->message("    stats:");
		messageFormatter->message("      time_user: 0.54");
		messageFormatter->message("      time_system: 0.21");
		messageFormatter->message("      time_elapsed: 1.8");
		messageFormatter->message("      mem_virtual: 13668");
		messageFormatter->message("      mem_resident: 1752");
		messageFormatter->message("  The MRMC Calculation command can be manually set using -m. The default is:");
		messageFormatter->message("    P{>1} [ tt U[0,x] reach ]");
		messageFormatter->message("  where x is the specified number of time units using -t, default is 1.");
	} else if(topic=="settings") {
		messageFormatter->notify ("Settings");
		messageFormatter->message("  Use the format -Ok=v,k=v,k=v or specify multiple -O ");
		messageFormatter->message("  Some key values:");
		messageFormatter->message("");
	} else if(topic=="topics") {
		messageFormatter->notify ("Help topics:");
		messageFormatter->message("  output          Displays the specification of the output format");
		messageFormatter->message("  To view topics: dftcalc --help=<topic>");
		messageFormatter->message("");
	} else {
		messageFormatter->reportAction("Unknown help topic: " + topic);
	}		
}

void print_help_output(MessageFormatter* messageFormatter) {
}

void print_version(MessageFormatter* messageFormatter) {
	messageFormatter->notify ("dftcalc");
	messageFormatter->message(string("  built on ") + COMPILETIME_DATE);
	{
		FileWriter out;
		out << string("  git version: ") + string(COMPILETIME_GITVERSION) + " (nearest)" << out.applypostfix;
		out << string("  git revision `") + COMPILETIME_GITREV + "'";
		if(COMPILETIME_GITCHANGED)
			out << " + uncommited changes";
		messageFormatter->message(out.toString());
	}
	messageFormatter->message("  ** Copyright statement. **");
	messageFormatter->flush();
}

std::string DFT::DFTCalc::getCoralRoot(MessageFormatter* messageFormatter) {
	
	if(!coralRoot.empty()) {
		return coralRoot;
	}
	
	char* root = getenv((const char*)"CORAL");
	std::string coralRoot = root?string(root):"";
	
	if(coralRoot=="") {
		if(messageFormatter) messageFormatter->reportError("Environment variable `CORAL' not set. Please set it to where coral can be found.");
		goto end;
	}
	
	// \ to /
	{
		char buf[coralRoot.length()+1];
		for(int i=coralRoot.length();i--;) {
			if(coralRoot[i]=='\\')
				buf[i] = '/';
			else
				buf[i] = coralRoot[i];
		}
		buf[coralRoot.length()] = '\0';
		if(buf[coralRoot.length()-1]=='/') {
			buf[coralRoot.length()-1] = '\0';
		}
		coralRoot = string(buf);
	}
end:
	return coralRoot;
}

std::string DFT::DFTCalc::getRoot(MessageFormatter* messageFormatter) {
	
	if(!dft2lntRoot.empty()) {
		return dft2lntRoot;
	}
	
	char* root = getenv((const char*)"DFT2LNTROOT");
	std::string dft2lntRoot = root?string(root):"";
	
	if(dft2lntRoot=="") {
		if(messageFormatter) messageFormatter->reportError("Environment variable `DFT2LNTROOT' not set. Please set it to where lntnodes/ can be found.");
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
		if(messageFormatter) messageFormatter->reportError("Could not stat DFT2LNTROOT (`" + dft2lntRoot + "')");
		dft2lntRoot = "";
		goto end;
	}
	
	if(stat((dft2lntRoot+DFT2LNT::LNTSUBROOT).c_str(),&rootStat)) {
		if(FileSystem::mkdir(dft2lntRoot+DFT2LNT::LNTSUBROOT,0755)) {
			if(messageFormatter) messageFormatter->reportError("Could not create LNT Nodes directory (`" + dft2lntRoot+DFT2LNT::LNTSUBROOT + "')");
			dft2lntRoot = "";
			goto end;
		}
	}

	if(stat((dft2lntRoot+DFT2LNT::BCGSUBROOT).c_str(),&rootStat)) {
		if(FileSystem::mkdir(dft2lntRoot+DFT2LNT::BCGSUBROOT,0755)) {
			if(messageFormatter) messageFormatter->reportError("Could not create BCG Nodes directory (`" + dft2lntRoot+DFT2LNT::BCGSUBROOT + "')");
			dft2lntRoot = "";
			goto end;
		}
	}
	
	if(messageFormatter) messageFormatter->reportAction("DFT2LNTROOT is: " + dft2lntRoot,VERBOSITY_DATA);
end:
	return dft2lntRoot;
}

std::string DFT::DFTCalc::getCADPRoot(MessageFormatter* messageFormatter) {
	
	
	string cadp = CADP::getRoot();
	
	if(cadp=="") {
		if(messageFormatter) messageFormatter->reportError("Environment variable `CORAL' not set. Please set it to where coral can be found.");
	}
	
	return cadp;
}


std::string intToString(int i) {
	std::stringstream ss;
	ss << i;
	return ss.str();
}

void DFT::DFTCalc::printOutput(const File& file) {
	std::string* outContents = FileSystem::load(file);
	if(outContents) {
		messageFormatter->notifyHighlighted("** OUTPUT of " + file.getFileName() + " **");
		messageFormatter->message(*outContents);
		messageFormatter->notifyHighlighted("** END output of " + file.getFileName() + " **");
		delete outContents;
	}
}

int DFT::DFTCalc::calculateDFT(const std::string& cwd, const File& dftOriginal, std::string mrmcCalcCommand, unordered_map<string,string> settings) {
	File dft    = dftOriginal.newWithPathTo(cwd);
	File svl    = dft.newWithExtension("svl");
	File svlLog = dft.newWithExtension("log");
	File exp    = dft.newWithExtension("exp");
	File bcg    = dft.newWithExtension("bcg");
	File imc    = dft.newWithExtension("imc");
	File ctmdpi = dft.newWithExtension("ctmdpi");
	File lab    = ctmdpi.newWithExtension("lab");
	File dot    = dft.newWithExtension("dot");
	File png    = dot.newWithExtension("png");
	File input  = dft.newWithExtension("input");

	Shell::RunStatistics stats;

	FileSystem::mkdir(File(cwd));

	if(FileSystem::exists(dft))    FileSystem::remove(dft);
	if(FileSystem::exists(svl))    FileSystem::remove(svl);
	if(FileSystem::exists(svlLog)) FileSystem::remove(svlLog);
	if(FileSystem::exists(exp))    FileSystem::remove(exp);
	if(FileSystem::exists(bcg))    FileSystem::remove(bcg);
	if(FileSystem::exists(imc))    FileSystem::remove(imc);
	if(FileSystem::exists(ctmdpi)) FileSystem::remove(ctmdpi);
	if(FileSystem::exists(lab))    FileSystem::remove(lab);
	if(FileSystem::exists(dot))    FileSystem::remove(dot);
	if(FileSystem::exists(png))    FileSystem::remove(png);
	if(FileSystem::exists(input))  FileSystem::remove(input);

	//printf("Copying %s to %s\n",dftOriginal.getFileRealPath().c_str(),dft.getFileRealPath().c_str());
	FileSystem::copy(dftOriginal,dft);

	int com = 0;
	int result = 0;
	Shell::SystemOptions sysOps;
	sysOps.verbosity = VERBOSITY_EXECUTIONS;
	sysOps.cwd = cwd;
	
	messageFormatter->notify("Calculating `"+dft.getFileBase()+"'");
	
	// dft -> exp, svl
	messageFormatter->reportAction("Translating DFT to EXP...",VERBOSITY_FLOW);
	sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".dft2lntc.report";
	sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".dft2lntc.err";
	sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".dft2lntc.out";
	std::stringstream ss;
	ss << dft2lntcExec.getFilePath()
	   << " --verbosity=" << messageFormatter->getVerbosity()
	   << " -s \"" + svl.getFileRealPath() + "\""
	   << " -x \"" + exp.getFileRealPath() + "\""
	   << " -b \"" + bcg.getFileRealPath() + "\""
	   << " \""    + dft.getFileRealPath() + "\""
	   << " --warn-code";
	ss << " -e \"";
	for(std::string e: evidence) {
		ss << e << ",";
	}
	ss << "\"";
	sysOps.command = ss.str();
	result = Shell::system(sysOps);

	if(result || !FileSystem::exists(exp) || !FileSystem::exists(svl)) {
		printOutput(File(sysOps.outFile));
		printOutput(File(sysOps.errFile));
		return 1;
	}

	// svl, exp -> bcg
	messageFormatter->reportAction("Building IMC...",VERBOSITY_FLOW);
	sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".svl.report";
	sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".svl.err";
	sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".svl.out";
	sysOps.command    = svlExec.getFilePath()
	                  + " \""    + svl.getFileRealPath() + "\"";
	result = Shell::system(sysOps);
	
	if(!FileSystem::exists(bcg)) {
		printOutput(File(sysOps.outFile));
		printOutput(File(sysOps.errFile));
		return 1;
	}
	
	// obtain memtime result from svl
	if(Shell::readMemtimeStatisticsFromLog(svlLog,stats)) {
		messageFormatter->reportWarning("Could not read from svl log file `" + svlLog.getFileRealPath() + "'");
	}
	
	// bcg -> ctmdpi, lab
	messageFormatter->reportAction("Translating IMC to CTMDPI...",VERBOSITY_FLOW);
	sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".imc2ctmdpi.report";
	sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".imc2ctmdpi.err";
	sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".imc2ctmdpi.out";
	sysOps.command    = imc2ctmdpExec.getFilePath()
	                  + " -a FAIL"
	                  + " -o \"" + ctmdpi.getFileRealPath() + "\""
	                  + " \""    + bcg.getFileRealPath() + "\""
					  ;
	result = Shell::system(sysOps);

	if(!FileSystem::exists(ctmdpi)) {
		printOutput(File(sysOps.outFile));
		printOutput(File(sysOps.errFile));
		return 1;
	}

	// -> mrmcinput
	MRMC::FileHandler* fileHandler = new MRMC::FileHandler(mrmcCalcCommand);
	fileHandler->generateInputFile(input);
	if(!FileSystem::exists(input)) {
		messageFormatter->reportError("Error generating MRMC input file `" + input.getFileRealPath() + "'");
		return 1;
	}

	// ctmdpi, lab, mrmcinput -> calculation
	messageFormatter->reportAction("Calculating probability...",VERBOSITY_FLOW);
	sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".mrmc.report";
	sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".mrmc.err";
	sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".mrmc.out";
	sysOps.command    = mrmcExec.getFilePath()
	                  + " ctmdpi"
	                  + " \""    + ctmdpi.getFileRealPath() + "\""
	                  + " \""    + lab.getFileRealPath() + "\""
	                  + " < \""  + input.getFileRealPath() + "\""
	                  ;
	result = Shell::system(sysOps);

	if(result) {
		printOutput(File(sysOps.outFile));
		printOutput(File(sysOps.errFile));
		return 1;
	}

	if(fileHandler->readOutputFile(File(sysOps.outFile))) {
		messageFormatter->reportError("Could not calculate");
		return 1;
	} else {
		double res = fileHandler->getResult();
		DFT::DFTCalculationResult calcResult;
		calcResult.dftFile = dft.getFilePath();
		calcResult.failprob = res;
		calcResult.stats = stats;
		results.insert(pair<string,DFT::DFTCalculationResult>(dft.getFileName(),calcResult));
	}


	if(!buildDot.empty()) {
		// bcg -> dot
		messageFormatter->reportAction("Building DOT from IMC...",VERBOSITY_FLOW);
		sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".bcg_io.report";
		sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".bcg_io.err";
		sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".bcg_io.out";
		sysOps.command    = bcgioExec.getFilePath()
						  + " \""    + bcg.getFileRealPath() + "\""
						  + " \""    + dot.getFileRealPath() + "\""
						  ;
		result = Shell::system(sysOps);
		
		if(!FileSystem::exists(dot)) {
			printOutput(File(sysOps.outFile));
			printOutput(File(sysOps.errFile));
			return 1;
		}
		
		// dot -> png
		messageFormatter->reportAction("Translating DOT to " + buildDot + "...",VERBOSITY_FLOW);
		sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".dot.report";
		sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".dot.err";
		sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".dot.out";
		sysOps.command    = dotExec.getFilePath()
						  + " -T" + buildDot
						  + " \""    + dot.getFileRealPath() + "\""
						  + " -o \"" + png.getFileRealPath() + "\""
						  ;
		result = Shell::system(sysOps);

		if(!FileSystem::exists(png)) {
			printOutput(File(sysOps.outFile));
			printOutput(File(sysOps.errFile));
			return 1;
		}
		
	}
	
	delete fileHandler;
	
	return 0;
}

int main(int argc, char** argv) {
	
	/* Set defaults */
	unordered_map<string,string> default_settings;
	
	/* Initialize default settings */
	unordered_map<string,string> settings = default_settings;
	
	/* Command line arguments and their default settings */
	string timeSpec           = "1";
	int    timeSpecSet        = 0;
	string resultFileName     = "";
	int    resultFileSet      = 0;
	string dotToType          = "png";
	int    dotToTypeSet       = 0;
	string outputFolder       = "output";
	int    outputFolderSet    = 0;
	string mrmcCalcCommand    = "";
	int    mrmcCalcCommandSet = 0;

	int verbosity            = 0;
	int print                = 0;
	int useColoredMessages   = 1;
	int printHelp            = 0;
	string printHelpTopic    = "";
	int printVersion         = 0;
	
	std::vector<std::string> failedBEs;
	
	/* Parse command line arguments */
	char c;
	while( (c = getopt(argc,argv,"C:e:h:m:pqr:t:v-:")) >= 0 ) {
		switch(c) {
			
			// -C FILE
			case 'C':
				outputFolder = string(optarg);
				outputFolderSet = 1;
				break;
			
			// -r FILE
			case 'r':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					resultFileName = "";
					resultFileSet = 1;
				} else {
					resultFileName = string(optarg);
					resultFileSet = 1;
				}
				break;
			
			// -p
			case 'p':
				print = 1;
				break;
			
			// -m MRMCCommand
			case 'm':
				mrmcCalcCommand = string(optarg);
				mrmcCalcCommandSet = true;
				break;
			
			// -t FILE
			case 't':
				timeSpec = string(optarg);
				timeSpecSet = 1;
				break;
			
			// -h
			case 'h':
				printHelp = true;
				break;
			
			// -v
			case 'v':
				++verbosity;
				break;
			
			// -q
			case 'q':
				--verbosity;
				break;
			
			// -e
			case 'e': {
				const char* begin = optarg;
				const char* end = begin;
				while(*begin) {
					end = begin;
					while(*end && *end!=',') ++end;
					if(begin<end) {
						failedBEs.push_back(std::string(begin,end));
					}
					if(!*end) break;
					begin = end + 1;
				}
				
			}
			
			// --
			case '-':
				if(!strncmp("help",optarg,4)) {
					printHelp = true;
					if(strlen(optarg)>5 && optarg[4]=='=') {
						printHelpTopic = string(optarg+5);
					}
				} else if(!strcmp("version",optarg)) {
					printVersion = true;
				} else if(!strcmp("color",optarg)) {
					useColoredMessages = true;
				} else if(!strcmp("times",optarg)) {
					if(strlen(optarg)>6 && optarg[5]=='=') {
					} else {
						printf("%s: --times needs argument\n\n",argv[0]);
						printHelp = true;
					}
				} else if(!strcmp("dot",optarg)) {
					dotToTypeSet = true;
					if(strlen(optarg)>4 && optarg[3]=='=') {
						dotToType = string(optarg+4);
						cerr << "DOT: " << dotToType << endl;
					}
				} else if(!strcmp("verbose",optarg)) {
					if(strlen(optarg)>8 && optarg[7]=='=') {
						verbosity = atoi(optarg+8);
					} else {
						++verbosity;
					}
				} else if(!strcmp("no-color",optarg)) {
					useColoredMessages = false;
				}
		}
	}
	
	/* Create a new compiler context */
	MessageFormatter* messageFormatter = new MessageFormatter(std::cerr);
	messageFormatter->useColoredMessages(useColoredMessages);
	messageFormatter->setVerbosity(verbosity);
	messageFormatter->setAutoFlush(true);

	/* Print help / version if requested and quit */
	if(printHelp) {
		print_help(messageFormatter,printHelpTopic);
		exit(0);
	}
	if(printVersion) {
		print_version(messageFormatter);
		exit(0);
	}
	
	{
		int t = atoi(timeSpec.c_str());
		if(t<=0) {
			messageFormatter->reportErrorAt(Location("commandline"),"-t requires a positive integer as argument");
		}
	}
	
	/* Parse command line arguments without a -X.
	 * These specify the input files.
	 */
	vector<File> dfts;
	if(optind<argc) {
		int isSet = 0;
		for(unsigned int i=optind; i<(unsigned int)argc; ++i) {
			if(argv[i][0]=='-') {
				if(strlen(argv[i])==1) {
				}
			} else {
				dfts.push_back(File(string(argv[i])).fix());
			}
		}
	}
	
	/* Enable Shell messages */
	Shell::messageFormatter = messageFormatter;
	
	/* Create the DFTCalc class */
	DFT::DFTCalc calc;
	calc.setMessageFormatter(messageFormatter);
	if(dotToTypeSet) calc.setBuildDOT(dotToType);
	
	/* Check if all needed tools are available */
	if(calc.checkNeededTools()) {
		messageFormatter->reportError("There was an error with the environment");
		return -1;
	}
	
	calc.setEvidence(failedBEs);
	
	/* Check if all went OK so far */
	if(messageFormatter->getErrors()>0) {
		return -1;
	}
	
	/* Change the CWD to ./output, creating the folder if not existent */
	File outputFolderFile = File(outputFolder).fix();
	FileSystem::mkdir(outputFolderFile);
	PushD workdir(outputFolderFile);
	
	/* Calculate DFTs */
	bool hasInput = false;
	for(File dft: dfts) {
		hasInput = true;
		if(FileSystem::exists(dft)) {
			string mrmcCommand = mrmcCalcCommandSet ? mrmcCalcCommand : "P{>1} [ tt U[0," + timeSpec + "] reach ]";
			calc.calculateDFT(outputFolderFile.getFileRealPath(),dft,mrmcCommand,settings);
		} else {
			messageFormatter->reportError("DFT File `" + dft.getFileRealPath() + "' does not exist");
		}
	}
	workdir.popd();
	
	/* If there were no DFT calculations performed... */
	if(!hasInput) {
		messageFormatter->reportWarning("No calculations performed");
	}
	
	/* Print result file */
	if(verbosity>=0 || print || (resultFileSet && resultFileName=="")) {
		if(mrmcCalcCommandSet) {
			messageFormatter->notify("Using: " + mrmcCalcCommand);
		} else {
			messageFormatter->notify("Within time units: " + timeSpec);
		}
		for(auto it: calc.getResults()) {
			std::stringstream out;
			out << "P(`" << it.first << "' fails)=" << it.second.failprob;
			messageFormatter->reportAction(out.str());
		}
	}
	
	/* Write result file */
	if(resultFileSet) {
		YAML::Emitter out;
		out << calc.getResults();
		if(resultFileName=="") {
			std::cout << string(out.c_str()) << std::endl;
		} else {
			std::ofstream resultFile(resultFileName);
			if(resultFile.is_open()) {
				messageFormatter->notify("Printing result to file: " + resultFileName);
				resultFile << string(out.c_str()) << std::endl;
				resultFile.flush();
			} else {
				messageFormatter->reportErrorAt(Location(resultFileName),"could not open file for printing result");
			}
		}
	}
	
	/* Free the messager */
	delete messageFormatter;
	
	return 0;
}
