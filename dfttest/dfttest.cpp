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
#include "dft2lnt.h"
#include "compiletime.h"
#include "dftcalc.h"
#ifdef HAVE_CADP
#include "CADP.h"
#endif

const int VERBOSITY_FLOW = 1;
const int VERBOSITY_DATA = 2;
const int VERBOSITY_FILE_SEARCH = 2;

void DFTTestResult::readYAMLNodeSpecific(const YAML::Node& node) {
	if(const YAML::Node itemNode = node["failprob"]) {
		failprob = itemNode.as<double>();
	}
#ifdef HAVE_CADP
	if(const YAML::Node itemNode = node["bcginfo"]) {
		itemNode >> bcgInfo;
	}
#endif
}
void DFTTestResult::writeYAMLNodeSpecific(YAML::Emitter& out) const {
	out << YAML::Key << "failprob" << YAML::Value << failprob;
#ifdef HAVE_CADP
	if(bcgInfo.willWriteSomething()) out << YAML::Key << "bcginfo" << YAML::Value << bcgInfo;
#endif
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


std::string getRoot(MessageFormatter* messageFormatter) {
	
	char* root = getenv((const char*)"DFT2LNTROOT");
	std::string dft2lntRoot = root ? string(root) : "";

	if (dft2lntRoot == "")
		dft2lntRoot = DFT2LNTROOT;

	for (int i = dft2lntRoot.length() - 1; i >= 0; i--) {
		if (dft2lntRoot[i] == '\\')
			dft2lntRoot[i] = '/';
	}
	if (dft2lntRoot[dft2lntRoot.length() - 1] == '/')
		dft2lntRoot = dft2lntRoot.substr(0, dft2lntRoot.length() - 1);

	if (!FileSystem::isDir(dft2lntRoot)) {
		if (messageFormatter)
			messageFormatter->reportError("DFT2LNTROOT does not exist or is not a directory: " + dft2lntRoot);
	}
	
#ifdef HAVE_CADP
	struct stat rootStat;
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
#endif
	
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

	/* Create a new compiler context */
	MessageFormatter* messageFormatter = new MessageFormatter(std::cerr);

	/* Parse command line arguments */
	int argi;
	for (argi = 1; argi < argc; argi++) {
		if (argv[argi][0] != '-')
			break;
		if (!strcmp(argv[argi], "--")) {
			argi++;
			break;
		}

		if (!strcmp(argv[argi], "-s")) {
			// -s FILE
			if(!strcmp(argv[++argi], "-"))
				testSuiteFileName = "";
			else
				testSuiteFileName = string(argv[++argi]);
			testSuiteFileSet = 1;
		} else if (!strcmp(argv[argi], "-c")) {
			forcedRunning = false;
			useCachedOnly = true;
		} else if (!strcmp(argv[argi], "-f")) {
			useCachedOnly = false;
			forcedRunning = true;
		} else if (!strcmp(argv[argi], "-t")) {
			// -t TREE
			limitTests.push_back(string(argv[++argi]));
		} else if (!strcmp(argv[argi], "-h")) {
			printHelp = true;
		} else if (!strcmp(argv[argi], "-C")) {
			outputMode = "csv";
		} else if (!strcmp(argv[argi], "-L")) {
			outputMode = "latex";
		} else if (!strncmp(argv[argi], "-v", 2)) {
			for (size_t i = 1; i < strlen(argv[argi]); i++) {
				if (argv[argi][i] == 'v') {
					++verbosity;
				} else {
					messageFormatter->reportError(std::string("Unknown argument: ") + argv[argi]);
					printHelp = true;
					break;
				}
			}
		} else if (!strcmp(argv[argi], "-q")) {
			verbosity = 0;
		} else if (!strncmp("--help", argv[argi], 6)) {
			printHelp = true;
			if(strlen(argv[argi]) > 7 && argv[argi][6]=='=') {
				printHelpTopic = string(argv[argi] + 7);
			}
		} else if (!strcmp("--version", argv[argi])) {
			printVersion = true;
		} else if (!strcmp("--color", argv[argi])) {
			useColoredMessages = true;
		} else if (!strncmp("--verbose", argv[argi], 9)) {
			if(strlen(argv[argi]) > 10 && argv[argi][9] == '=') {
				verbosity = atoi(argv[argi] + 10);
			} else {
				++verbosity;
			}
		} else if (!strcmp("--no-color", argv[argi])) {
			useColoredMessages = false;
		} else {
			std::cerr << "Unknown argument: " << argv[argi] << "\n";
			printHelp = true;
		}
	}
	
//	printf("args:\n");
//	for(unsigned int i=0; i<(unsigned int)argc; i++) {
//		printf("  %s\n",argv[i]);
//	}
	
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
	
	if(dft2lntRoot.empty()) return 1;
	
	DFTTestSuite suite(messageFormatter);
	suite.setUseCachedOnly(useCachedOnly);
	suite.setForcedRunning(forcedRunning);

	/* Parse command line arguments without a -X.
	 * These specify the input files.
	 */
	 {
		int argc_current = argi;
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
		DFTTestRun run(outputFormatter,messageFormatter,dft2lntRoot);
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
	
	if(const YAML::Node itemNode = node["timeunits"]) {
		unsigned int value;
		try {
			value = itemNode.as<unsigned int>();
		} catch(YAML::Exception& e) {
			reportYAMLException(e);
			goto error;
		}
		test->setTimeUnits(value);
	}
	if(const YAML::Node itemNode = node["dft"]) {
		std::string dft;
		try { dft = itemNode.as<std::string>(); }
		catch(YAML::Exception& e) { reportYAMLException(e); wentOK = false; }
		test->setFile(File(dft).fixWithOrigin(getOrigin().getPathTo()));
	}
	if(const YAML::Node itemNode = node["evidence"]) {
		std::vector<std::string>& evidence = test->getEvidence();
		/* The evidence should be a sequence of strings */
		if(itemNode.Type()==YAML::NodeType::Sequence) {
			try {
				for (YAML::const_iterator it = itemNode.begin(); it != itemNode.end(); it++) {
					evidence.push_back(it->as<std::string>());
				}
			} catch(YAML::Exception& e) {
				fprintf(stderr, "Error pushing back or converting.\n");
				reportYAMLException(e);
				wentOK = false;
			}
		} else {
			messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),node.Mark().line),"expected sequence of node names");
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
#ifndef WIN32
				return SIGQUIT;
#else
				return SIGTERM;
#endif
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
		std::vector<YAML::Node> docs = YAML::LoadAll(fin);
		if(!docs.empty()) {
			docs[0] >> dftcalcResults;
		}
	}
	map<string,DFT::DFTCalculationResult>::iterator res = dftcalcResults.find(test->getFile().getFileName());
	result->failprob = -1;
	if(res != dftcalcResults.end()) {
		result->stats.maxMem(res->second.stats);
		// HACK assumes that we have a single result item
		for(auto it: res->second.failProbs) {
			result->failprob = stod(it.valStr());
			break;
		}
	}

#ifdef HAVE_CADP
	// Read memory usage from SVL log file
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
#endif

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
#ifdef HAVE_CADP
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
#endif
	
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
	
	return runDftcalc(test);
}
