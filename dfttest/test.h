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

class Test {
protected:
	std::string fullname;
	std::string shortDescription;
	std::string longDescription;
	TestSuite* suite;
public:
	Test():
		shortDescription(""),
		longDescription ("") {
	}
	
	Test(File file):
		shortDescription(""),
		longDescription ("") {
	}
	
	virtual ~Test() {
	}
	
	void setFullname(const std::string& fullname) {this->fullname = fullname;}
	const std::string& getFullname() const {return fullname;}
	void setLongDescription(const std::string& longDescription) {this->longDescription = longDescription;}
	void setShortDescription(const std::string& shortDescription) {this->shortDescription = shortDescription;}
	const std::string& getShortDescription() const {return shortDescription;}
	const std::string& getLongDescription() const {return longDescription;}
	
	void setParentSuite(TestSuite* suite) { this->suite = suite; }
	TestSuite* getParentSuite() { return suite; }
};

class TestSuite {
protected:
	File origin;
	MessageFormatter* messageFormatter;
	vector<Test*> tests;
	bool forcedRunning;
	bool useCachedOnly;
	vector<Test*> limitTests;
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
	
	void writeTestFile(File file);
	
	void readTestFile(File file);
	
	void updateOrigin() {
		writeTestFile(getOrigin());
	}
	
	const File& getOrigin() const {return origin;}
	vector<Test*> getTests() { return tests; }
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
public:
	
	TestRun(MessageFormatter* messageFormatter):
		messageFormatter(messageFormatter) {
	}
	
	
	void setHideOutput(bool hideOutput) {this->hideOutput = hideOutput;}
	bool getHideOutput() const {return hideOutput;}
	
	virtual void runSpecific(Test* test) = 0;
	
	void run(Test* test);
	
	void reportTestStart(Test* test, string name, string verifiedDesc, ConsoleWriter::Color& verifiedColor, string verifiedResult);
	
	void reportTestEnd(Test* test, bool ok);
	
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
	
};

} // Namespace: Test

#endif