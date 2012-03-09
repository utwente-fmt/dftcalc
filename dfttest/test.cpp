/*
 * test.cpp
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#include "yaml-cpp/yaml.h"
#include "test.h"

namespace Test {

const std::string fileExtension = "test";

const int VERBOSITY_FLOW = 1;
const int VERBOSITY_DATA = 2;
const int VERBOSITY_EXTRA = 3;

void TestSpecification::append(const TestSpecification& other) {
	for(auto result: other.results) {
		this->results.insert(result);
	}
	for(auto verifiedResult: other.verifiedResults) {
		this->verifiedResults.insert(verifiedResult);
	}
	appendSpecific(other);
}

std::pair<std::string,TestResult*> TestSpecification::getLastResult(string iteration) {
	
	// Loop over all the results, starting with the last
	auto it = getResults().rbegin();
	for(;it!=getResults().rend(); ++it) {
		
		// Check if this result contains a result of the specified iteration
		std::map<string,::Test::TestResult*>::iterator it2 = it->second.find(iteration);
		if(it2 != it->second.end()) {
			
			// If it does, check if it's a valid result and return it if so
			if(it2->second->isValid()) {
				return std::pair<std::string,TestResult*>(it->first,it2->second);
			}
		}
	}
	return std::pair<std::string,TestResult*>("",NULL);
}

bool TestSuite::isInLimitTests(TestSpecification* test) {
	for(auto it=limitTests.begin();it!=limitTests.end(); ++it) {
		if(*it == test) {
			return true;
		}
	}
	return false;
}

TestSpecification* TestSuite::readYAMLNodeSpecific(const YAML::Node& node) {
	TestSpecification* test = new TestSpecification();
	return test;
}

void TestSuite::writeYAMLNodeSpecific(TestSpecification* test, YAML::Emitter& node) {
}

TestSpecification* TestSuite::readYAMLNode(const YAML::Node& node) {
	
	// Determine what format the YAML content is in
	int format = 0;
	if(const YAML::Node* generalNode = node.FindValue("general")) {
		format = 1;
		if(const YAML::Node* itemNode = generalNode->FindValue("format")) {
			try {
				*itemNode >> format;
			} catch(YAML::Exception& e) {
				// could not determine error
			}
		}
	}
	
	std::stringstream v;
	v << "Format: " << format;
	messageFormatter->reportAction3(v.str(),VERBOSITY_EXTRA);
	
	TestSpecification* test = readYAMLNodeSpecific(node);
	if(!test) return NULL;
	
	switch(format) {
		case 0:
			readYAMLNodeV0(test,node);
			break;
		case 1:
			readYAMLNodeV1(test,node);
			break;
		default:
			// error
			break;
	}
	
	return test;
}

void TestSuite::writeYAMLNode(TestSpecification* test, YAML::Emitter& out) {
	out << YAML::BeginMap;
	writeYAMLNodeV1(test,out);
	writeYAMLNodeSpecific(test,out);
	out << YAML::EndMap;
}

void TestSuite::readYAMLNodeV1(TestSpecification* test, const YAML::Node& node) {
	if(!test) return;
	if(const YAML::Node* generalNode = node.FindValue("general")) {
		if(const YAML::Node* itemNode = generalNode->FindValue("fullname")) {
			std::string fullname;
			try { *itemNode >> fullname; }
			catch(YAML::Exception& e) { reportYAMLException(e); }
			test->setFullname(fullname);
		}
		if(const YAML::Node* itemNode = generalNode->FindValue("uuid")) {
			std::string sdesc;
			try { *itemNode >> sdesc; }
			catch(YAML::Exception& e) { reportYAMLException(e); }
			test->setUUID(sdesc);
			test->setHadUUIDOnLoad(true);
		} else {
			test->setHadUUIDOnLoad(false);
		}
		if(const YAML::Node* itemNode = generalNode->FindValue("longdesc")) {
			std::string ldesc;
			try { *itemNode >> ldesc; }
			catch(YAML::Exception& e) { reportYAMLException(e); }
			test->setLongDescription(ldesc);
		}
	}
	if(const YAML::Node* itemNode = node.FindValue("results")) {
		if(itemNode->Type()==YAML::NodeType::Sequence) {
			
			// Iterate over the testruns
			for(YAML::Iterator itR = itemNode->begin(); itR!=itemNode->end(); ++itR) {
				
				// 
				for(YAML::Iterator it = itR->begin(); it!=itR->end(); ++it) {
					string resultTime;
					try {
						it.first() >> resultTime;
						//cerr << "Found result: " << resultTime << it.second().Type() << endl;
						std::map<std::string,TestResult*>& results = test->getResults()[resultTime];
						if(it.second().Type()==YAML::NodeType::Map) {
							
							// Iterate over individual results of a testrun
							for(YAML::Iterator it2 = it.second().begin(); it2!=it.second().end(); ++it2) {
								string iteration;
								TestResult* iterationResults = newTestResult();
								try {
									it2.first() >> iteration;
									iterationResults->readYAMLNode(it2.second());
									results.insert(pair<std::string,TestResult*>(iteration,iterationResults));
								} catch(YAML::Exception& e) {
									reportYAMLException(e);
								}
							}
						}
					} catch(YAML::Exception& e) {
						reportYAMLException(e);
					}
				}
			}
		}
	}
	if(const YAML::Node* itemNode = node.FindValue("verified")) {
		if(itemNode->Type()==YAML::NodeType::Map) {
			for(YAML::Iterator it = itemNode->begin(); it!=itemNode->end(); ++it) {
				string motive;
				TestResult* iterationResults = newTestResult();
				try {
					it.first() >> motive;
					iterationResults->readYAMLNode(it.second());
					test->getVerifiedResults().insert(pair<std::string,TestResult*>(motive,iterationResults));
				} catch(YAML::Exception& e) {
					reportYAMLException(e);
				}
			}
		}
	}
}

void TestSuite::writeYAMLNodeV1(TestSpecification* test, YAML::Emitter& out) {
	out << YAML::Key   << "general";
	out << YAML::Value << YAML::BeginMap;
		out << YAML::Key   << "fullname";
		out << YAML::Value << test->getFullname();
		out << YAML::Key   << "uuid";
		out << YAML::Value << test->getUUID();
		out << YAML::Key   << "longdesc";
		out << YAML::Value << test->getLongDescription();
		out << YAML::Key   << "format";
		out << YAML::Value << 1;
	out << YAML::EndMap;
	out << YAML::Key   << "results";
	out << YAML::Value << YAML::BeginSeq;
	for(auto itResult=test->getResults().begin(); itResult!=test->getResults().end(); ++itResult) {
		out << YAML::BeginMap;
		out << YAML::Key   << itResult->first;
		//out << YAML::Value << itResult->second;
		out << YAML::Value;
		out << YAML::BeginMap;
		for(auto itIteration=itResult->second.begin(); itIteration!=itResult->second.end(); ++itIteration) {
			out << YAML::Key   << itIteration->first;
			out << YAML::Value;
			itIteration->second->writeYAMLNode(out);
		}
		out << YAML::EndMap;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
	out << YAML::Key   << "verified";
		out << YAML::Value;
		out << YAML::BeginMap;
		for(auto itIteration=test->getVerifiedResults().begin(); itIteration!=test->getVerifiedResults().end(); ++itIteration) {
			out << YAML::Key   << itIteration->first;
			out << YAML::Value;
			itIteration->second->writeYAMLNode(out);
		}
		out << YAML::EndMap;
}

void TestSuite::readYAMLNodeV0(TestSpecification* test, const YAML::Node& node) {
	if(!test) return;
	if(const YAML::Node* itemNode = node.FindValue("fullname")) {
		std::string fullname;
		try { *itemNode >> fullname; }
		catch(YAML::Exception& e) { reportYAMLException(e); }
		test->setFullname(fullname);
	}
	if(const YAML::Node* itemNode = node.FindValue("uuid")) {
		std::string sdesc;
		try { *itemNode >> sdesc; }
		catch(YAML::Exception& e) { reportYAMLException(e); }
		test->setUUID(sdesc);
		test->setHadUUIDOnLoad(true);
	} else {
		test->setHadUUIDOnLoad(false);
	}
	if(const YAML::Node* itemNode = node.FindValue("longdesc")) {
		std::string ldesc;
		try { *itemNode >> ldesc; }
		catch(YAML::Exception& e) { reportYAMLException(e); }
		test->setLongDescription(ldesc);
	}
}

void TestSuite::writeYAMLNodeV0(TestSpecification* test, YAML::Emitter& out) {
	out << YAML::Key   << "fullname";
	out << YAML::Value << test->getFullname();
	out << YAML::Key   << "uuid";
	out << YAML::Value << test->getUUID();
	out << YAML::Key   << "longdesc";
	out << YAML::Value << test->getLongDescription();
}

void TestSuite::reportYAMLException(YAML::Exception& e) {
	if(messageFormatter) {
		messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),e.mark.line),e.msg);
	}
}

void TestSuite::writeTestFile(File file) {
	YAML::Emitter out;
	
	out << YAML::BeginSeq;
	for(TestSpecification* test: tests) {
		writeYAMLNode(test,out);
	}
	out << YAML::EndSeq;
	
	File outFile = file;
	while(true) {
		if(FileSystem::canCreateOrModify(outFile)) {
			break;
		} else {
			messageFormatter->reportAction("Test suite file is not writable, please enter new filename");
			messageFormatter->getConsoleWriter() << " > Append to suite file [ " << ConsoleWriter::Color::Cyan << file.getFileName() << ConsoleWriter::Color::Reset << " ]: ";
			char input[PATH_MAX+1];
			std::cin.getline(input,PATH_MAX);
			std::string inputStr = std::string(input);
			if(inputStr.empty()) inputStr = file.getFileName();
			outFile = File(inputStr);
		}
	}
	std::ofstream resultFile(file.getFileRealPath());
	if(resultFile.is_open()) {
		resultFile << string(out.c_str());
	} else {
		messageFormatter->reportError("Could not write to `" + outFile.getFileRealPath() + "'");
	}
	resultFile.close();
	
}

void TestSuite::readTestFile(File file) {
	tests.clear();
	origin = file;
	if(FileSystem::exists(file)) {
		readAndAppendTestFile(origin);
	} else {
		if(messageFormatter) messageFormatter->reportWarningAt(Location(file.getFileRealPath()),"file does not exist");
	}
	testWritability();
}

void TestSuite::readAndAppendTestFile(File file) {
	
	vector<TestSpecification*> loadedTests;
	
	if(!FileSystem::exists(file)) {
		if(messageFormatter) messageFormatter->reportErrorAt(Location(file.getFileRealPath()),"file does not exist");
		return;
	}
	messageFormatter->reportAction("Loading test suite file `" + file.getFileRealPath() + "'",VERBOSITY_FLOW);
	std::ifstream fin(file.getFileRealPath());
	if(fin.is_open()) {
		fin.seekg(0);
		try {
			YAML::Parser parser(fin);
			loadTests(parser,loadedTests);
		} catch(YAML::Exception e) {
			reportYAMLException(e);
		}
	} else {
		messageFormatter->reportError("could open suite file for reading");
	}
	
	// Merge current list of tests with the list of tests we just loaded
	mergeTestLists(tests,loadedTests);
}

void TestSuite::readAndAppendToTestFile(File file) {
	
	vector<TestSpecification*> loadedTests;
	
	if(!FileSystem::exists(file)) {
		if(messageFormatter) messageFormatter->reportErrorAt(Location(file.getFileRealPath()),"file does not exist");
		return;
	}
	messageFormatter->reportAction("Loading test suite file `" + file.getFileRealPath() + "'",VERBOSITY_FLOW);
	std::ifstream fin(file.getFileRealPath());
	if(fin.is_open()) {
		fin.seekg(0);
		try {
			YAML::Parser parser(fin);
			loadTests(parser,loadedTests);
		} catch(YAML::Exception e) {
			reportYAMLException(e);
		}
		// Merge current list of tests with the list of tests we just loaded
		mergeTestLists(loadedTests,tests);
		tests = loadedTests;
	} else {
		messageFormatter->reportError("could open suite file for reading");
	}
	
}

void TestSuite::mergeTestLists(vector<TestSpecification*>& main, vector<TestSpecification*>& tba) {
	for(TestSpecification* tbaTest: tba) {
		
		// check if exists already, based on uuid
		TestSpecification* old = NULL;
		for(TestSpecification* test: main) {
			if(*tbaTest == *test) {
				old = test;
				break;
			}
		}
		
		// If it's new, add it
		if(!old) {
			main.push_back(tbaTest);
			messageFormatter->reportAction2("Added test: " + tbaTest->getFullname(),VERBOSITY_DATA);
		
		// If it already exists, merge (overwrite)
		} else {
			old->append(*tbaTest);
			messageFormatter->reportAction2("Appended test: " + old->getFullname(),VERBOSITY_DATA);
			delete tbaTest;
		}
		
	}
	messageFormatter->reportAction("Test suite file loaded",VERBOSITY_FLOW);
	
}

void TestSuite::loadTests(YAML::Parser& parser, vector<TestSpecification*>& tests) {
	YAML::Node doc;
	while(parser.GetNextDocument(doc)) {
		if(doc.Type()==YAML::NodeType::Sequence) {
			for(YAML::Iterator it = doc.begin(); it!=doc.end(); ++it) {
					messageFormatter->reportAction2("Loading test...",VERBOSITY_DATA);
				string key;
				string value;
				if(it->Type()==YAML::NodeType::Map) {
					TestSpecification* t = readYAMLNode(*it);
					if(t) {
						messageFormatter->reportAction3("Loaded test: " + t->getFullname(),VERBOSITY_EXTRA);
						tests.push_back(t);
						t->setParentSuite(this);
					}
					
				}
			}
		} else if(doc.Type()==YAML::NodeType::Map) {
			TestSpecification* t = readYAMLNode(doc);
			if(t) {
				tests.push_back(t);
				t->setParentSuite(this);
			}
		}
	}
}

void TestSuite::createTestFile(File file) {
	{
		messageFormatter->reportAction("No test suite file specified");
		messageFormatter->getConsoleWriter() << " > Append to suite file [ " << ConsoleWriter::Color::Cyan << file.getFileName() << ConsoleWriter::Color::Reset << " ]: ";
		char input[PATH_MAX+1];
		std::cin.getline(input,PATH_MAX);
		std::string inputStr = std::string(input);
		if(!inputStr.empty()) file = File(inputStr);
	}
	if(FileSystem::exists(file)) {
		readTestFile(file);
	} else {
		tests.clear();
		origin = file;
		testWritability();
	}
}

void TestSuite::testWritability() {
	File outFile = origin;
	bool changed = false;
	while(true) {
		if(FileSystem::canCreateOrModify(outFile)) {
			break;
		} else {
			messageFormatter->reportAction("Test suite file is not writable, please enter new filename");
			messageFormatter->getConsoleWriter() << " > Append to suite file [ " << ConsoleWriter::Color::Cyan << origin.getFileName() << ConsoleWriter::Color::Reset << " ]: ";
			char input[PATH_MAX+1];
			std::cin.getline(input,PATH_MAX);
			std::string inputStr = std::string(input);
			if(inputStr.empty()) inputStr = origin.getFileName();
			outFile = File(inputStr);
			bool changed = true;
		}
	}
	if(changed) {
		File oldOrigin = origin;
		origin = outFile;
		originChanged(oldOrigin);
		
		if(FileSystem::exists(origin)) {
			readAndAppendToTestFile(origin);
		}
		updateOrigin();
	}
}

TestResult* TestSuite::newTestResult() {
	return new TestResult();
}

const YAML::Node& TestResult::readYAMLNode(const YAML::Node& node) {
	readYAMLNodeSpecific(node);
	if(const YAML::Node* itemNode = node.FindValue("stats")) {
		*itemNode >> stats;
	}
	return node;
}

YAML::Emitter& TestResult::writeYAMLNode(YAML::Emitter& out) const {
	out << YAML::BeginMap;
	out << YAML::Key << "stats" << YAML::Value << stats;
	writeYAMLNodeSpecific(out);
	out << YAML::EndMap;
	return out;
}

void TestRun::run(TestSpecification* test) {
	
	// Running test '...'
	
	string timeStamp;
	TestResult* result;
	
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	char buffer[100];
	strftime(buffer,100,"%Y-%m-%d %H:%M:%S",timeinfo);
	string timeStampStart = string(buffer);
	
	reportTestStart(test);
	for(string iteration: iterations) {
		bool cached = false;
		std::pair<std::string,TestResult*> cachedResult = test->getLastResult(iteration);
		if(!test->getParentSuite()->getForcedRunning() && cachedResult.second && cachedResult.second->isValid()) {
			cached = true;
			result = cachedResult.second;
			timeStamp = cachedResult.first;
		} else if(test->getParentSuite()->getUseCachedOnly()) {
			cached = true;
			result = NULL;
			timeStamp = "";
		} else {
			timeStamp = timeStampStart;
			result = runSpecific(test,timeStamp,iteration);
			test->addResult(timeStamp,iteration,result);
			test->getParentSuite()->updateOrigin();
		}
		reportTestResult(test,iteration,timeStamp,result,cached);
	}
	reportTestEnd(test);
	
	// Compare results
}

void TestRun::reportTestStart(TestSpecification* test) {
	ConsoleWriter& consoleWriter = messageFormatter->getConsoleWriter();
	
	// Bail if output should not be displayed
	if(hideOutput) return;
	
	// Notification prefix
	consoleWriter << ConsoleWriter::Color::BlueBright << "::" << "  ";
	
	//
	consoleWriter << "Test " << ConsoleWriter::Color::WhiteBright << test->getFullname();
	
	consoleWriter << consoleWriter.applypostfix;
	
	successes.clear();
	failures.clear();
	
	consoleWriter << "    ";
		consoleWriter << ConsoleWriter::Color::WhiteBright;
		consoleWriter.outlineLeftNext(12,' ');
		consoleWriter << ConsoleWriter::_push << "Iteration" << ConsoleWriter::_pop;
		consoleWriter << ConsoleWriter::Color::Reset << " ¦ ";
	for(const TestResultColumn& column: reportColumns) {
		consoleWriter << ConsoleWriter::Color::WhiteBright;
		consoleWriter.outlineLeftNext(12,' ');
		consoleWriter << ConsoleWriter::_push << column.name << ConsoleWriter::_pop;
		consoleWriter << ConsoleWriter::Color::Reset << " ¦ ";
	}
		consoleWriter.appendPostfix();
	
	for(const std::pair<std::string,TestResult*>& vtest: test->getVerifiedResults()) {
		displayResult(test,"",vtest.first,vtest.second,false,Test::VERIFIEDOK);
	}
}

void TestRun::reportTestEnd(TestSpecification* test) {
	ConsoleWriter& consoleWriter = messageFormatter->getConsoleWriter();
	
	// Bail if output should not be displayed
	if(hideOutput) return;
	
	
	consoleWriter << " ";// << ConsoleWriter::Color::WhiteBright << ">" << ConsoleWriter::Color::Reset << "  ";
	consoleWriter << "Test ";
	if(failures.empty()) {
		consoleWriter << ConsoleWriter::Color::GreenBright << "OK";
	} else if(successes.empty()) {
		consoleWriter << ConsoleWriter::Color::RedBright << "FAILED";
	} else {
		bool hadFirst = false;
		consoleWriter << ConsoleWriter::Color::Yellow << "MIXED";
		consoleWriter << ConsoleWriter::Color::Reset << " [";
		for(string s: successes) {
			if(hadFirst) consoleWriter << ConsoleWriter::Color::Reset << ",";
			consoleWriter << ConsoleWriter::Color::GreenBright << s;
			hadFirst = true;
		}
		for(string s: failures) {
			if(hadFirst) consoleWriter << ConsoleWriter::Color::Reset << ",";
			consoleWriter << ConsoleWriter::Color::RedBright << s;
			hadFirst = true;
		}
		consoleWriter << ConsoleWriter::Color::Reset << "]";
	}
	consoleWriter << ConsoleWriter::Color::Reset;
	consoleWriter.appendPostfix();
}

void TestRun::reportTestResult(TestSpecification* test, const std::string& iteration, const string& timeStamp, TestResult* testResult, bool cached) {
	Test::ResultStatus resultStatus = Test::UNKNOWN;
	if(testResult) {
		resultStatus = test->verify(testResult);
		if(testResult->isValid() && resultStatus==Test::OK) {
			successes.push_back(iteration);
		} else if(testResult->isValid() && resultStatus==Test::FAILED) {
			failures.push_back(iteration);
		}
	}
	displayResult(test,timeStamp,iteration,testResult,cached,resultStatus);
}

void TestRun::run(TestSuite& suite) {
	
	// Run all the tests in the suite
	for(TestSpecification* test: suite.getTests()) {
		
		// If there's a limit on the tests, check if this test is in the list
		bool ok = true;
		if(!suite.getLimitTests().empty()) {
			ok = false;
			for(TestSpecification* ltest: suite.getLimitTests()) {
				if(test == ltest) {
					ok = true;
				}
			}
		}
		
		// If green light, run the test
		if(ok) {
			run(test);
			suite.updateOrigin();
		}
		
		// If during the running of the test there was a request to stop the
		// suite, do so
		if(requestStopSuite) {
			requestStopSuite = false;
			break;
		}
	}
}

void TestRun::fillDisplayMapBase(TestSpecification* test, string timeStamp, string iteration, TestResult* result, bool cached, std::map<std::string,std::string>& content, std::map<std::string,ConsoleWriter::Color>& colors) {
	if(result->stats.time_monraw>0) {
		stringstream ss_time;
		ss_time.precision(10);
		ss_time << result->stats.time_monraw << "s";
		content["time"] = ss_time.str();
	} else {
		content["time"] = "-";
	}
	if(result->stats.mem_virtual>0) {
		stringstream ss_mem;
		ss_mem << result->stats.mem_virtual << "kiB";
		content["memory"] = ss_mem.str();
	} else {
		content["memory"] = "-";
	}
	fillDisplayMap(test,timeStamp,iteration,result,cached,content,colors);
}

void TestRun::displayResult(TestSpecification* test, string timeStamp, string iteration, TestResult* result, bool cached, const Test::ResultStatus& resultStatus = Test::UNKNOWN) {
	std::map<std::string,std::string> content;
	std::map<std::string,ConsoleWriter::Color> colors;
	if(result) {
		fillDisplayMapBase(test,timeStamp,iteration,result,cached,content,colors);
	}
	
	ConsoleWriter& consoleWriter = messageFormatter->getConsoleWriter();
	
	// Prefix
	switch(resultStatus) {
		case OK:
			consoleWriter << " " << ConsoleWriter::Color::GreenBright << "o" << ConsoleWriter::Color::Reset << "  ";
			break;
		case FAILED:
			consoleWriter << " " << ConsoleWriter::Color::Red << "x" << ConsoleWriter::Color::Reset << "  ";
			break;
		case UNKNOWN:
			consoleWriter << " " << ConsoleWriter::Color::Yellow << "-" << ConsoleWriter::Color::Reset << "  ";
			break;
		case VERIFIEDOK:
		default:
			consoleWriter << " " << ConsoleWriter::Color::Reset << " " << ConsoleWriter::Color::Reset << "  ";
			break;
	}
	
	{
		if(resultStatus==VERIFIEDOK)
			consoleWriter << ConsoleWriter::Color::GreenBright;
		else
			consoleWriter << ConsoleWriter::Color::Reset;
		consoleWriter.outlineLeftNext(11,' ');
		consoleWriter << ConsoleWriter::_push;
		consoleWriter << iteration;
		consoleWriter << ConsoleWriter::_pop;
	}
	
	if(cached) {
		consoleWriter << ConsoleWriter::Color::Cyan;
		consoleWriter << "c";
	} else {
		consoleWriter << ConsoleWriter::Color::Reset;
		consoleWriter << " ";
	}
	consoleWriter << ConsoleWriter::Color::Reset << " ¦ ";
	
	for(const TestResultColumn& column: reportColumns) {
		if(resultStatus==VERIFIEDOK)
			consoleWriter << ConsoleWriter::Color::GreenBright;
		else {
			consoleWriter << colors[column.id];
		}
		if(column.alignRight)
			consoleWriter.outlineRightNext(12,' ');
		else
			consoleWriter.outlineLeftNext(12,' ');
		consoleWriter << ConsoleWriter::_push << content[column.id] << ConsoleWriter::_pop;
		consoleWriter << ConsoleWriter::Color::Reset << " ¦ ";
	}
	consoleWriter.appendPostfix();
}


} // Namespace: Test