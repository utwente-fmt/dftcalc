/*
 * test.h
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

namespace Test {
	class TestSpecification;
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
#include "TestResult.h"
#include "TestOutput.h"
#include "TestRun.h"

using namespace std;

namespace Test {

extern const std::string fileExtension;
extern const int VERBOSITY_FLOW;
extern const int VERBOSITY_DATA;

class TestSpecification {
protected:
	std::string fullname;
	std::string uuid;
	bool hadUUIDOnLoad;
	std::string longDescription;
	TestSuite* suite;
	std::map<std::string,std::map<std::string,TestResult*>> results;
	std::map<std::string,TestResult*> verifiedResults;
public:
	TestSpecification():
		fullname(""),
		uuid(""),
		hadUUIDOnLoad(false),
		longDescription (""),
		suite (NULL) {
		System::generateUUID(32,uuid);
	}
	
	TestSpecification(std::string fullname):
		fullname(fullname),
		uuid(""),
		hadUUIDOnLoad(false),
		longDescription (""),
		suite (NULL) {
		System::generateUUID(32,uuid);
	}
	
	virtual ~TestSpecification() {
	}
	
	void setFullname(const std::string& fullname) {this->fullname = fullname;}
	const std::string& getFullname() const {return fullname;}
	void setLongDescription(const std::string& longDescription) {this->longDescription = longDescription;}
	void setUUID(const std::string& uuid) {this->uuid = uuid;}
	const std::string& getUUID() const {return uuid;}
	void setHadUUIDOnLoad(bool hadUUIDOnLoad) {this->hadUUIDOnLoad = hadUUIDOnLoad;}
	bool getHadUUIDOnLoad() const {return hadUUIDOnLoad;}
	const std::string& getLongDescription() const {return longDescription;}
	
	void setResults(std::map<std::string,std::map<std::string,TestResult*>> results) {this->results = results;}
	std::map<std::string,std::map<std::string,TestResult*>>& getResults() {return results;}
	const std::map<std::string,std::map<std::string,TestResult*>>& getResults() const {return results;}
	
	void addResult(std::string timeStamp, std::string iteration, TestResult* result) {
		results[timeStamp][iteration] = result;
	}
	
	void setParentSuite(TestSuite* suite) { this->suite = suite; }
	TestSuite* getParentSuite() { return suite; }
	
	bool operator==(const TestSpecification& other) {
		if((!hadUUIDOnLoad || !other.hadUUIDOnLoad)) {
			return fullname != "" && fullname == other.fullname;
		}
		return uuid == other.uuid;
	}
	
	void append(const TestSpecification& other);
	virtual void appendSpecific(const TestSpecification& other) {};

	std::pair<std::string,TestResult*> getLastResult(string iteration);
	std::pair<std::string,TestResult*> getLastValidResult(string iteration);
	
	virtual ResultStatus verify(TestResult* result) {};
	
	virtual std::map<std::string,TestResult*>& getVerifiedResults() { return verifiedResults; }
	
	virtual string getShortName() const {
		return getFullname().substr(0,10);
	}
};

class TestSuite {
protected:
	File origin;
	MessageFormatter* messageFormatter;
	vector<TestSpecification*> tests;
	bool forcedRunning;
	bool useCachedOnly;
	vector<TestSpecification*> limitTests;
	bool loadTests(YAML::Parser& parser, vector<TestSpecification*>& tests);
	bool mergeTestLists(vector<TestSpecification*>& main, vector<TestSpecification*>& tba);
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
	
	void setLimitTests(const vector<TestSpecification*>& limitTests) {this->limitTests = limitTests;}
	const vector<TestSpecification*>& getLimitTests() const {return limitTests;}
	
	bool isInLimitTests(TestSpecification* test);
	
	virtual TestSpecification* readYAMLNodeSpecific(const YAML::Node& node);
	virtual void writeYAMLNodeSpecific(TestSpecification* test, YAML::Emitter& node);
	
	TestSpecification* readYAMLNode(const YAML::Node& node);
	void readYAMLNodeV0(TestSpecification* test, const YAML::Node& node);
	void readYAMLNodeV1(TestSpecification* test, const YAML::Node& node);
	
	void writeYAMLNode(TestSpecification* test, YAML::Emitter& out);
	void writeYAMLNodeV0(TestSpecification* test, YAML::Emitter& out);
	void writeYAMLNodeV1(TestSpecification* test, YAML::Emitter& out);
	
	void reportYAMLException(YAML::Exception& e);
	
	void writeTestFile(File file);
	
	bool readTestFile(File file);
	bool readAndAppendTestFile(File file);
	bool readAndAppendToTestFile(File file);
	void createTestFile(File file);
	void testWritability();
	
	void updateOrigin() {
		writeTestFile(getOrigin());
	}
	
	const File& getOrigin() const {return origin;}
	vector<TestSpecification*>& getTests() { return tests; }
	size_t getTestCount() { return tests.size(); }
	
	virtual void originChanged(const File& from) = 0;
	
	virtual TestResult* newTestResult();
};

} // Namespace: Test

#endif