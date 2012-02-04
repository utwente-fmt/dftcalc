/*
	testsuite <tests>
	testsuite -t <test>
	testsuite -s <suitefile>


	testsuite <tests>
	 -> loop over <tests>, performing them

	testsuite -s <suitefile>
	 -> read suitefile
	 -> loop over tests in <suitefile>, performing them
	 
	testsuite -t <test>
	 -> perform <test>

	To perform a test:
	- loop over all the commands
	  - save intermediate results
	  - verify intermediate results
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

const int VERBOSITY_FLOW = 1;
const int VERBOSITY_DATA = 1;
const int VERBOSITY_FILE_SEARCH = 2;

void DFTTestResult::readYAMLNodeSpecific(const YAML::Node& node) {
	if(const YAML::Node* itemNode = node.FindValue("failprob")) {
		*itemNode >> failprob;
	}
}
void DFTTestResult::writeYAMLNodeSpecific(YAML::Emitter& out) const {
	out << YAML::Key << "failprob" << YAML::Value << failprob;
}

void print_help(MessageFormatter* messageFormatter, string topic) {
	if(topic.empty()) {
		messageFormatter->notify ("dfttest [options] [suite.test]");
		messageFormatter->message("  Calculates the failure probability for the DFT files in the specified test");
		messageFormatter->message("  file. Result is written to stdout and saved in the test file.");
		messageFormatter->message("  Check dfttest --help=output for more details regarding the output.");
		messageFormatter->message("");
		messageFormatter->notify ("General Options:");
		messageFormatter->message("  -h, --help      Show this help.");
		messageFormatter->message("  --color         Use colored messages.");
		messageFormatter->message("  --no-color      Do not use colored messages.");
		messageFormatter->message("  --version       Print version info and quit.");
		messageFormatter->message("");
		messageFormatter->notify ("Debug Options:");
		messageFormatter->message("  --verbose=x     Set verbosity to x, -1 <= x <= 5.");
		messageFormatter->message("  -v, --verbose   Increase verbosity. Up to 5 levels.");
		messageFormatter->message("  -q              Decrease verbosity.");
		messageFormatter->message("");
		messageFormatter->notify ("Test Options:");
		messageFormatter->message("  -c              Do not run tests, only show cached results.");
		messageFormatter->message("  -f              Force running all tests, regardless of cached results");
		messageFormatter->message("  -t DFTFILE      Limit testing to this DFT. Multiple allowed.");
		messageFormatter->flush();
	} else if(topic=="output") {
		messageFormatter->notify ("Timing Output");
		messageFormatter->message("  Time measurements are done using platform specific implementations.");
		messageFormatter->message("  On Linux, CLOCK_MONOTONIC_RAW is used and on Windows the API call");
		messageFormatter->message("  QueryPerformanceCounter is used.");
		messageFormatter->message("  Both implementations aim to assure there is no influence from other");
		messageFormatter->message("  programs such as NTP. The measurement is as accurate as the clock of ");
		messageFormatter->message("  the hardware is.");
	} else {
		messageFormatter->reportAction("Unknown help topic: " + topic);
	}		
}

void print_version(MessageFormatter* messageFormatter) {
	messageFormatter->notify ("dfttest");
	messageFormatter->message(string("  built on ") + COMPILETIME_DATE);
	{
		FileWriter out;
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
		//if(messageFormatter) messageFormatter->reportError("Environment variable `CORAL' not set. Please set it to where coral can be found.");
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
		//if(messageFormatter) messageFormatter->reportError("Environment variable `DFT2LNTROOT' not set. Please set it to where lntnodes/ can be found.");
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
		suite.readTestFile(file);
		messageFormatter->reportAction("Using test file: " + file.getFileRealPath(),VERBOSITY_FLOW);
		return true;
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
	vector<Test::Test*> inputTests;

	int verbosity = 0;
	int useColoredMessages   = 1;
	int printHelp            = 0;
	string printHelpTopic    = "";
	int printVersion         = 0;
	bool useCachedOnly       = false;
	bool forcedRunning       = false;
	vector<string> limitTests;
	
	/* Parse command line arguments */
	char c;
	while( (c = getopt(argc,argv,"cfh:qs:t:v-:")) >= 0 ) {
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

	DFTTestSuite suite(messageFormatter);
	suite.setUseCachedOnly(useCachedOnly);
	suite.setForcedRunning(forcedRunning);

	/* Parse command line arguments without a -X.
	 * These specify the input files.
	 */
	int argc_current = optind;
	if(argc_current<argc) {
		string testSuiteFile = string(argv[argc_current]);
		
		bool success = false;
		
		vector<File> tryOut;
		
		conditionalAdd(tryOut,File(testSuiteFile));
		conditionalAdd(tryOut,File(".",testSuiteFile,"test"));
		conditionalAdd(tryOut,File(dft2lntRoot+DFT2LNT::TESTSUBROOT,testSuiteFile));
		conditionalAdd(tryOut,File(dft2lntRoot+DFT2LNT::TESTSUBROOT,testSuiteFile,"test"));
		
		for(File file: tryOut) {
			success = tryReadFile(messageFormatter,file,suite);
			if(success) break;
		}
		
		if(!success) {
			if(verbosity<2) {
				messageFormatter->reportWarning("No tests found, use -vv to show the tried paths");
			}
		}
		
		argc_current++;
	}
	
	suite.applyLimitTests(limitTests);

	if(suite.getTestCount()>0) {
		DFTTestRun run(messageFormatter,dft2lntRoot,coralRoot);
		run.run(suite);
	} else {
		messageFormatter->reportWarning("No tests performed");
	}
	
	delete messageFormatter;
	return 0;
}

void DFTTestSuite::applyLimitTests(const vector<string>& limitTests) {
	for(Test::Test* testGeneric: tests) {
		DFTTest* test = static_cast<DFTTest*>(testGeneric);
		bool addTest = false;
		cerr << "test: " << test->getFile().getFileName() << endl;
		for(string ltest: limitTests) {
			cerr << "  ltest: " << ltest << endl;
			if(test->getFile().getFileName() == ltest) {
				cerr << "    BAM" << endl;
				addTest = true;
			}
		}
		if(addTest) this->limitTests.push_back(test);
	}
	for(Test::Test* testGeneric: this->limitTests) {
		cerr << "Limit test: " << testGeneric->getFullname() << endl;
	}
}

Test::Test* DFTTestSuite::readYAMLNodeSpecific(const YAML::Node& node) {
	bool wentOK = true;
	
	DFTTest* test = new DFTTest();
	
	//node << YAML::
	
	if(const YAML::Node* itemNode = node.FindValue("dft")) {
		std::string dft;
		try { *itemNode >> dft; }
		catch(YAML::Exception& e) { if(messageFormatter) messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),e.mark.line),e.msg); wentOK = false; }
		test->setFile(File(dft));
	} else {
		if(messageFormatter) messageFormatter->reportErrorAt(Location(getOrigin().getFileRealPath(),node.GetMark().line),"Test does not specify a DFT file");
		wentOK = false;
		goto error;
	}
	if(const YAML::Node* itemNode = node.FindValue("verified")) {
		if(itemNode->Type()==YAML::NodeType::Map) {
			for(YAML::Iterator it = itemNode->begin(); it!=itemNode->end(); ++it) {
				string key;
				double value;
				try {
					it.first() >> key;
					it.second() >> value;
					//cerr << "Found verified result: " << key << " -> " << value << endl;
					test->getVerifiedResults().insert(pair<std::string,double>(key,value));
				} catch(YAML::Exception& e) {
					if(messageFormatter) messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),e.mark.line),e.msg);
					wentOK = false;
				}
			}
		}
	}
	if(const YAML::Node* itemNode = node.FindValue("results")) {
		if(itemNode->Type()==YAML::NodeType::Sequence) {
			
			// Iterate over the testruns
			for(YAML::Iterator itR = itemNode->begin(); itR!=itemNode->end(); ++itR) {
				
				// 
				for(YAML::Iterator it = itR->begin(); it!=itR->end(); ++it) {
					string resultTime;
					it.first() >> resultTime;
					//cerr << "Found result: " << resultTime << it.second().Type() << endl;
					std::map<std::string,DFTTestResult>& results = test->getResults()[resultTime];
					if(it.second().Type()==YAML::NodeType::Map) {
						
						// Iterate over individual results of a testrun
						for(YAML::Iterator it2 = it.second().begin(); it2!=it.second().end(); ++it2) {
							string iteration;
							DFTTestResult iterationResults;
							try {
								it2.first() >> iteration;
								it2.second() >> iterationResults;
								results.insert(pair<std::string,DFTTestResult>(iteration,iterationResults));
							} catch(YAML::Exception& e) {
								if(messageFormatter) messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),e.mark.line),e.msg);
								wentOK = false;
							}
						}
					}
				}
			}
		}
	}
	
	if(wentOK) return test;
error:
	if(test) delete test;
	return NULL;
		
}

void DFTTestSuite::writeYAMLNodeSpecific(Test::Test* testGeneric, YAML::Emitter& out) {
	DFTTest* test = static_cast<DFTTest*>(testGeneric);
	out << YAML::Key   << "dft";
	out << YAML::Value << test->getFile().getFilePath();
	out << YAML::Key   << "verified";
	out << YAML::Value << test->getVerifiedResults();
	out << YAML::Key   << "results";
	out << YAML::Value << YAML::BeginSeq;
	for(auto it=test->getResults().begin(); it!=test->getResults().end(); ++it) {
		out << YAML::BeginMap;
		out << YAML::Key   << it->first;
		out << YAML::Value << it->second;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
}

void DFTTestSuite::setMessageFormatter(MessageFormatter* messageFormatter) {
	this->messageFormatter = messageFormatter;
}

DFTTestResult DFTTestRun::runDftcalc(DFTTest* test) {
	// Result
	DFTTestResult result;
	
	// Run dftcalc
	File dftcalcResultFile = File("output",test->getFile().getFileBase(),"result.dftcalc");
	FileSystem::remove(dftcalcResultFile);
	string dftcalc = dft2lntRoot + "/bin/dftcalc '" + test->getFile().getFileRealPath() + "' " + test->getTimeDftcalc() + " -r " + dftcalcResultFile.getFileRealPath();
	
	//clock_t start = clock();
	Shell::RunStatistics stats;
	Shell::system(dftcalc,VERBOSITY_EXECUTIONS,&stats);
	//clock_t end = clock();
	result.time_user    = stats.time_user;
	result.time_system  = stats.time_system;
	result.time_elapsed = stats.time_elapsed;
	result.time_monraw  = stats.time_monraw;
	result.mem_virtual  = stats.mem_virtual;
	result.mem_resident = stats.mem_resident;
	
	// Obtain dftcalc result
	std::map<string,double> dftcalcResults;
	{
		std::ifstream fin(dftcalcResultFile.getFileRealPath());
		YAML::Parser parser(fin);
		YAML::Node doc;
		if(parser.GetNextDocument(doc)) {
			doc >> dftcalcResults;
		}
	}
	map<string,double>::iterator res = dftcalcResults.find(test->getFile().getFileName());
	if(res != dftcalcResults.end()) {
		result.failprob = res->second;
	} else {
		result.failprob = -1;
	}
	return result;
}

DFTTestResult DFTTestRun::runCoral(DFTTest* test) {
	// Result
	DFTTestResult result;
	
	File coralResultFile = File("output",test->getFile().getFileBase(),"result.coral");
	FileSystem::remove(coralResultFile);
	string coral = coralRoot + "/coral -f '" + test->getFile().getFileRealPath() + "' -C output " + test->getTimeCoral() + " -O " + coralResultFile.getFileRealPath();;
	
	Shell::RunStatistics stats;
	Shell::system(coral,VERBOSITY_EXECUTIONS,&stats);//,".",coralResultFile.getFileRealPath());
	
	result.time_user    = stats.time_user;
	result.time_system  = stats.time_system;
	result.time_elapsed = stats.time_elapsed;
	result.time_monraw  = stats.time_monraw;
	result.mem_virtual  = stats.mem_virtual;
	result.mem_resident = stats.mem_resident;
	
	std::ifstream fin(coralResultFile.getFileRealPath());
	char buffer[1000];
	result.failprob = -1;
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
			sscanf(c,"%lf",&result.failprob);
			break;
		}
	}
	return result;
}

void DFTTestRun::displayResult(DFTTest* test, string timeStamp, string iteration, DFTTestResult& result, double verified, bool cached) {
	stringstream ss;
	stringstream ss_time;
	ss.precision(10);
	ss_time.precision(10);
	ss << result.failprob;
	ss_time << result.time_monraw;
	if(result.failprob<0) {
		reportTestFailure(test,iteration,"n/a","",cached);
	} else {
		if(verified<0) {
			reportTestUndecided(test,iteration,ss.str(),ss_time.str(),cached);
		} else {
			if(verified==result.failprob) {
				reportTestSuccess(test,iteration,ss.str(),ss_time.str(),cached);
			} else {
				reportTestFailure(test,iteration,ss.str(),ss_time.str(),cached);
			}
		}
	}
}

DFTTestResult DFTTestRun::getLastResult(DFTTest* test, string iteration) {
	auto it = test->getResults().rbegin();
	for(;it!=test->getResults().rend(); ++it) {
		std::map<string,DFTTestResult>::iterator it2 = it->second.find(iteration);
		if(it2 != it->second.end()) {
			//if(it2!=test->getVerifiedResults().end()) { // FIXME: what does this if do?
			if(it2->second.failprob>=0) {
				return it2->second;
			}
		}
	}
	return DFTTestResult();
}

void DFTTestRun::getLastResults(DFTTest* test, map<string,double>& results) {
	auto it = test->getResults().begin();
	for(;it!=test->getResults().end(); ++it) {
		auto it2 = it->second.begin();
		for(;it2!=it->second.end(); ++it2) {
			//results.insert(pair<string,double>(it2->first,it2->second));
			results[it2->first] = it2->second.failprob;
		}
		
//			if(it2 != it->second.end()) {
//				if(it2!=test->getVerifiedResults().end()) {
//					return it2->second;
//				}
//			}
	}
}

bool DFTTestRun::checkCached(DFTTest* test, string iteration, double verified, DFTTestResult& result) {
	DFTTestResult resultCached = getLastResult(test,iteration);
	
	// If forced running all tests is enabled, do so
	if(test->getParentSuite()->getForcedRunning()) {
		return false;
	}
	
	// If using only cache is enabled, do so
	if(test->getParentSuite()->getUseCachedOnly()) {
		result = resultCached;
		return true;
	}
	
	// In the normal case, use cache if it is a proper result
	if(verified>=0 && resultCached.failprob==verified) {
		result = resultCached;
		return true;
	}
	return false;
}

void DFTTestRun::runSpecific(Test::Test* testGeneric) {
	DFTTest* test = static_cast<DFTTest*>(testGeneric);
	
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	char buffer[100];
	strftime(buffer,100,"%Y-%m-%d %H:%M:%S",timeinfo);
	string timeStamp = string(buffer);
	
	double verified = -1;
	string verifiedDesc = "";
	ConsoleWriter::Color verifiedColor = ConsoleWriter::Color::Reset;
	//std::map<string,double>::iterator it = test->getVerifiedResults().find("manual");
	std::map<string,double>::iterator it = test->getVerifiedResults().begin();
	if(it!=test->getVerifiedResults().end()) {
		verified = it->second;
		verifiedDesc = it->first;
		verifiedColor = ConsoleWriter::Color::GreenBright;
	} else {
		map<string,double> results;
		getLastResults(test,results);
		bool allSameAndUseful = true;
		auto it2 = results.begin();
		if(it2!=results.end()) {
			verified = it2->second;
			for(;it2!=results.end();++it2) {
				if(it2->second!=verified) allSameAndUseful = false;
			}
		} else {
			allSameAndUseful = false;
		}
		if(allSameAndUseful) {
			verifiedDesc = "agreement";
			verifiedColor = ConsoleWriter::Color::Green;
		} else {
			verified = -1;
		}
	}
	
	if(verified<0) {
		verifiedColor = ConsoleWriter::Color::Yellow;
		verifiedDesc = "no verification";
	}
	
	{
		stringstream ss;
		if(verified>=0) {
			ss.precision(10);
			ss << verified;
		}
		reportTestStart(test,test->getFullname() + " (" + test->getFile().getFileName()+ ")",verifiedDesc,verifiedColor,ss.str());
	}
	
	// Run DFTCalc
	DFTTestResult dftResult;
	{
		bool useCached = checkCached(test,"dftcalc",verified,dftResult);
		if(!useCached) {
			dftResult = runDftcalc(test);
			test->getResults()[timeStamp]["dftcalc"] = dftResult;
			test->getParentSuite()->updateOrigin();
		}
		displayResult(test,timeStamp,"dftcalc",dftResult,verified,useCached);
	}
	
	// Run Coral
	DFTTestResult coralResult;
	{
		bool useCached = checkCached(test,"coral",verified,coralResult);
		if(!useCached) {
			coralResult = runCoral(test);
			test->getResults()[timeStamp]["coral"] = coralResult;
			test->getParentSuite()->updateOrigin();
		}
		displayResult(test,timeStamp,"coral",coralResult,verified,useCached);
	}
	
	// Check
	reportTestEnd(test,dftResult.failprob>=0 && dftResult.failprob==coralResult.failprob);
}
