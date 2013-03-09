/*
 * dftcalc.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg, extended by Dennis Guck and Axel Belinfante
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
#include "imca.h"

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
		messageFormatter->message("  -R              Reuse existing output files.");
		messageFormatter->message("  --mrmc          Use MRMC. (standard setting)");
		messageFormatter->message("  --imca          Use IMCA instead of MRMC.");
		messageFormatter->message("");
		messageFormatter->notify ("Debug Options:");
		messageFormatter->message("  --verbose=x     Set verbosity to x, -1 <= x <= 5.");
		messageFormatter->message("  -v, --verbose   Increase verbosity. Up to 5 levels.");
		messageFormatter->message("  -q              Decrease verbosity.");
		messageFormatter->message("");
		messageFormatter->notify ("Output Options:");
		messageFormatter->message("  -r FILE         Output result in YAML format to this file. (see --help=output)");
		messageFormatter->message("  -c FILE         Output result as CSV format to this file. (see --help=output)");
		messageFormatter->message("  -p              Print result to stdout.");
		messageFormatter->message("  -e evidence     Comma separated list of BE names that fail at startup.");
		messageFormatter->message("  -m              Calculate mean time to failure using IMCA.");
		messageFormatter->message("                  Overrules -i, -t, -f, --mrmc.");
		messageFormatter->message("  -i l u s        Calculate P(DFT fails in [0,x] time units) for each x in interval,");
		messageFormatter->message("                  where interval is given by [l .. u] with step s ");
		messageFormatter->message("  -t xList        Calculate P(DFT fails in [0,x] time units) for each x in xList,");
		messageFormatter->message("                  where xList is a whitespace-separated list of values, default is \"1\"");
		messageFormatter->message("  -I l u          Calculate P(DFT fails in [l,u] time units) where l can be >= 0");
		messageFormatter->message("  -f <command>    Raw Calculation formula for MRMC or IMCA. Overrules -t.");
		messageFormatter->message("                  See --mrmc and --imca");
		messageFormatter->message("  -E errorbound   Error bound, to be passed to IMCA.");
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
		messageFormatter->message("  The Calculation command can be manually set using -f.");
		messageFormatter->message("  For MRMC the default is:");
		messageFormatter->message("    P{>1} [ tt U[0,n] reach ]");
		messageFormatter->message("  and for IMCA the default is:");
		messageFormatter->message("    -max -tb -T n");
		messageFormatter->message("  where n is the mission time (specified via -t or -i), default is 1.");
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

std::string DFT::DFTCalc::getImcaRoot(MessageFormatter* messageFormatter) {
	
	if(!imcaRoot.empty()) {
		return imcaRoot;
	}
	
	char* root = getenv((const char*)"IMCA");
	std::string imcaRoot = root?string(root):"";
	if(imcaRoot=="") {
		if(messageFormatter) messageFormatter->reportError("Environment variable `IMCA' not set. Please set it to where IMCA can be found.");
		goto end;
	}
	
	// \ to /
	{
		char buf[imcaRoot.length()+1];
		for(int i=imcaRoot.length();i--;) {
			if(imcaRoot[i]=='\\')
				buf[i] = '/';
			else
				buf[i] = imcaRoot[i];
		}
		buf[imcaRoot.length()] = '\0';
		if(buf[imcaRoot.length()-1]=='/') {
			buf[imcaRoot.length()-1] = '\0';
		}
		imcaRoot = string(buf);
	}
end:
	return imcaRoot;
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

int isReal(string s, double *res) {
	double tmp_real;
	char * tmp_string;
	int r  = sscanf(s.c_str(),"%lf%s",&tmp_real, &tmp_string);
	if (r == 1) {
		if (res != 0)
			*res = tmp_real;
		return 1;
	}
	return 0;
}

std::string doubleToString(double d) {
	std::stringstream ss;
	ss << d;
	return ss.str();
}

double normalize(double d) {
	std::string s = doubleToString(d);
	return atof(s.c_str());
}

void DFT::DFTCalc::printOutput(const File& file, int status) {
	std::string* outContents = FileSystem::load(file);
	if(outContents) {
		messageFormatter->notifyHighlighted("** OUTPUT of " + file.getFileName() + " **");
		if (status!=0)
			messageFormatter->message(*outContents, MessageFormatter::MessageType::Error);
		else
			messageFormatter->message(*outContents);
		messageFormatter->notifyHighlighted("** END output of " + file.getFileName() + " **");
		delete outContents;
	}
}

int DFT::DFTCalc::calculateDFT(const bool reuse, const std::string& cwd, const File& dftOriginal, const std::vector<std::pair<std::string,std::string>>& calcCommands, unordered_map<string,string> settings, bool calcImca) {
	File dft    = dftOriginal.newWithPathTo(cwd);
	File svl    = dft.newWithExtension("svl");
	File svlLog = dft.newWithExtension("log");
	File exp    = dft.newWithExtension("exp");
	File bcg    = dft.newWithExtension("bcg");
	File imc    = dft.newWithExtension("imc");
	File ctmdpi = dft.newWithExtension("ctmdpi");
	File ma     = dft.newWithExtension("ma");
	File lab    = ctmdpi.newWithExtension("lab");
	File dot    = dft.newWithExtension("dot");
	File png    = dot.newWithExtension("png");
	File input  = dft.newWithExtension("input");
	File inputImca  = dft.newWithExtension("inputImca");

	Shell::RunStatistics stats;

	FileSystem::mkdir(File(cwd));

	if(!reuse) {
		//messageFormatter->notify("Deleting generated files");
		if(FileSystem::exists(dft))    FileSystem::remove(dft);
		if(FileSystem::exists(svl))    FileSystem::remove(svl);
		if(FileSystem::exists(svlLog)) FileSystem::remove(svlLog);
		if(FileSystem::exists(exp))    FileSystem::remove(exp);
		if(FileSystem::exists(bcg))    FileSystem::remove(bcg);
		if(FileSystem::exists(imc))    FileSystem::remove(imc);
		if(FileSystem::exists(ctmdpi)) FileSystem::remove(ctmdpi);
		if(FileSystem::exists(ma))     FileSystem::remove(ma);
		if(FileSystem::exists(lab))    FileSystem::remove(lab);
		if(FileSystem::exists(dot))    FileSystem::remove(dot);
		if(FileSystem::exists(png))    FileSystem::remove(png);
		if(FileSystem::exists(input))  FileSystem::remove(input);
	} else {
		messageFormatter->reportAction("Preserving generated files",VERBOSITY_FLOW);
	}

	if(!reuse || !FileSystem::exists(dft)) {
		//printf("Copying %s to %s\n",dftOriginal.getFileRealPath().c_str(),dft.getFileRealPath().c_str());
		FileSystem::copy(dftOriginal,dft);
	} else {
		messageFormatter->reportAction("Reusing copy of original dft file",VERBOSITY_FLOW);
	}

	int com = 0;
	int result = 0;
	Shell::SystemOptions sysOps;
	sysOps.verbosity = VERBOSITY_EXECUTIONS;
	sysOps.cwd = cwd;
	
	messageFormatter->notify("Calculating `"+dft.getFileBase()+"'");

	if(!reuse || !FileSystem::exists(exp) || !FileSystem::exists(svl)) {
		// dft -> exp, svl
		messageFormatter->reportAction("Translating DFT to EXP...",VERBOSITY_FLOW);
		sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".dft2lntc.report";
		sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".dft2lntc.err";
		sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".dft2lntc.out";
			std::stringstream ss;
		ss << dft2lntcExec.getFilePath()
		   << " --verbose=" << messageFormatter->getVerbosity()
		   << " -s \"" + svl.getFileRealPath() + "\""
		   << " -x \"" + exp.getFileRealPath() + "\""
		   << " -b \"" + bcg.getFileRealPath() + "\""
		   << " -n \"" + dftOriginal.getFileRealPath() + "\""
		   << " \""    + dft.getFileRealPath() + "\""
		   << " --warn-code";
		ss << " -e \"";
		for(std::string e: evidence) {
			ss << e << ",";
		}
		ss << "\"";
		if (!messageFormatter->usingColoredMessages())
			ss << " --no-color";
		sysOps.command = ss.str();
		result = Shell::system(sysOps);

		if(result || !FileSystem::exists(exp) || !FileSystem::exists(svl)) {
			printOutput(File(sysOps.outFile), result);
			printOutput(File(sysOps.errFile), result);
			return 1;
		} else if (messageFormatter->getVerbosity() >= 5) {
			printOutput(File(sysOps.outFile), result);
			printOutput(File(sysOps.errFile), result);
		}
	} else {
		messageFormatter->reportAction("Reusing DFT to EXP translation result",VERBOSITY_FLOW);
	}

	if (!reuse || !FileSystem::exists(bcg)) {
		// svl, exp -> bcg
		messageFormatter->reportAction("Building IMC...",VERBOSITY_FLOW);
		sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".svl.report";
		sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".svl.err";
		sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".svl.out";
		sysOps.command    = svlExec.getFilePath()
			                + " \""    + svl.getFileRealPath() + "\"";
		result = Shell::system(sysOps);

		if(!FileSystem::exists(bcg)) {
			printOutput(File(sysOps.outFile), result);
			printOutput(File(sysOps.errFile), result);
			return 1;
		}
	} else {
		messageFormatter->reportAction("Reusing IMC",VERBOSITY_FLOW);
	}
	
	// obtain memtime result from svl
	if(Shell::readMemtimeStatisticsFromLog(svlLog,stats)) {
		messageFormatter->reportWarning("Could not read from svl log file `" + svlLog.getFileRealPath() + "'");
	}
	
	if(!calcImca) {
		if(!reuse || !FileSystem::exists(ctmdpi)) {
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
				printOutput(File(sysOps.outFile), result);
				printOutput(File(sysOps.errFile), result);
				return 1;
			}
		} else {
			messageFormatter->reportAction("Reusing IMC to CTMDPI translation result",VERBOSITY_FLOW);
		}

		std::vector<DFT::DFTCalculationResultItem> resultItems;
		for(auto mrmcCalcCommand: calcCommands) {

			// -> mrmcinput
			MRMC::FileHandler* fileHandler = new MRMC::FileHandler(mrmcCalcCommand.first);
			fileHandler->generateInputFile(input);
			if(!FileSystem::exists(input)) {
				messageFormatter->reportError("Error generating MRMC input file `" + input.getFileRealPath() + "'");
				return 1;
			}

			// ctmdpi, lab, mrmcinput -> calculation
			messageFormatter->reportAction("Calculating probability with MRMC...",VERBOSITY_FLOW);
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
				printOutput(File(sysOps.outFile), result);
				printOutput(File(sysOps.errFile), result);
				return 1;
			} else if (messageFormatter->getVerbosity() >= 5) {
				printOutput(File(sysOps.outFile), result);
				printOutput(File(sysOps.errFile), result);
			}

			if(fileHandler->readOutputFile(File(sysOps.outFile))) {
				messageFormatter->reportError("Could not calculate");
				return 1;
			} else {
				double res = fileHandler->getResult();
				DFT::DFTCalculationResultItem calcResultItem;
				calcResultItem.missionTime = mrmcCalcCommand.second;
				calcResultItem.mrmcCommand = mrmcCalcCommand.first;
				calcResultItem.failprob = res;
				resultItems.push_back(calcResultItem);
			}
	
			delete fileHandler;
		}
		DFT::DFTCalculationResult calcResult;
		calcResult.dftFile = dft.getFilePath();
		calcResult.failprobs = resultItems;
		calcResult.stats = stats;
		results.insert(pair<string,DFT::DFTCalculationResult>(dft.getFileName(), calcResult));
	} else {
		if(!reuse || !FileSystem::exists(ma)) {
			// bcg -> ma
			messageFormatter->reportAction("Translating IMC to IMCA format...",VERBOSITY_FLOW);
			sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".bcg2imca.report";
			sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".bcg2imca.err";
			sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".bcg2imca.out";
			sysOps.command    = bcg2imcaExec.getFilePath()
						+ " " + bcg.getFileRealPath()
						+ " " + ma.getFileRealPath()
						+ " FAIL"
						;
			result = Shell::system(sysOps);

			if(!FileSystem::exists(ma)) {
				printOutput(File(sysOps.outFile), result);
				printOutput(File(sysOps.errFile), result);
				return 1;
			}
		} else {
			messageFormatter->reportAction("Reusing IMC to IMCA format translation result",VERBOSITY_FLOW);
		}
		
		std::vector<DFT::DFTCalculationResultItem> resultItems;
		for(auto imcaCalcCommand: calcCommands) {

			// -> imcainput
			IMCA::FileHandler* fileHandler = new IMCA::FileHandler(imcaCalcCommand.first);
		
			// imca -> calculation
			messageFormatter->reportAction("Calculating probability with IMCA...",VERBOSITY_FLOW);
			sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".imca.report";
			sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".imca.err";
			sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".imca.out";
			sysOps.command    = imcaExec.getFilePath()
						+ " "    + ma.getFileRealPath()
						+ " "    + imcaCalcCommand.first
						;
			result = Shell::system(sysOps);

			if(result) {
				printOutput(File(sysOps.outFile), result);
				printOutput(File(sysOps.errFile), result);
				return 1;
			} else if (messageFormatter->getVerbosity() >= 5) {
				printOutput(File(sysOps.outFile), result);
				printOutput(File(sysOps.errFile), result);
			}

			if(fileHandler->readOutputFile(File(sysOps.outFile))) {
				messageFormatter->reportError("Could not calculate");
				return 1;
			} else {
				std::vector<std::pair<std::string,IMCA::T_Chance>> imcaResult = fileHandler->getResults();
				for(auto imcaResultItem: imcaResult) {
					DFT::DFTCalculationResultItem calcResultItem;
					//if (imcaResult.size() == 1 && imcaResultItem.first == "?")
					if (imcaResultItem.first == "" || imcaResultItem.first == "?")
						calcResultItem.missionTime = imcaCalcCommand.second;
					else
						calcResultItem.missionTime = imcaResultItem.first;
					calcResultItem.mrmcCommand = imcaCalcCommand.first;
					calcResultItem.failprob = imcaResultItem.second;
					resultItems.push_back(calcResultItem);
				}
			}
	
			delete fileHandler;
		}
		DFT::DFTCalculationResult calcResult;
		calcResult.dftFile = dft.getFilePath();
		calcResult.failprobs = resultItems;
		calcResult.stats = stats;
		results.insert(pair<string,DFT::DFTCalculationResult>(dft.getFileName(), calcResult));
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
			printOutput(File(sysOps.outFile), result);
			printOutput(File(sysOps.errFile), result);
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
			printOutput(File(sysOps.outFile), result);
			printOutput(File(sysOps.errFile), result);
			return 1;
		}
		
	}
	
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
	string timeIntervalLwb    = "";
	string timeIntervalUpb    = "";
	string timeIntervalStep   = "";
	int    timeIntervalSet    = 0;
	string timeLwb            = "";
	string timeUpb            = "";
	int    timeLwbUpbSet      = 0;
	string yamlFileName       = "";
	int    yamlFileSet        = 0;
	string csvFileName        = "";
	int    csvFileSet         = 0;
	string dotToType          = "png";
	int    dotToTypeSet       = 0;
	string outputFolder       = "output";
	int    outputFolderSet    = 0;
	string calcCommand        = "";
	int    calcCommandSet     = 0;
	int    mttf               = 0;
	string errorBound         = "";
	int    errorBoundSet      = 0;

	int verbosity            = 0;
	int print                = 0;
	int reuse                = 0;
	int useColoredMessages   = 1;
	int printHelp            = 0;
	string printHelpTopic    = "";
	int printVersion         = 0;
	bool calcImca = false;
	
	std::vector<std::string> failedBEs;
	
	/* Parse command line arguments */
	char c;
	while( (c = getopt(argc,argv,"C:e:E:f:mpqr:Rc:t:i:I:hv-:")) >= 0 ) {
		switch(c) {
			
			// -C FILE
			case 'C':
				outputFolder = string(optarg);
				outputFolderSet = 1;
				break;
			
			// -r FILE
			case 'r':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					yamlFileName = "";
					yamlFileSet = 1;
				} else {
					yamlFileName = string(optarg);
					yamlFileSet = 1;
				}
				break;
			
			// -c FILE
			case 'c':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					csvFileName = "";
					csvFileSet = 1;
				} else {
					csvFileName = string(optarg);
					csvFileSet = 1;
				}
				break;
			
			// -p
			case 'p':
				print = 1;
				break;
			
			// -R
			case 'R':
				reuse = 1;
				break;
			
			// -E Error bound
			case 'E':
				errorBound = string(optarg);
				errorBoundSet = true;
				break;
			
			// -f MRMC/IMCA Command
			case 'f':
				calcCommand = string(optarg);
				calcCommandSet = true;
				//calcImca = false;
				break;
			
			// -m
			case 'm':
				mttf = 1;
				break;
			
			// -t STRING containing time values separated by whitespace
			case 't':
				timeSpec = string(optarg);
				timeSpecSet = 1;
				break;
			
			// -i STRING STRING STRING
			case 'i':
				timeIntervalLwb = string(optarg);
				timeIntervalUpb = string(argv[optind]);
				optind++;
				timeIntervalStep = string(argv[optind]);
				optind++;
				timeIntervalSet = 1;
				break;
			
			// -I STRING STRING
			case 'I':
				timeLwb = string(optarg);
				timeUpb = string(argv[optind]);
				optind++;
				timeLwbUpbSet = 1;
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
				} else if(!strncmp("dot",optarg,3)) {
					dotToTypeSet = true;
					if(strlen(optarg)>4 && optarg[3]=='=') {
						dotToType = string(optarg+4);
						cerr << "DOT: " << dotToType << endl;
					}
				} else if(!strncmp("verbose",optarg,7)) {
					if(strlen(optarg)>8 && optarg[7]=='=') {
						verbosity = atoi(optarg+8);
					} else if(strlen(optarg)==7) {
						++verbosity;
					}
				} else if(!strcmp("no-color",optarg)) {
					useColoredMessages = false;
				}
				if(!strcmp("mrmc",optarg)) {
					calcImca=false;
				}else if(!strcmp("imca",optarg)) {
					calcImca=true;
				}
		}
	}

	/* Create a new compiler context */
	MessageFormatter* messageFormatter = new MessageFormatter(std::cerr);
	messageFormatter->useColoredMessages(useColoredMessages);
	messageFormatter->setVerbosity(verbosity);
	messageFormatter->setAutoFlush(true);

	string imcaEb("");
	if (errorBoundSet) {
		double t;
		if (!isReal(errorBound, &t) || t<=0) {
			messageFormatter->reportErrorAt(Location("commandline -E flag"),"Given error bound is not a positive real: "+errorBound);
		}
		imcaEb = " -e " + errorBound;
	}

	if (mttf && (calcCommandSet || timeIntervalSet || timeSpecSet ||timeLwbUpbSet)) {
		messageFormatter->reportWarningAt(Location("commandline"),"MTTF flag (-m) has been given: ignoring time specifications and calculation commands");
	}
	if (mttf) {
		calcImca = true;
		calcCommandSet = true;
		calcCommand = "-et -max" + imcaEb;
		timeIntervalSet = false;
		timeSpecSet = false;
	}
	if (timeLwbUpbSet) {
		double tl;
		if(!isReal(timeLwb, &tl) || tl<0) {
			messageFormatter->reportErrorAt(Location("commandline -I flag"),"Given interval lwb is not a non-negative real: "+timeLwb);
		}
		double tu;
		if(!isReal(timeUpb, &tu) || tu<=0) {
			messageFormatter->reportErrorAt(Location("commandline -I flag"),"Given interval upb is not a positive real: "+timeUpb);
		}
		calcImca = true;
		calcCommandSet = true;
		calcCommand = "-max -tb -F " +timeLwb + " -T " + timeUpb + imcaEb;
		timeIntervalSet = false;
		timeSpecSet = false;
	}

	if (!calcCommandSet && !timeIntervalSet)
		timeSpecSet = 1; // default

	/* Print help / version if requested and quit */
	if(printHelp) {
		print_help(messageFormatter,printHelpTopic);
		exit(0);
	}
	if(printVersion) {
		print_version(messageFormatter);
		exit(0);
	}
	
	std::vector<std::pair<std::string,std::string>> mrmcCommands;
	std::vector<std::pair<std::string,std::string>> imcaCommands;
	if(calcCommandSet) {
		mrmcCommands.push_back(pair<string,string>(calcCommand,"?"));
		imcaCommands.push_back(pair<string,string>(calcCommand,"?"));
	} else if (timeSpecSet) {
		std::string str = timeSpec;
		size_t b, e;
		bool hasValidItems = false;
		bool hasInvalidItems = false;
		std::string sep(" \t,;"); // separator characters that we allow
		while((b=str.find_first_not_of(sep)) != string::npos) {
			//cerr << "str: \"" + str + "\"" << endl;
			//cerr << "b: " << b << endl;
			// find first separator character
			e=str.substr(b).find_first_of(sep);
			//cerr << "e: " << e << endl;
			std::string s;
			s = str.substr(b,e);
			if (e!=string::npos) {
				str = str.substr(b+e+1);
			} else {
				str = "";
			}
			//cerr<< "s: \"" << s << "\"" << endl;
			double t;
			if(!isReal(s, &t) || t<0) {
				hasInvalidItems = true;
				messageFormatter->reportErrorAt(Location("commandline -t flag"),"Given mission time value is not a non-negative real: "+s);
			} else {
				hasValidItems = true;
				mrmcCommands.push_back(pair<string,string>("P{>1} [ tt U[0," + s + "] reach ]", s));
				imcaCommands.push_back(pair<string,string>("-max -tb -T " + s + imcaEb, s));
			}
		}
		if (!hasInvalidItems && !hasValidItems) {
			messageFormatter->reportErrorAt(Location("commandline -t flag"),"Given mission time list of values is empty");
		}
	} else if (timeIntervalSet) {
		bool hasItems = false;
		bool intervalErrorReported = false;
		double lwb;
		if(!isReal(timeIntervalLwb, &lwb) || lwb<0) {
			messageFormatter->reportErrorAt(Location("commandline -i flag"),"Given interval lwb is not a non-negative real: "+timeIntervalLwb);
			intervalErrorReported = true;
		}
		double upb;
		if(!isReal(timeIntervalUpb, &upb) || upb<=0) {
			messageFormatter->reportErrorAt(Location("commandline -i flag"),"Given interval upb is not a positive real: "+timeIntervalUpb);
			intervalErrorReported = true;
		}
		double step;
		if(!isReal(timeIntervalStep, &step) || step<=0) {
			messageFormatter->reportErrorAt(Location("commandline -i flag"),"Given interval step is not a positive real: "+timeIntervalStep);
			intervalErrorReported = true;
		}
		/* Check if all went OK so far */
		if(messageFormatter->getErrors()>0) {
			return -1;
		}
		for(double n=lwb; normalize(n) <= normalize(upb); n+= step) {
			hasItems = true;
			std::string s = doubleToString(n);
			mrmcCommands.push_back(pair<string,string>("P{>1} [ tt U[0," + s + "] reach ]", s));
		}
		std::string s_from = doubleToString(lwb);
		std::string s_to = doubleToString(upb);
		std::string s_step = doubleToString(step);
		imcaCommands.push_back(pair<string,string>("-max -tb -b " + s_from + " -T " + s_to + " -i "+s_step + imcaEb, "?"));
		if (!hasItems && !intervalErrorReported) {
			messageFormatter->reportErrorAt(Location("commandline -i flag"),"Given interval is empty (lwb > upb)");
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
	bool hasErrors = false;
	for(File dft: dfts) {
		hasInput = true;
		if(FileSystem::exists(dft)) {
			bool res = calc.calculateDFT(reuse, outputFolderFile.getFileRealPath(),dft,(calcImca?imcaCommands:mrmcCommands),settings,calcImca);
			hasErrors = hasErrors || res;
		} else {
			messageFormatter->reportError("DFT File `" + dft.getFileRealPath() + "' does not exist");
		}
	}
	workdir.popd();

	if (hasErrors) {
		return(1);
	}
	
	/* If there were no DFT calculations performed... */
	if(!hasInput) {
		messageFormatter->reportWarning("No calculations performed");
	}
	
	/* Show results */
	if(verbosity>0 || print) {
		if(calcCommandSet) {
			messageFormatter->notify("Using: " + calcCommand);
		} else if (timeIntervalSet) {
			messageFormatter->notify("Within time interval: [" + timeIntervalLwb + " .. " + timeIntervalUpb +"], stepsize " + timeIntervalStep);
		} else {
			messageFormatter->notify("Within time units: " + timeSpec);
		}
		std::stringstream out;
		for(auto it: calc.getResults()) {
			std::string fName = it.first;
			for(auto it2: it.second.failprobs) {
				if (mttf) {
					out << "MTTF(`" << fName << "'" << ")=" << it2.failprob << std::endl;;
				} else {
					out << "P(`" << fName << "'" << ", " << it2.mrmcCommand << ", " << it2.missionTime << ", " << "fails)=" << it2.failprob << std::endl;;
				}
			}
		}
		std::cout << out.str();
	}
	
	/* Write yaml file */
	if(yamlFileSet) {
		YAML::Emitter out;
		out << calc.getResults();
		if(yamlFileName=="") {
			std::cout << string(out.c_str()) << std::endl;
		} else {
			std::ofstream yamlFile(yamlFileName);
			if(yamlFile.is_open()) {
				messageFormatter->notify("Printing yaml to file: " + yamlFileName);
				yamlFile << string(out.c_str()) << std::endl;
				yamlFile.flush();
			} else {
				messageFormatter->reportErrorAt(Location(yamlFileName),"could not open file for printing yaml");
			}
		}
	}
	
	/* Write csv file */
	if(csvFileSet) {
		std::stringstream out;
		if (mttf) {
			out << "Mean Time to Failure" << std::endl;
			for(auto it: calc.getResults()) {
				std::string fName = it.first;
				for(auto it2: it.second.failprobs) {
					out << it2.failprob << std::endl;
				}
			}
		} else {
			out << "Time" << ", " << "Unreliability" << std::endl;
			for(auto it: calc.getResults()) {
				std::string fName = it.first;
				for(auto it2: it.second.failprobs) {
					out << it2.missionTime << ", " << it2.failprob << std::endl;
				}
			}
		}
		if(csvFileName=="") {
			std::cout << out.str();
		} else {
			std::ofstream csvFile(csvFileName);
			if(csvFile.is_open()) {
				messageFormatter->notify("Printing csv to file: " + csvFileName);
				csvFile << out.str();
				csvFile.flush();
			} else {
				messageFormatter->reportErrorAt(Location(csvFileName),"could not open file for printing csv");
			}
		}
	}
	
	/* Free the messager */
	delete messageFormatter;
	
	return 0;
}

