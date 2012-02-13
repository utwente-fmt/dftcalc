/*
 * test.h
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

namespace Test {
	class Test;
	class TestSuite;
}

#ifndef TEST_H
#define TEST_H

#include <fstream>
#include <iostream>
#include <time.h>

#include "yaml-cpp/yaml.h"
//#include "pcrecpp.h"

#include "FileSystem.h"
#include "Shell.h"
#include "MessageFormatter.h"
#include "mrmc.h"

using namespace std;

namespace Test {

extern const std::string fileExtension;
extern const int VERBOSITY_FLOW;
extern const int VERBOSITY_DATA;

class Test {
protected:
	std::string fullname;
	std::string uuid;
	bool hadUUIDOnLoad;
	std::string longDescription;
	TestSuite* suite;
public:
	Test():
		fullname(""),
		uuid(""),
		hadUUIDOnLoad(false),
		longDescription (""),
		suite (NULL) {
		System::generateUUID(32,uuid);
	}
	
	Test(std::string fullname):
		fullname(fullname),
		uuid(""),
		hadUUIDOnLoad(false),
		longDescription (""),
		suite (NULL) {
		System::generateUUID(32,uuid);
	}
	
	virtual ~Test() {
	}
	
	void setFullname(const std::string& fullname) {this->fullname = fullname;}
	const std::string& getFullname() const {return fullname;}
	void setLongDescription(const std::string& longDescription) {this->longDescription = longDescription;}
	void setUUID(const std::string& uuid) {this->uuid = uuid;}
	const std::string& getUUID() const {return uuid;}
	void setHadUUIDOnLoad(bool hadUUIDOnLoad) {this->hadUUIDOnLoad = hadUUIDOnLoad;}
	bool getHadUUIDOnLoad() const {return hadUUIDOnLoad;}
	const std::string& getLongDescription() const {return longDescription;}
	
	void setParentSuite(TestSuite* suite) { this->suite = suite; }
	TestSuite* getParentSuite() { return suite; }
	
	bool operator==(const Test& other) {
		if((!hadUUIDOnLoad || !other.hadUUIDOnLoad) && fullname != "" && fullname == other.fullname) {
			return true;
		}
		return uuid == other.uuid;
	}
	
	void append(const Test& other);
	virtual void appendSpecific(const Test& other) {};
};

class TestSuite {
protected:
	File origin;
	MessageFormatter* messageFormatter;
	vector<Test*> tests;
	bool forcedRunning;
	bool useCachedOnly;
	vector<Test*> limitTests;
	void loadTests(YAML::Parser& parser, vector<Test*>& tests);
	void mergeTestLists(vector<Test*>& main, vector<Test*>& tba);
public:
	
	TestSuite(MessageFormatter* messageFormatter = NULL):
		messageFormatter(messageFormatter),
		forcedRunning(false),
		useCachedOnly(false) {
	}
	
	virtual ~TestSuite() {
		for(size_t i=tests.size();i--;) {
			assert(tests[i] && "TesSuite should not have a NULL test...");
			delete tests[i];
		}
	}
	
	void setForcedRunning(bool forcedRunning) {this->forcedRunning = forcedRunning;}
	void setUseCachedOnly(bool useCachedOnly) {this->useCachedOnly = useCachedOnly;}
	bool getForcedRunning() const {return forcedRunning;}
	bool getUseCachedOnly() const {return useCachedOnly;}
	
	void setLimitTests(const vector<Test*>& limitTests) {this->limitTests = limitTests;}
	const vector<Test*>& getLimitTests() const {return limitTests;}
	
	bool isInLimitTests(Test* test);
	
	virtual Test* readYAMLNodeSpecific(const YAML::Node& node);
	
	virtual void writeYAMLNodeSpecific(Test* test, YAML::Emitter& node);
	
	Test* readYAMLNode(const YAML::Node& node);
	
	void writeYAMLNode(Test* test, YAML::Emitter& out);
	
	void reportYAMLException(YAML::Exception& e);
	
	void writeTestFile(File file);
	
	void readTestFile(File file);
	void readAndAppendTestFile(File file);
	void readAndAppendToTestFile(File file);
	void createTestFile(File file);
	void testWritability();
	
	void updateOrigin() {
		writeTestFile(getOrigin());
	}
	
	const File& getOrigin() const {return origin;}
	vector<Test*>& getTests() { return tests; }
	size_t getTestCount() { return tests.size(); }
	
	virtual void originChanged(const File& from) = 0;
};

class TestResult {
public:
	float time_user;
	float time_system;
	float time_elapsed;
	float time_monraw;
	float mem_virtual;
	float mem_resident;

	TestResult():
		time_user(0.0f),
		time_system(0.0f),
		time_elapsed(0.0f),
		mem_virtual(0.0f),
		mem_resident(0.0f) {
	}
	
	virtual void readYAMLNodeSpecific(const YAML::Node& node) {}
	virtual void writeYAMLNodeSpecific(YAML::Emitter& out) const {}
	
	const YAML::Node& readYAMLNode(const YAML::Node& node);
	
	YAML::Emitter& writeYAMLNode(YAML::Emitter& out) const;
};

class TestRun {
public:
	static const int VERBOSITY_EXECUTIONS = 1;
protected:
	
	MessageFormatter* messageFormatter;
	bool hideOutput;
	bool requestStopTest;
	bool requestStopSuite;
public:
	
	TestRun(MessageFormatter* messageFormatter):
		requestStopTest(false),
		requestStopSuite(false),
		messageFormatter(messageFormatter) {
	}
	
	
	void setHideOutput(bool hideOutput) {this->hideOutput = hideOutput;}
	bool getHideOutput() const {return hideOutput;}
	
	virtual void runSpecific(Test* test) = 0;
	
	void run(Test* test);
	
	void reportTestStart(Test* test, string name, string verifiedDesc, ConsoleWriter::Color& verifiedColor, string verifiedResult);
	
	void reportTestEnd(Test* test, vector<string>& successes, vector<string>& failures);
	
	void reportTest(Test* test,
	                const ConsoleWriter::Color& itemColor, char item,
	                const ConsoleWriter::Color& iterationColor, string iteration,
	                const ConsoleWriter::Color& resultColor, string result,
	                const ConsoleWriter::Color& timeColor, string time, bool cached);
	
	void reportTestSuccess  (Test* test,string iteration, string result, string time, bool cached);
	void reportTestCached   (Test* test,string iteration, string result, string time, bool cached);
	void reportTestFailure  (Test* test,string iteration, string result, string time, bool cached);
	void reportTestUndecided(Test* test,string iteration, string result, string time, bool cached);
	
	void run(TestSuite& suite);
	int betweenIterations() {
		return requestStopTest;
	}
	
};

} // Namespace: Test

#endif