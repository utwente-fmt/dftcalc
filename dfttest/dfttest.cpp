/*
 * dfttest.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include <vector>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <limits.h>

#ifdef WIN32
#include <io.h>
#endif

using namespace std;

#include "dfttest.h"
#include "test.h"
#include "Shell.h"
#include "MessageFormatter.h"
#include "DFTreeBCGNodeBuilder.h"
#include "compiletime.h"
#include "dftcalc.h"
#include "CADP.h"

const int VERBOSITY_FLOW = 1;
const int VERBOSITY_DATA = 2;
const int VERBOSITY_FILE_SEARCH = 2;

void DFTTestResult::readYAMLNodeSpecific(const YAML::Node& node) {
	if(const YAML::Node* itemNode = node.FindValue("failprob")) {
		*itemNode >> failprob;
	}
	if(const YAML::Node* itemNode = node.FindValue("bcginfo")) {
		*itemNode >> bcgInfo;
	}
}
void DFTTestResult::writeYAMLNodeSpecific(YAML::Emitter& out) const {
	out << YAML::Key << "failprob" << YAML::Value << failprob;
	if(bcgInfo.willWriteSomething()) out << YAML::Key << "bcginfo" << YAML::Value << bcgInfo;
}

Test::ResultStatus DFTTestResult::getResultStatus(Test::TestSpecification* testGeneric) {
	DFTTest* test = static_cast<DFTTest*>(testGeneric);
	
	double verified = test->getVerifiedValue();
	
	//std::cerr << "Comparing2 " << failprob << ", and " << verified;
	if(failprob>=0 && (verified<0 || verified==failprob)) {
		return Test::OK;
	} else {
		return Test::FAILED;
	}
}

void print_help(MessageFormatter* messageFormatter, string topic) {
	if(topic.empty()) {
		messageFormatter->notify ("dfttest [options] [suite.test]");
		messageFormatter->message("  Calculates the failure probability for the DFT files in the specified test");
		messageFormatter->message("  file. Result is written to stdout and saved in the test file.");
		messageFormatter->message("  Check dfttest --help=input for more details regarding the suite file format.");
		messageFormatter->message("  Check dfttest --help=output for more details regarding the output.");
		messageFormatter->message("");
		messageFormatter->message("  If the specified suite file does not exist, it will be created. If the suite");
		messageFormatter->message("  file is not writable, you will be asked to specify a suite file to save to.");
		messageFormatter->message("  If that suite file already exists, the suites will be merged in such a way");
		messageFormatter->message("  that nothing is overwritten.");
		messageFormatter->message("");
		messageFormatter->notify ("Common usage:");
		messageFormatter->message("  dfttest <suite.test> -t <tree.dft>   Run only <tree.dft>, adds <tree.dft>");
		messageFormatter->message("  dfttest <suite.test> -ct <tree.dft>  Adds <tree.dft>, no test is performed");
		messageFormatter->message("");
		messageFormatter->notify ("General Options:");
		messageFormatter->message("  -h, --help      Show this help.");
		messageFormatter->message("  --help=x        Show help about topic x.");
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
		messageFormatter->notify ("Test Options:");
		messageFormatter->message("  -c              Do not run tests, only show cached results.");
		messageFormatter->message("  -f              Force running all tests, regardless of cached results");
		messageFormatter->message("  -t DFTFILE      Add/Limit testing to this DFT. Multiple allowed.");
		messageFormatter->message("  -L              Output is the content of a LaTeX tabular. Implies -c.");
		messageFormatter->message("  -C              Output is in CSV. Implies -c.");
		messageFormatter->message("");
		print_help(messageFormatter,"topics");
		messageFormatter->flush();
	} else if(topic=="output") {
		messageFormatter->notify ("Output");
		messageFormatter->reportAction("Timing");
		messageFormatter->message("  Time measurements are done using platform specific implementations.");
		messageFormatter->message("  On Linux, CLOCK_MONOTONIC_RAW is used and on Windows the API call");
		messageFormatter->message("  QueryPerformanceCounter is used.");
		messageFormatter->message("  Both implementations aim to assure there is no influence from other");
		messageFormatter->message("  programs such as NTP. The measurement is as accurate as the clock of ");
		messageFormatter->message("  the hardware is.");
		messageFormatter->reportAction("Memory");
		messageFormatter->message("  Memory measurements are done by SVL itself, using the program specified in");
		messageFormatter->message("  CADP_TIME environment variable. ");
		messageFormatter->reportAction("BCG Info");
		messageFormatter->message("  Information of the generated BCG, like states and transitions, is obtained");
		messageFormatter->message("  by calling bcg_info.");
	} else if(topic=="input") {
		messageFormatter->notify ("Suite Input");
		messageFormatter->message("  A test suite file is a file in YAML format. It contains a list of tests, where");
		messageFormatter->message("  each test is a map with settings. Supported keys in this map:");
		messageFormatter->message("    - <key>      : <value>");
		messageFormatter->message("    - general    : a map containing general information about the test:");
		messageFormatter->message("      - fullname : a descriptive name of the test");
		messageFormatter->message("      - longdesc : a longer description of the test");
		messageFormatter->message("      - uuid     : a unique identifier for the test");
		messageFormatter->message("      - format   : a unique identifier for the test");
		messageFormatter->message("    - dft      : relative or absolute path to the DFT file");
		messageFormatter->message("    - timeunits: result will reflect P(\"dft fails within timeunits\")");
		messageFormatter->message("    - verified : a map containing verified results as value and motives as key");
		messageFormatter->message("    - results  : a list of maps containing resultmaps");
		messageFormatter->message("                 a resultmap's key is the time the test was started");
		messageFormatter->message("                 a resultmap's value is again a map with the obtained results as");
		messageFormatter->message("                 value and the origin of the results as key");
		messageFormatter->message("");
		messageFormatter->message("  A complete example:");
		messageFormatter->message("  - general:");
		messageFormatter->message("      fullname: Basic Event");
		messageFormatter->message("      uuid: 35896B7BE62877CD3255CA3E1579E976A2DDD9DDFEFC761A51075EBB97BD71A7");
		messageFormatter->message("      longdesc: \"\"");
		messageFormatter->message("      format: 1");
		messageFormatter->message("    results:");
		messageFormatter->message("      - 2012-02-29 17:36:11:");
		messageFormatter->message("          coral:");
		messageFormatter->message("            stats:");
		messageFormatter->message("              time_monraw: 5.78413");
		messageFormatter->message("            failprob: 0.3934693");
		messageFormatter->message("            bcginfo:");
		messageFormatter->message("              states: 4");
		messageFormatter->message("              transitions: 7");
		messageFormatter->message("          dftcalc:");
		messageFormatter->message("            stats:");
		messageFormatter->message("              time_monraw: 3.15412");
		messageFormatter->message("              mem_virtual: 13668");
		messageFormatter->message("              mem_resident: 1752");
		messageFormatter->message("            failprob: 0.3934693");
		messageFormatter->message("            bcginfo:");
		messageFormatter->message("              states: 4");
		messageFormatter->message("              transitions: 6");
		messageFormatter->message("    verified:");
		messageFormatter->message("      manual:");
		messageFormatter->message("        stats:");
		messageFormatter->message("          {}");
		messageFormatter->message("        failprob: 0.3934693");
		messageFormatter->message("        bcginfo:");
		messageFormatter->message("          states: 0");
		messageFormatter->message("          transitions: 0");
		messageFormatter->message("    timeunits: 1");
		messageFormatter->message("    dft: /opt/dftroot/b.dft");
	} else if(topic=="settings") {
		messageFormatter->notify ("Settings");
		messageFormatter->message("  Use the format -Ok=v,k=v,k=v or specify multiple -O ");
		messageFormatter->message("  Some key values:");
		messageFormatter->message("");
	} else if(topic=="topics") {
		messageFormatter->notify ("Help topics:");
		messageFormatter->message("  input           Displays the input format of a suite file");
		messageFormatter->message("  output          Shows some considerations about the output (timing)");
		messageFormatter->message("  To view topics: dfttest --help=<topic>");
		messageFormatter->message("");
	} else {
		messageFormatter->reportAction("Unknown help topic: " + topic);
		print_help(messageFormatter,"topics");
	}		
}

void print_version(MessageFormatter* messageFormatter) {
	messageFormatter->notify ("dfttest");
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

std::string getCoralRoot(MessageFormatter* messageFormatter) {
	
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

std::string getRoot(MessageFormatter* messageFormatter) {
	
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

void conditionalAdd(vector<File>& tryOut, const File& file) {
	for(File f: tryOut) {
		if(f==file) return;
	}
	tryOut.push_back(file);
}

bool tryReadFile(MessageFormatter* messageFormatter, File& file, DFTTestSuite& suite) {
	if(FileSystem::exists(file)) {
		if(suite.readTestFile(file)) {
			messageFormatter->reportError("Error loading test file: " + suite.getOrigin().getFileRealPath());
			return false;
		} else {
			messageFormatter->reportAction("Using test file: " + suite.getOrigin().getFileRealPath(),VERBOSITY_FLOW);
			return true;
		}
	} else {
		messageFormatter->reportWarning("Test file does not exist: " + file.getFileRealPath(),VERBOSITY_FILE_SEARCH);
		return false;
	}
}

int main(int argc, char** argv) {
	
	/* Command line arguments and their default settings */
	string testSuiteFileName  = "";
	int    testSuiteFileSet   = 0;
//	string testFileName       = "";
//	int    testFileSet        = 0;
	vector<Test::TestSpecification*> inputTests;

	int verbosity = 0;
	int useColoredMessages   = 1;
	int printHelp            = 0;
	string printHelpTopic    = "";
	int printVersion         = 0;
	bool useCachedOnly       = false;
	bool forcedRunning       = false;
	vector<string> limitTests;
	string outputMode        = "nice";
	
	/* Parse command line arguments */
	char c;
	while( (c = getopt(argc,argv,"Ccfh:Lqs:t:v-:")) >= 0 ) {
		switch(c) {

			// -s FILE
			case 's':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					testSuiteFileName = "";
					testSuiteFileSet = 1;
				} else {
					testSuiteFileName = string(optarg);
					testSuiteFileSet = 1;
				}
				break;

//			// -t FILE
//			case 't':
//				if(strlen(optarg)==1 && optarg[0]=='-') {
//					testFileName = "";
//					testFileSet = 1;
//				} else {
//					testFileName = string(optarg);
//					testFileSet = 1;
//				}
//				break;

			// -c
			case 'c':
				forcedRunning = false;
				useCachedOnly = true;
				break;

			// -f
			case 'f':
				useCachedOnly = false;
				forcedRunning = true;
				break;

			// -t
			case 't':
				limitTests.push_back(string(optarg));
				break;

			// -h
			case 'h':
				printHelp = true;
				break;
			
			// -C
			case 'C':
				outputMode = "csv";
				break;
			
			// -L
			case 'L':
				outputMode = "latex";
				break;
			
			// -v
			case 'v':
				++verbosity;
				break;

			// -q
			case 'q':
				--verbosity;
				break;

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
	
//	printf("args:\n");
//	for(unsigned int i=0; i<(unsigned int)argc; i++) {
//		printf("  %s\n",argv[i]);
//	}
	
	/* Create a new compiler context */
	MessageFormatter* messageFormatter = new MessageFormatter(std::cerr);
	messageFormatter->useColoredMessages(useColoredMessages);
	messageFormatter->setVerbosity(verbosity);
	messageFormatter->setAutoFlush(true);
	
	Shell::messageFormatter = messageFormatter;
	
	messageFormatter->notify("Initializing...",VERBOSITY_FLOW);
	
	
	/* Print help / version if requested and quit */
	if(printHelp) {
		print_help(messageFormatter,printHelpTopic);
		exit(0);
	}
	if(printVersion) {
		print_version(messageFormatter);
		exit(0);
	}
	
	string dft2lntRoot = getRoot(messageFormatter);
	string coralRoot = getCoralRoot(messageFormatter);
	
	if(dft2lntRoot.empty()) return 1;
	if(coralRoot.empty()) return 1;
	
	DFTTestSuite suite(messageFormatter);
	suite.setUseCachedOnly(useCachedOnly);
	suite.setForcedRunning(forcedRunning);

	/* Parse command line arguments without a -X.
	 * These specify the input files.
	 */
	 {
		int argc_current = optind;
		bool success = false;
		if(argc_current<argc) {
			string testSuiteFile = string(argv[argc_current]);
			
			vector<File> tryOut;
			
			conditionalAdd(tryOut,File(testSuiteFile));
			conditionalAdd(tryOut,File(".",testSuiteFile,Test::fileExtension));
			conditionalAdd(tryOut,File(dft2lntRoot+DFT2LNT::TESTSUBROOT,testSuiteFile));
			conditionalAdd(tryOut,File(dft2lntRoot+DFT2LNT::TESTSUBROOT,testSuiteFile,Test::fileExtension));
			
			for(File file: tryOut) {
				success = tryReadFile(messageFormatter,file,suite);
				if(success) break;
			}
			
			if(!success) {
				if(verbosity<2) {
					messageFormatter->reportWarning("No test suite found, use -vv to show the tried paths");
				}
			}
			
			argc_current++;
		}
		
		if(!success && limitTests.size()>0) {
			File suiteFile(limitTests[0]);
			suiteFile.setPathTo(".").setFileExtension(Test::fileExtension).fix();
			suite.createTestFile(suiteFile);
			success = true;
		}
	 }
	
	// Add DFT tests from file if not in the suite
	for(string loadTest: limitTests) {
		// Initially mark the file as to load
		bool load = true;
		File loadTestFile = File(loadTest).fix();
		
		if(!FileSystem::exists(loadTestFile)) load = false;
		
		// Check if the file is already in the suite, if so, mark as don't load
		if(load) {
			for(auto it=suite.getTests().begin(); it!=suite.getTests().end(); it++) {
				DFTTest* test = static_cast<DFTTest*>(*it);
				if(loadTestFile == test->getFile()) {
					load = false;
					break;
				}
			}
		}
		
		// If still marked to load, load
		if(load) {
			DFTTest* test = new DFTTest(loadTestFile);
			suite.getTests().push_back(test);
			test->setParentSuite(&suite);
		}
	}
	
	// Apply limits
	suite.applyLimitTests(limitTests);
	
	if(suite.getTestCount()>0) {
		if(verbosity>=VERBOSITY_DATA) {
			messageFormatter->notify("Going to perform these tests:",VERBOSITY_DATA);
			for(auto it=suite.getTests().begin(); it!=suite.getTests().end(); it++) {
				DFTTest* test = static_cast<DFTTest*>(*it);
				messageFormatter->reportAction(test->getFile().getFilePath(),VERBOSITY_DATA);
			}
		}
		
		Test::OutputFormatter* outputFormatter = new Test::OutputFormatterNice();
		if(outputMode=="csv") {
		} else if(outputMode=="latex") {
			outputFormatter = new Test::OutputFormatterLaTeX();
		}
		DFTTestRun run(outputFormatter,messageFormatter,dft2lntRoot,coralRoot);
		run.setHideOutput(verbosity<0);
		run.run(suite);
		messageFormatter->notify("Done.",VERBOSITY_FLOW);
	} else {
		messageFormatter->notify("Nothing done.");
	}
	
	delete messageFormatter;
	return 0;
}

void DFTTest::appendSpecific(const TestSpecification& otherGeneric) {
	const DFTTest& other = static_cast<const DFTTest&>(otherGeneric);
}

void DFTTestSuite::applyLimitTests(const vector<string>& limitTests) {
	for(Test::TestSpecification* testGeneric: tests) {
		DFTTest* test = static_cast<DFTTest*>(testGeneric);
		bool addTest = false;
		for(string ltest: limitTests) {
			if(test->getFile().getFileName() == ltest) {
				addTest = true;
			}
		}
		if(addTest) this->limitTests.push_back(test);
	}
}

Test::TestSpecification* DFTTestSuite::readYAMLNodeSpecific(const YAML::Node& node) {
	bool wentOK = true;
	
	DFTTest* test = new DFTTest();
	
	//node << YAML::
	
	if(const YAML::Node* itemNode = node.FindValue("timeunits")) {
		unsigned int value;
		try { *itemNode >> value; }
		catch(YAML::Exception& e) { reportYAMLException(e); wentOK = false; }
		test->setTimeUnits(value);
	}
	if(const YAML::Node* itemNode = node.FindValue("dft")) {
		std::string dft;
		try { *itemNode >> dft; }
		catch(YAML::Exception& e) { reportYAMLException(e); wentOK = false; }
		test->setFile(File(dft).fixWithOrigin(getOrigin().getPathTo()));
	}
	if(const YAML::Node* itemNode = node.FindValue("evidence")) {
		std::vector<std::string>& evidence = test->getEvidence();
		/* The evidence should be a sequence... */
		if(itemNode->Type()==YAML::NodeType::Sequence) {
			
			/* It can be a sequence of sequences of strings */
//			YAML::Iterator itR = itemNode->begin();
//			if(itR!=itemNode->end() && itR->Type()==YAML::NodeType::Sequence) {
//				messageFormatter->message("Sequence of sequences of strings");
//				try {
//					*itemNode >> evidence;
//				} catch(YAML::Exception& e) {
//					reportYAMLException(e);
//					wentOK = false;
//				}
			
			/* Or just a sequence of strings */
//			} else {
				try {
					*itemNode >> evidence;
				} catch(YAML::Exception& e) {
					reportYAMLException(e);
					wentOK = false;
				}
//			}
		} else {
			messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),node.GetMark().line),"expected sequence of node names");
		}
	}
	
	if(wentOK) return test;
error:
	if(test) delete test;
	return NULL;
		
}

void DFTTestSuite::writeYAMLNodeSpecific(Test::TestSpecification* testGeneric, YAML::Emitter& out) {
	DFTTest* test = static_cast<DFTTest*>(testGeneric);
	out << YAML::Key   << "timeunits";
	out << YAML::Value << test->getTimeUnits();
	out << YAML::Key   << "dft";
	out << YAML::Value << test->getFile().getFilePath();
	out << YAML::Key   << "evidence";
//	if(test->getEvidence().size()==1) {
//		out << YAML::Value << test->getEvidence()[0];
//	} else {
		out << YAML::Value << test->getEvidence();
//	}
}

void DFTTestSuite::originChanged(const File& from) {
	for(Test::TestSpecification* testGeneric: tests) {
		DFTTest* test = static_cast<DFTTest*>(testGeneric);
		File oldFile = test->getFile();
		//std::cerr << "Updating DFT file from " << test->getFile().getFileRealPath() << " ( " << test->getFile().getFilePath() << " ) ";
		test->setFile(oldFile.newWithPathTo(from.getPathTo()));
		//std::cerr << " to " << test->getFile().getFileRealPath() << " ( " << test->getFile().getFilePath() << " ) " << std::endl;
	}
}


void DFTTestSuite::setMessageFormatter(MessageFormatter* messageFormatter) {
	this->messageFormatter = messageFormatter;
}

int DFTTestRun::handleSignal(int signal) {
	//cout << "HELLO " << messageFormatter << endl;
	if(messageFormatter) {
		ConsoleWriter& cw = messageFormatter->getConsoleWriter();
		messageFormatter->getConsoleWriter().appendPostfix();
		stringstream ss;
		ss << "Caught signal `" << signal << "', ";
		cw << " " << ConsoleWriter::Color::CyanBright << ">" << ConsoleWriter::Color::WhiteBright << "  " << ss.str();
		while(true) {
			cw << "terminate " << ConsoleWriter::Color::MagentaBright << "i" << ConsoleWriter::Color::WhiteBright << "teration";
			cw << ", "         << ConsoleWriter::Color::MagentaBright << "t" << ConsoleWriter::Color::WhiteBright << "est";
			cw << " or "       << ConsoleWriter::Color::MagentaBright << "p" << ConsoleWriter::Color::WhiteBright << "rogram";
			cw << "?" << ConsoleWriter::Color::Reset;
			char answer[10];
			std::cin.getline(answer,9);
			if(!strncmp("i",answer,1)) {
				return 0;
			} else if(!strncmp("t",answer,1)) {
				requestStopTest = true;
				return SIGINT;
			} else if(!strncmp("p",answer,1)) {
				requestStopSuite = true;
				return SIGQUIT;
			} else {
				cw << " " << ConsoleWriter::Color::CyanBright << ">" << ConsoleWriter::Color::WhiteBright << "  ";
			}
		}
		cw.appendPostfix();
	}
	return signal;
}

DFTTestResult* DFTTestRun::runDftcalc(DFTTest* test) {
	// Result
	DFTTestResult* result = new DFTTestResult();
	
	// Run dftcalc
	File outputDir("output_dftcalc");
	outputDir.fix();
	FileSystem::mkdir(outputDir);
	File dftcalcResultFile = File(outputDir.getFilePath(),test->getFile().getFileBase(),"result.dftcalc");
	File bcgFile  = File(outputDir.getFilePath(),test->getFile().getFileBase(),"bcg");
	File statFile = File(outputDir.getFilePath(),test->getFile().getFileBase()+".stats","log");
	File svlLogFile = File(outputDir.getFilePath(),test->getFile().getFileBase(),"log");
	FileSystem::remove(dftcalcResultFile);
	string dftcalc = dft2lntRoot + "/bin/dftcalc '" + test->getFile().getFileRealPath() + "'"
	                                                + " "    + test->getTimeDftcalc()
	                                                + " -C " + outputDir.getFilePath()
	                                                + " -r " + dftcalcResultFile.getFileRealPath()
	                                                ;
	dftcalc += " -e \"";
	for(std::string e: test->getEvidence()) {
		dftcalc += e;
		dftcalc += ",";
	}
	dftcalc += "\"";
	
	//clock_t start = clock();
	Shell::SystemOptions options;
	options.command = dftcalc;
	options.verbosity = VERBOSITY_EXECUTIONS;
	if(Shell::memtimeAvailable()) {
		messageFormatter->reportAction("Using memtime for resource statistics",VERBOSITY_DATA);
		options.statProgram = "memtime";
		options.statFile = statFile.getFileRealPath();
	}
	options.signalHandler = [this](int signal) -> int { return this->handleSignal(signal); };
	Shell::RunStatistics stats;
	Shell::system(options,&stats);
	
	//clock_t end = clock();
	result->stats = stats;
	
	// Obtain dftcalc result
	std::map<string,DFT::DFTCalculationResult> dftcalcResults;
	{
		std::ifstream fin(dftcalcResultFile.getFileRealPath());
		YAML::Parser parser(fin);
		YAML::Node doc;
		if(parser.GetNextDocument(doc)) {
			doc >> dftcalcResults;
		}
	}
	map<string,DFT::DFTCalculationResult>::iterator res = dftcalcResults.find(test->getFile().getFileName());
	result->failprob = -1;
	if(res != dftcalcResults.end()) {
		result->stats.maxMem(res->second.stats);
		// HACK assumes that we have a single result item
		for(auto it: res->second.failprobs) {
			result->failprob = it.failprob;
			break;
		}
	}
	
	// Read memory usage from SVL log file
	// FIXME: this is the wrong SVL log file... where to find one for coral?
	Shell::RunStatistics svlStats;
	if(Shell::readMemtimeStatisticsFromLog(svlLogFile,svlStats)) {
		messageFormatter->reportWarning("Could not read from svl log file `" + svlLogFile.getFileRealPath() + "'");
	} else {
		messageFormatter->reportAction2("Read from svl log file `" + svlLogFile.getFileRealPath() + "'",VERBOSITY_DATA);
		result->stats.maxMem(svlStats);
	}
	
	// Read statistics of BCG file
	DFT::CADP::BCGInfo bcgInfo;
	if(DFT::CADP::BCG_Info(bcgFile,bcgInfo)) {
		messageFormatter->reportWarning("Could not read from BCG file `" + bcgFile.getFileRealPath() + "'");
	} else {
		result->bcgInfo = bcgInfo;
		messageFormatter->reportAction2("Read from BCG file `" + bcgFile.getFileRealPath() + "'",VERBOSITY_DATA);
	}
	
	return result;
}

DFTTestResult* DFTTestRun::runCoral(DFTTest* test) {
	// Result
	DFTTestResult* result = new DFTTestResult();
	
	File outputDir("output_coral");
	outputDir.fix();
	FileSystem::mkdir(outputDir);
	File coralResultFile = File(outputDir.getFilePath(),test->getFile().getFileBase(),"result.coral");
	File bcgFile  = File(outputDir.getFilePath(),test->getFile().getFileBase()+".coral","bcg");
	File statFile = File(outputDir.getFilePath(),test->getFile().getFileBase()+".stats","log");
	File svlLogFile = File(outputDir.getFilePath(),"uniformizer_error","log");
	File mrmcLogFile = File(outputDir.getFilePath(),"mrmc_error","log");
	File dft2bcgLogFile = File(outputDir.getFilePath(),"conversion_error","log");
	File compositionLogFile = File(outputDir.getFilePath(),"composition_error","log");
	File activateLogFile = File(outputDir.getFilePath(),"activate_error","log");
	File imc2ctmdpLogFile = File(outputDir.getFilePath(),"imc2ctmdp_error","log");
	
	set<File> logList = { svlLogFile, mrmcLogFile, dft2bcgLogFile, compositionLogFile, activateLogFile, imc2ctmdpLogFile};
	
	FileSystem::remove(coralResultFile);
	string coral = coralRoot + "/coral -f '" + test->getFile().getFileRealPath() + "'"
	                                         + " "   + test->getTimeCoral()
	                                         + " -C " + bcgFile.getPathTo() + "/" + bcgFile.getFileBase()
	                                         + " -M " + outputDir.getFilePath()
	                                         + " -l " + outputDir.getFilePath()
	                                         + " -O " + coralResultFile.getFileRealPath()
	                                         ;
	Shell::SystemOptions options;
	options.command = coral;
	options.verbosity = VERBOSITY_EXECUTIONS;
	if(Shell::memtimeAvailable()) {
		messageFormatter->reportAction("Using memtime for resource statistics",VERBOSITY_DATA);
		options.statProgram = "memtime";
		options.statFile = statFile.getFileRealPath();
	}
	Shell::RunStatistics stats;
	Shell::system(options,&stats);//,".",coralResultFile.getFileRealPath());

	std::ifstream fin(coralResultFile.getFileRealPath());
	char buffer[1000];
	result->failprob = -1;
	switch(0) default: {
		// Skip the header
		fin.getline(buffer,1000);
		//cerr << "NOW: " << string(buffer) << endl;
		if(strncmp("Time",buffer,4)) {
			break;
		}
		//cerr << "CONTINUE" << endl;
		
		// FIXME: make sure it's the correct one?
		fin.getline(buffer,1000);
		char* c = buffer;
		while(*c && *c!=',') c++;
		if(*c==',') {
			c+=2;
			//cerr << "SCANNING" << string(c) << endl;
			sscanf(c,"%lf",&result->failprob);
			break;
		}
	}
	
	// Read memory usage from SVL log file
	// FIXME: this is the wrong SVL log file... where to find one for coral?
	for(File logFile: logList) {
		Shell::RunStatistics logStats;
		if(Shell::readMemtimeStatisticsFromLog(logFile,logStats)) {
			messageFormatter->reportWarning("Could not read from svl log file `" + logFile.getFileRealPath() + "'");
		} else {
			stats.maxMem(logStats);
		}
	}
	
	result->stats = stats;
	
	// Read statistics of BCG file
	DFT::CADP::BCGInfo bcgInfo;
	if(DFT::CADP::BCG_Info(bcgFile,bcgInfo)) {
		messageFormatter->reportWarning("Could not read from BCG file `" + bcgFile.getFileRealPath() + "'");
	}
	result->bcgInfo = bcgInfo;
	
	return result;
}

void DFTTest::getLastResults(map<string,double>& results) {
	
	// Loop over all the results, starting with the first
	auto it = getResults().begin();
	for(;it!=getResults().end(); ++it) {
		
		// Loop over all the iterations of the current result
		auto it2 = it->second.begin();
		for(;it2!=it->second.end(); ++it2) {
			
			// If the result is valid...
			DFTTestResult* result = static_cast<DFTTestResult*>(it2->second);
			if(result->failprob>=0) {
				
				// insert the result in the output map
				results[it2->first] = result->failprob;
			}
		}
		
	}
}

::Test::ResultStatus DFTTest::verify(::Test::TestResult* resultGeneric) {
	DFTTestResult* result = static_cast<DFTTestResult*>(resultGeneric);
	double verified = getVerifiedValue();
	//std::cerr << "Comparing " << result->failprob << ", and " << verified;
	if(result->failprob>=0 && (verified<0 || verified==result->failprob)) {
		return ::Test::OK;
	} else {
		return ::Test::FAILED;
	}
}

void DFTTestRun::fillDisplayMap(Test::TestSpecification* testGeneric, string timeStamp, string iteration, Test::TestResult* resultGeneric, bool cached, std::map<std::string,std::string>& content, std::map<std::string,ConsoleWriter::Color>& colors) {
	DFTTest* test = static_cast<DFTTest*>(testGeneric);
	DFTTestResult* result = static_cast<DFTTestResult*>(resultGeneric);
	if(result->failprob>=0) {
		std::stringstream ss;
		ss << result->failprob;
		content["failprob"] = ss.str();
	} else {
		content["failprob"] = "-";
	}
	if(result->bcgInfo.states>0) {
		std::stringstream ss;
		ss << result->bcgInfo.states;
		content["states"] = ss.str();
	} else {
		content["states"] = "-";
	}
	if(result->bcgInfo.transitions>0) {
		std::stringstream ss;
		ss << result->bcgInfo.transitions;
		content["transitions"] = ss.str();
	} else {
		content["transitions"] = "-";
	}
	
	// Fill in relative result to compareBase
	if(result->failprob>=0) {
		std::pair<std::string,Test::TestResult*> baseResultGeneric = test->getLastResult(compareBase);
		DFTTestResult* baseResult = static_cast<DFTTestResult*>(baseResultGeneric.second);
		if(baseResult) {
			std::stringstream ss;
			ss << baseResult->stats.time_monraw/result->stats.time_monraw;
			content["speedup"] = ss.str();
		} else {
			content["speedup"] = "-";
		}
	} else {
		content["speedup"] = "-";
	}
}

Test::TestResult* DFTTestRun::runSpecific(Test::TestSpecification* testGeneric, const string& timeStamp, const string& iteration) {
	DFTTest* test = static_cast<DFTTest*>(testGeneric);
	
	if(iteration=="dftcalc") {
		return runDftcalc(test);
	} else {
		return runCoral(test);
	}
	
}
