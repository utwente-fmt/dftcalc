/*
 * dfttest.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#ifndef DFTTEST_H
#define DFTTEST_H

#include <sstream>
#include "test.h"

class DFTTestResult: public Test::TestResult {
public:
	double failprob;

	DFTTestResult():
		failprob(-1.0f) {
	}

	void readYAMLNodeSpecific(const YAML::Node& node);
	void writeYAMLNodeSpecific(YAML::Emitter& out) const;
};

const YAML::Node& operator>>(const YAML::Node& node, DFTTestResult& result) {
	return result.readYAMLNode(node);
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFTTestResult& result) {
	return result.writeYAMLNode(out);
}

class DFTTest: public Test::Test {
protected:
	unsigned int timeUnits;
	File file;
	std::map<std::string,double> verifiedResults;
	std::map<std::string,std::map<std::string,DFTTestResult>> results;
public:
	void setTimeUnits(unsigned int timeUnits) {this->timeUnits = timeUnits;}
	unsigned int getTimeUnits() const {return timeUnits;}
	void setFile(const File& file) {this->file = file;}
	const File& getFile() const {return file;}
	void setVerifiedResults(std::map<std::string,double> verifiedResults) {this->verifiedResults = verifiedResults;}
	std::map<std::string,double>& getVerifiedResults() {return verifiedResults;}
	const std::map<std::string,double>& getVerifiedResults() const {return verifiedResults;}
	void setResults(std::map<std::string,std::map<std::string,DFTTestResult>> results) {this->results = results;}
	std::map<std::string,std::map<std::string,DFTTestResult>>& getResults() {return results;}
	const std::map<std::string,std::map<std::string,DFTTestResult>>& getResults() const {return results;}
	
	DFTTest():
		timeUnits(1) {
	}
	DFTTest(File file):
		timeUnits(1),
		file(file) {
	}
	virtual ~DFTTest() {
	}
	
	std::string getTimeCoral() {
		std::stringstream out;
		out << "-t " << timeUnits;
		return out.str();
	}
	
	std::string getTimeDftcalc() {
		std::stringstream out;
		out << "-t " << timeUnits;
		return out.str();
	}
	
	virtual void appendSpecific(const Test& other);
};

class DFTTestSuite: public Test::TestSuite {
public:

	DFTTestSuite(MessageFormatter* messageFormatter = NULL):
		TestSuite(messageFormatter) {
	}
	
	void applyLimitTests(const vector<string>& limitTests);
	
	virtual Test::Test* readYAMLNodeSpecific(const YAML::Node& node);
	
	virtual void writeYAMLNodeSpecific(Test::Test* testGeneric, YAML::Emitter& out);
	
	virtual void originChanged(const File& from);
	
	void setMessageFormatter(MessageFormatter* messageFormatter);
	
};

class DFTTestRun: public Test::TestRun {
private:
	string dft2lntRoot;
	string coralRoot;
	std::map<std::string,double> results;
public:
	DFTTestRun(MessageFormatter* messageFormatter, string dft2lntRoot, string coralRoot):
		TestRun(messageFormatter),
		dft2lntRoot(dft2lntRoot),
		coralRoot(coralRoot) {
	}
	
	DFTTestResult runDftcalc(DFTTest* test);
	DFTTestResult runCoral(DFTTest* test);
	
	int handleSignal(int signal);
	
	void displayResult(DFTTest* test, string timeStamp, string iteration, DFTTestResult& result, double verified, bool cached);
	DFTTestResult getLastResult(DFTTest* test, string iteration);
	
	void getLastResults(DFTTest* test, map<string,double>& results);
	
	bool checkCached(DFTTest* test, string iteration, double verified, DFTTestResult& result);
	
	virtual void runSpecific(Test::Test* testGeneric);
};

#endif