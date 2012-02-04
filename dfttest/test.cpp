#include "test.h"

namespace Test {

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
		catch(YAML::Exception& e) { if(messageFormatter) messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),e.mark.line),e.msg); }
		test->setFullname(fullname);
	}
	if(const YAML::Node* itemNode = node.FindValue("shortdesc")) {
		std::string sdesc;
		try { *itemNode >> sdesc; }
		catch(YAML::Exception& e) { if(messageFormatter) messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),e.mark.line),e.msg); }
		test->setShortDescription(sdesc);
	}
	if(const YAML::Node* itemNode = node.FindValue("longdesc")) {
		std::string ldesc;
		try { *itemNode >> ldesc; }
		catch(YAML::Exception& e) { if(messageFormatter) messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),e.mark.line),e.msg); }
		test->setLongDescription(ldesc);
	}
	return test;
}

void TestSuite::writeYAMLNode(Test* test, YAML::Emitter& out) {
	out << YAML::BeginMap;
	out << YAML::Key   << "fullname";
	out << YAML::Value << test->getFullname();
	out << YAML::Key   << "shortdesc";
	out << YAML::Value << test->getShortDescription();
	out << YAML::Key   << "longdesc";
	out << YAML::Value << test->getLongDescription();
	writeYAMLNodeSpecific(test,out);
	out << YAML::EndMap;
}

void TestSuite::writeTestFile(File file) {
	YAML::Emitter out;
	
	out << YAML::BeginSeq;
	for(Test* test: tests) {
		writeYAMLNode(test,out);
	}
	out << YAML::EndSeq;
	
	std::ofstream resultFile(file.getFileRealPath());
	if(resultFile.is_open()) {
		resultFile << string(out.c_str());
	} else {
		bool done = false;
		while(!done) {
			messageFormatter->reportError("Test suite file is not writable, please enter new filename");
			messageFormatter->getConsoleWriter() << " > New filename [ " << ConsoleWriter::Color::Cyan << file.getFileName() << ConsoleWriter::Color::Reset << " ]: ";
			char input[PATH_MAX+1];
			std::cin.getline(input,PATH_MAX);
			std::string inputStr = std::string(input);
			if(inputStr.empty()) inputStr = file.getFileName();
			std::ofstream resultFileInput(inputStr);
			if(resultFileInput.is_open()) {
				resultFileInput << string(out.c_str());
				done = true;
			}
		}
	}
}

void TestSuite::readTestFile(File file) {
	
	tests.clear();
	origin = file;
	
	if(!FileSystem::exists(file)) {
		if(messageFormatter) messageFormatter->reportErrorAt(Location(file.getFileRealPath()),"file does not exist");
		return;
	}
	std::ifstream fin(file.getFileRealPath());
	YAML::Parser parser(fin);
	YAML::Node doc;
	
	while(parser.GetNextDocument(doc)) {
	
		if(doc.Type()==YAML::NodeType::Sequence) {
			for(YAML::Iterator it = doc.begin(); it!=doc.end(); ++it) {
				string key;
				string value;
				if(it->Type()==YAML::NodeType::Map) {
					Test* t = readYAMLNode(*it);
					if(t) {
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
	
	{
		std::ofstream resultFile(file.getFileRealPath());
		if(!resultFile.is_open()) {
			bool done = false;
			while(!done) {
				messageFormatter->reportAction("Test suite file is not writable, please enter new filename");
				messageFormatter->getConsoleWriter() << " > New filename [ " << ConsoleWriter::Color::Cyan << file.getFileName() << ConsoleWriter::Color::Reset << " ]: ";
				char input[PATH_MAX+1];
				std::cin.getline(input,PATH_MAX);
				std::string inputStr = std::string(input);
				if(inputStr.empty()) inputStr = file.getFileName();
				std::ofstream resultFileInput(inputStr);
				if(resultFileInput.is_open()) {
					File oldOrigin = origin;
					origin = File(inputStr);
					originChanged(oldOrigin);
					done = true;
				}
			}
		}
		
		
		
		updateOrigin();
	}
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

void TestRun::reportTestEnd(Test* test, bool ok) {
	ConsoleWriter& consoleWriter = messageFormatter->getConsoleWriter();
	
	// Bail if output should not be displayed
	if(hideOutput) return;
	
	
	consoleWriter << " " << ConsoleWriter::Color::WhiteBright << ">" << ConsoleWriter::Color::Reset << "  ";
	consoleWriter << "Test ";
	if(ok) {
		consoleWriter << ConsoleWriter::Color::GreenBright << "OK";
	} else {
		consoleWriter << ConsoleWriter::Color::RedBright << "FAILED";
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
	}
}

} // Namespace: Test