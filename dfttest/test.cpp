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

void Test::append(const Test& other) {
	appendSpecific(other);
}


bool TestSuite::isInLimitTests(Test* test) {
	for(auto it=limitTests.begin();it!=limitTests.end(); ++it) {
		if(*it == test) {
			return true;
		}
	}
	return false;
}

Test* TestSuite::readYAMLNodeSpecific(const YAML::Node& node) {
	Test* test = new Test();
	return test;
}

void TestSuite::writeYAMLNodeSpecific(Test* test, YAML::Emitter& node) {
}

Test* TestSuite::readYAMLNode(const YAML::Node& node) {
	Test* test = readYAMLNodeSpecific(node);
	if(!test) return NULL;
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
	return test;
}

void TestSuite::writeYAMLNode(Test* test, YAML::Emitter& out) {
	out << YAML::BeginMap;
	out << YAML::Key   << "fullname";
	out << YAML::Value << test->getFullname();
	out << YAML::Key   << "uuid";
	out << YAML::Value << test->getUUID();
	out << YAML::Key   << "longdesc";
	out << YAML::Value << test->getLongDescription();
	writeYAMLNodeSpecific(test,out);
	out << YAML::EndMap;
}

void TestSuite::reportYAMLException(YAML::Exception& e) {
	if(messageFormatter) {
		messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),e.mark.line),e.msg);
	}
}

void TestSuite::writeTestFile(File file) {
	YAML::Emitter out;
	
	out << YAML::BeginSeq;
	for(Test* test: tests) {
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
	
	vector<Test*> loadedTests;
	
	if(!FileSystem::exists(file)) {
		if(messageFormatter) messageFormatter->reportErrorAt(Location(file.getFileRealPath()),"file does not exist");
		return;
	}
	messageFormatter->reportAction("Loading test suite file `" + file.getFileRealPath() + "'",VERBOSITY_DATA);
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
	
	vector<Test*> loadedTests;
	
	if(!FileSystem::exists(file)) {
		if(messageFormatter) messageFormatter->reportErrorAt(Location(file.getFileRealPath()),"file does not exist");
		return;
	}
	messageFormatter->reportAction("Loading test suite file `" + file.getFileRealPath() + "'",VERBOSITY_DATA);
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

void TestSuite::mergeTestLists(vector<Test*>& main, vector<Test*>& tba) {
	for(Test* tbaTest: tba) {
		
		// check if exists already, based on uuid
		Test* old = NULL;
		for(Test* test: main) {
			if(*tbaTest == *test) {
				old = test;
				break;
			}
		}
		
		// If it's new, add it
		if(!old) {
			main.push_back(tbaTest);
			messageFormatter->reportAction("  Added test: " + tbaTest->getFullname(),VERBOSITY_DATA);
		
		// If it already exists, merge (overwrite)
		} else {
			old->append(*tbaTest);
			messageFormatter->reportAction("  Appended test: " + old->getFullname(),VERBOSITY_DATA);
			delete tbaTest;
		}
		
	}
	messageFormatter->reportAction("Test suite file loaded",VERBOSITY_FLOW);
	
}

void TestSuite::loadTests(YAML::Parser& parser, vector<Test*>& tests) {
	YAML::Node doc;
	messageFormatter->reportAction("  - Loading tests ...",VERBOSITY_DATA);
	while(parser.GetNextDocument(doc)) {
	messageFormatter->reportAction("  - Loading tests ...",VERBOSITY_DATA);
		if(doc.Type()==YAML::NodeType::Sequence) {
			for(YAML::Iterator it = doc.begin(); it!=doc.end(); ++it) {
					messageFormatter->reportAction("  - Loading test",VERBOSITY_DATA);
				string key;
				string value;
				if(it->Type()==YAML::NodeType::Map) {
					Test* t = readYAMLNode(*it);
					if(t) {
						messageFormatter->reportAction("    - Loaded test: " + t->getFullname(),VERBOSITY_DATA);
						tests.push_back(t);
						t->setParentSuite(this);
					}
					
				}
			}
		} else if(doc.Type()==YAML::NodeType::Map) {
			Test* t = readYAMLNode(doc);
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
		}
	}
	File oldOrigin = origin;
	origin = outFile;
	originChanged(oldOrigin);

	if(FileSystem::exists(origin)) {
		readAndAppendToTestFile(origin);
	}
	updateOrigin();
}

const YAML::Node& TestResult::readYAMLNode(const YAML::Node& node) {
	if(const YAML::Node* itemNode = node.FindValue("time_monraw")) {
		*itemNode >> time_monraw;
	}
	if(const YAML::Node* itemNode = node.FindValue("time_user")) {
		*itemNode >> time_user;
	}
	if(const YAML::Node* itemNode = node.FindValue("time_system")) {
		*itemNode >> time_system;
	}
	if(const YAML::Node* itemNode = node.FindValue("time_elapsed")) {
		*itemNode >> time_elapsed;
	}
	if(const YAML::Node* itemNode = node.FindValue("mem_virtual")) {
		*itemNode >> mem_virtual;
	}
	if(const YAML::Node* itemNode = node.FindValue("mem_resident")) {
		*itemNode >> mem_resident;
	}
	readYAMLNodeSpecific(node);
	return node;
}

YAML::Emitter& TestResult::writeYAMLNode(YAML::Emitter& out) const {
	out << YAML::BeginMap;
	if(time_monraw>0)  out << YAML::Key << "time_monraw" << YAML::Value << time_monraw;
	if(time_user>0)    out << YAML::Key << "time_user" << YAML::Value << time_user;
	if(time_system>0)  out << YAML::Key << "time_system" << YAML::Value << time_system;
	if(time_elapsed>0) out << YAML::Key << "time_elapsed" << YAML::Value << time_elapsed;
	if(mem_virtual>0)  out << YAML::Key << "mem_virtual" << YAML::Value << mem_virtual;
	if(mem_resident>0) out << YAML::Key << "mem_resident" << YAML::Value << mem_resident;
	writeYAMLNodeSpecific(out);
	out << YAML::EndMap;
	return out;
}

void TestRun::run(Test* test) {
	
	// Running test '...'
	
	runSpecific(test);
	
	// Compare results
}

void TestRun::reportTestStart(Test* test, string name, string verifiedDesc, ConsoleWriter::Color& verifiedColor, string verifiedResult) {
	ConsoleWriter& consoleWriter = messageFormatter->getConsoleWriter();
	
	// Bail if output should not be displayed
	if(hideOutput) return;
	
	// Notification prefix
	consoleWriter << ConsoleWriter::Color::BlueBright << "::" << "  ";
	
	//
	consoleWriter << "Test " << ConsoleWriter::Color::WhiteBright << name;
	
	consoleWriter << consoleWriter.applypostfix;
	
	if(!verifiedResult.empty() || !verifiedDesc.empty()) {
		reportTest(test,
				   ConsoleWriter::Color::WhiteBright,'>',
				   verifiedColor,verifiedDesc,
				   ConsoleWriter::Color::WhiteBright,verifiedResult,
				   ConsoleWriter::Color::WhiteBright,"",
				   false
		);
	}
	
}

void TestRun::reportTestEnd(Test* test, vector<string>& successes, vector<string>& failures) {
	ConsoleWriter& consoleWriter = messageFormatter->getConsoleWriter();
	
	// Bail if output should not be displayed
	if(hideOutput) return;
	
	
	consoleWriter << " " << ConsoleWriter::Color::WhiteBright << ">" << ConsoleWriter::Color::Reset << "  ";
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

void TestRun::reportTest(Test* test,
				const ConsoleWriter::Color& itemColor, char item,
				const ConsoleWriter::Color& iterationColor, string iteration,
				const ConsoleWriter::Color& resultColor, string result,
				const ConsoleWriter::Color& timeColor, string time, bool cached) {
	ConsoleWriter& consoleWriter = messageFormatter->getConsoleWriter();
	
	// Bail if output should not be displayed
	if(hideOutput) return;
	
	// Prefix
	consoleWriter << " " << itemColor << string(&item,1) << "  ";
	
	//
	consoleWriter << iterationColor;
	consoleWriter.outlineLeftNext(15,' ');
	consoleWriter << iteration;
	if(!result.empty()) {
		consoleWriter << ConsoleWriter::Color::Reset << " : ";
	}
	consoleWriter << resultColor;
	consoleWriter.outlineLeftNext(15,' ');
	consoleWriter << result;
	if(!time.empty()) {
		consoleWriter << ConsoleWriter::Color::Reset << " ( ";
		consoleWriter << timeColor;
		consoleWriter.outlineLeftNext(15,' ');
		consoleWriter << ConsoleWriter::_push << time << "s" << ConsoleWriter::_pop;
		consoleWriter << ConsoleWriter::Color::Reset << " )";
	} else {
		consoleWriter.outlineLeftNext(20,' ');
		consoleWriter << "";
	}
	if(cached) {
		consoleWriter << ConsoleWriter::Color::Reset << " ( ";
		consoleWriter << ConsoleWriter::Color::Cyan  << "cached";
		consoleWriter << ConsoleWriter::Color::Reset << " )";
	}
	
	consoleWriter << consoleWriter.applypostfix;
}

void TestRun::reportTestSuccess(Test* test,string iteration, string result, string time, bool cached) {
	reportTest(test,
			   ConsoleWriter::Color::GreenBright,'o',
			   ConsoleWriter::Color::Reset,iteration,
			   ConsoleWriter::Color::WhiteBright,result,
			   ConsoleWriter::Color::WhiteBright,time,
			   cached
	);
}

void TestRun::reportTestCached(Test* test,string iteration, string result, string time, bool cached) {
	reportTest(test,
			   ConsoleWriter::Color::Green,'c',
			   ConsoleWriter::Color::Reset,iteration,
			   ConsoleWriter::Color::WhiteBright,result,
			   ConsoleWriter::Color::WhiteBright,time,
			   cached
	);
}

void TestRun::reportTestFailure(Test* test,string iteration, string result, string time, bool cached) {
	reportTest(test,
			   ConsoleWriter::Color::Red,'x',
			   ConsoleWriter::Color::Reset,iteration,
			   ConsoleWriter::Color::WhiteBright,result,
			   ConsoleWriter::Color::WhiteBright,time,
			   cached
	);
}
void TestRun::reportTestUndecided(Test* test,string iteration, string result, string time, bool cached) {
//		ConsoleWriter& consoleWriter = messageFormatter->getConsoleWriter();
//		
//		// Success prefix
//		consoleWriter << " " << ConsoleWriter::Color::BlueBright << ">" << "  ";
//		
//		//
//		consoleWriter << ConsoleWriter::Color::Reset << iteration << ": " << ConsoleWriter::Color::WhiteBright << result;
//		
//		consoleWriter << consoleWriter.applypostfix;
	reportTest(test,
			   ConsoleWriter::Color::YellowBright,'-',
			   ConsoleWriter::Color::Reset,iteration,
			   ConsoleWriter::Color::WhiteBright,result,
			   ConsoleWriter::Color::WhiteBright,time,
			   cached
	);
}

void TestRun::run(TestSuite& suite) {
	for(Test* test: suite.getTests()) {
		bool ok = true;
		if(!suite.getLimitTests().empty()) {
			ok = false;
			for(Test* ltest: suite.getLimitTests()) {
				if(test == ltest) {
					ok = true;
				}
			}
		}
		if(ok) {
			run(test);
			suite.updateOrigin();
		}
		if(requestStopSuite) {
			requestStopSuite = false;
			break;
		}
	}
}

} // Namespace: Test