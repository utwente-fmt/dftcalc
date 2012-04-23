/*
 * test.cpp
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#include "yaml-cpp/yaml.h"
#include "test.h"
#include "TestOutput.h"

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
			
			return std::pair<std::string,TestResult*>(it->first,it2->second);
		}
	}
	return std::pair<std::string,TestResult*>("",NULL);
}

std::pair<std::string,TestResult*> TestSpecification::getLastValidResult(string iteration) {
	
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
		out << YAML::Key   << itResult->first; // time the test was executed
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
	if(const YAML::Node* itemNode = node.FindValue("verified")) {
		if(itemNode->Type()==YAML::NodeType::Map) {
			for(YAML::Iterator it = itemNode->begin(); it!=itemNode->end(); ++it) {
				string motive;
				TestResult* iterationResults = newTestResult();
				try {
					it.first() >> motive;
					iterationResults->readYAMLNodeSpecific(it.second());
					test->getVerifiedResults().insert(pair<std::string,TestResult*>(motive,iterationResults));
				} catch(YAML::Exception& e) {
					reportYAMLException(e);
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
									const YAML::Node& node = it2.second();
									if(const YAML::Node* itemNode = node.FindValue("time_monraw")) {
										*itemNode >> iterationResults->stats.time_monraw;
									}
									if(const YAML::Node* itemNode = node.FindValue("time_user")) {
										*itemNode >> iterationResults->stats.time_user;
									}
									if(const YAML::Node* itemNode = node.FindValue("time_system")) {
										*itemNode >> iterationResults->stats.time_system;
									}
									if(const YAML::Node* itemNode = node.FindValue("time_elapsed")) {
										*itemNode >> iterationResults->stats.time_elapsed;
									}
									if(const YAML::Node* itemNode = node.FindValue("mem_virtual")) {
										*itemNode >> iterationResults->stats.mem_virtual;
									}
									if(const YAML::Node* itemNode = node.FindValue("mem_resident")) {
										*itemNode >> iterationResults->stats.mem_resident;
									}
									iterationResults->readYAMLNodeSpecific(it2.second());
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
}

void TestSuite::writeYAMLNodeV0(TestSpecification* test, YAML::Emitter& out) {
	out << YAML::Key   << "fullname";
	out << YAML::Value << test->getFullname();
	out << YAML::Key   << "uuid";
	out << YAML::Value << test->getUUID();
	out << YAML::Key   << "longdesc";
	out << YAML::Value << test->getLongDescription();
	out << YAML::Key   << "verified";
	out << YAML::Value << test->getVerifiedResults();
	out << YAML::Key   << "results";
	out << YAML::Value << YAML::BeginSeq;
	for(auto itResult=test->getResults().begin(); itResult!=test->getResults().end(); ++itResult) {
		out << YAML::BeginMap;
		out << YAML::Key   << itResult->first; // time the test was executed
		out << YAML::Value;
		out << YAML::BeginMap;
		for(auto itIteration=itResult->second.begin(); itIteration!=itResult->second.end(); ++itIteration) {
			out << YAML::Key   << itIteration->first;
			out << YAML::Value;
			if(itIteration->second->stats.time_monraw>0)  out << YAML::Key << "time_monraw"  << YAML::Value << itIteration->second->stats.time_monraw;
			if(itIteration->second->stats.time_user>0)    out << YAML::Key << "time_user"    << YAML::Value << itIteration->second->stats.time_user;
			if(itIteration->second->stats.time_system>0)  out << YAML::Key << "time_system"  << YAML::Value << itIteration->second->stats.time_system;
			if(itIteration->second->stats.time_elapsed>0) out << YAML::Key << "time_elapsed" << YAML::Value << itIteration->second->stats.time_elapsed;
			if(itIteration->second->stats.mem_virtual>0)  out << YAML::Key << "mem_virtual"  << YAML::Value << itIteration->second->stats.mem_virtual;
			if(itIteration->second->stats.mem_resident>0) out << YAML::Key << "mem_resident" << YAML::Value << itIteration->second->stats.mem_resident;
			itIteration->second->writeYAMLNodeSpecific(out);
		}
		out << YAML::EndMap;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;}

void TestSuite::reportYAMLException(YAML::Exception& e) {
	if(messageFormatter) {
		messageFormatter->reportErrorAt(Location(origin.getFileRealPath(),e.mark.line),e.msg);
	}
}

void TestSuite::writeTestFile(File file) {
	if(FileSystem::canCreateOrModify(file)) {
		YAML::Emitter out;

		out << YAML::BeginSeq;
		for(TestSpecification* test: tests) {
			writeYAMLNode(test,out);
		}
		out << YAML::EndSeq;
		errno = 0;
		std::ofstream resultFile(file.getFileRealPath());
		if(resultFile.is_open()) {
			resultFile << string(out.c_str());
			resultFile.flush();
		} else {
			messageFormatter->reportError("Could not write to `" + file.getFileRealPath() + "': " + string(strerror(errno)));
		}
	} else {
		testWritability();
	}
	
//	File outFile = file;
//	while(true) {
//		if(FileSystem::canCreateOrModify(outFile)) {
//			break;
//		} else {
//			messageFormatter->reportAction("Test suite file is not writable, please enter new filename");
//			messageFormatter->getConsoleWriter() << " > Append to suite file [ " << ConsoleWriter::Color::Cyan << file.getFileName() << ConsoleWriter::Color::Reset << " ]: ";
//			char input[PATH_MAX+1];
//			std::cin.getline(input,PATH_MAX);
//			std::string inputStr = std::string(input);
//			if(inputStr.empty()) inputStr = file.getFileName();
//			outFile = File(inputStr);
//		}
//	}
	
}

bool TestSuite::readTestFile(File file) {
	bool error = false;
	tests.clear();
	origin = file;
	if(FileSystem::exists(file)) {
		error = readAndAppendTestFile(origin) ? true : error ;
	} else {
		error = true;
		if(messageFormatter) messageFormatter->reportWarningAt(Location(file.getFileRealPath()),"file does not exist");
	}
	testWritability();
	return error;
}

bool TestSuite::readAndAppendTestFile(File file) {
	bool error = false;
	
	vector<TestSpecification*> loadedTests;
	
	if(!FileSystem::exists(file)) {
		if(messageFormatter) messageFormatter->reportErrorAt(Location(file.getFileRealPath()),"file does not exist");
		return true;
	}
	messageFormatter->reportAction("Loading test suite file `" + file.getFileRealPath() + "'",VERBOSITY_FLOW);
	std::ifstream fin(file.getFileRealPath());
	if(fin.is_open()) {
		fin.seekg(0);
		try {
			YAML::Parser parser(fin);
			error = loadTests(parser,loadedTests) ? true : error ;
		} catch(YAML::Exception e) {
			error = true;
			reportYAMLException(e);
		}
	} else {
		error = true;
		messageFormatter->reportError("could not open suite file for reading");
	}
	
	// Merge current list of tests with the list of tests we just loaded
	error = mergeTestLists(tests,loadedTests) ? true : error ;
	return error;
}

bool TestSuite::readAndAppendToTestFile(File file) {
	bool error = false;
	
	vector<TestSpecification*> loadedTests;
	
	if(!FileSystem::exists(file)) {
		if(messageFormatter) messageFormatter->reportErrorAt(Location(file.getFileRealPath()),"file does not exist");
		return true;
	}
	messageFormatter->reportAction("Loading test suite file `" + file.getFileRealPath() + "'",VERBOSITY_FLOW);
	std::ifstream fin(file.getFileRealPath());
	if(fin.is_open()) {
		fin.seekg(0);
		try {
			YAML::Parser parser(fin);
			error = loadTests(parser,loadedTests) ? true : error ;
		} catch(YAML::Exception e) {
			error = true;
			reportYAMLException(e);
		}
		// Merge current list of tests with the list of tests we just loaded
		error = mergeTestLists(loadedTests,tests) ? true : error ;
		tests = loadedTests;
	} else {
		error = true;
		messageFormatter->reportError("could not open suite file for reading");
	}
	return error;
}

bool TestSuite::mergeTestLists(vector<TestSpecification*>& main, vector<TestSpecification*>& tba) {
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
	return false;
}

bool TestSuite::loadTests(YAML::Parser& parser, vector<TestSpecification*>& tests) {
	YAML::Node doc;
	bool error = false;
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
					} else {
						error = true;
						messageFormatter->reportAction3("FAILED to load test: " + t->getFullname());
					}
					
				}
			}
		} else if(doc.Type()==YAML::NodeType::Map) {
			TestSpecification* t = readYAMLNode(doc);
			if(t) {
				messageFormatter->reportAction3("Loaded test: " + t->getFullname(),VERBOSITY_EXTRA);
				tests.push_back(t);
				t->setParentSuite(this);
			} else {
				error = true;
				messageFormatter->reportAction3("FAILED to load test: " + t->getFullname());
			}
		}
	}
	return error;
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
	
	// Loop until the user enters a filename we can create or modify
	while(true) {
		if(FileSystem::canCreateOrModify(outFile)) {
			break;
		} else {
			messageFormatter->reportAction("Test suite file ( `" + outFile.getFileRealPath() + "' ) is not writable, please enter new filename");
			messageFormatter->getConsoleWriter() << " > Append to suite file [ " << ConsoleWriter::Color::Cyan << origin.getFileName() << ConsoleWriter::Color::Reset << " ]: ";
			char input[PATH_MAX+1];
			std::cin.getline(input,PATH_MAX);
			std::string inputStr = std::string(input);
			if(inputStr.empty()) inputStr = origin.getFileName();
			outFile = File(inputStr);
			bool changed = true;
		}
	}
	
	// If the filename is a new filename, notify subclass
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

} // Namespace: Test
