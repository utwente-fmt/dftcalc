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
#include "DFTCalculationResult.h"
#include "CADP.h"

class DFTTestResult: public Test::TestResult {
public:
	double failprob;
	DFT::CADP::BCGInfo bcgInfo;

	DFTTestResult():
		failprob(-1.0f) {
	}
	
	Test::ResultStatus getResultStatus(Test::TestSpecification* testGeneric);
	
	void readYAMLNodeSpecific(const YAML::Node& node);
	void writeYAMLNodeSpecific(YAML::Emitter& out) const;
	
	virtual bool isValid() {
		return failprob>=0;
	}
	
	virtual bool isEqual(TestResult* other) {
		DFTTestResult* otherDFT = static_cast<DFTTestResult*>(other);
		return failprob == otherDFT->failprob;
	}
};

const YAML::Node& operator>>(const YAML::Node& node, DFTTestResult& result) {
	return result.readYAMLNode(node);
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFTTestResult& result) {
	return result.writeYAMLNode(out);
}

class DFTTest: public Test::TestSpecification {
protected:
	unsigned int timeUnits;
	File file;
	std::vector<std::string> evidence;
public:
	void setTimeUnits(unsigned int timeUnits) {this->timeUnits = timeUnits;}
	unsigned int getTimeUnits() const {return timeUnits;}
	void setFile(const File& file) {this->file = file;}
	const File& getFile() const {return file;}
	void setEvidence(std::vector<std::string>& evidence) {this->evidence = evidence;}
	const std::vector<std::string>& getEvidence() const {return evidence;}
	std::vector<std::string>& getEvidence() {return evidence;}

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
	
	void getLastResults(map<string,double>& results);
	
	virtual void appendSpecific(const TestSpecification& other);
	
	double getVerifiedValue() {
		std::string reason;
		return getVerifiedValue(reason);
	}
	double getVerifiedValue(std::string& reason) {
		double verified = -1;
		std::map<string,::Test::TestResult*>::iterator it = getVerifiedResults().begin();
		if(it!=getVerifiedResults().end()) {
			DFTTestResult* verifiedResult = static_cast<DFTTestResult*>(it->second);
			verified = verifiedResult->failprob;
		} else {
			map<string,double> results;
			getLastResults(results);
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
			if(!allSameAndUseful) {
				verified = -1;
			}
		}
		return verified;
	}
	
	virtual ::Test::ResultStatus verify(::Test::TestResult* result);
	virtual string getShortName() const {
		return file.getFileBase();
	}
};

class DFTTestSuite: public Test::TestSuite {
public:

	DFTTestSuite(MessageFormatter* messageFormatter = NULL):
		TestSuite(messageFormatter) {
	}
	
	void applyLimitTests(const vector<string>& limitTests);
	
	virtual Test::TestSpecification* readYAMLNodeSpecific(const YAML::Node& node);
	
	virtual void writeYAMLNodeSpecific(Test::TestSpecification* testGeneric, YAML::Emitter& out);
	
	virtual void originChanged(const File& from);
	
	void setMessageFormatter(MessageFormatter* messageFormatter);
	
	virtual Test::TestResult* newTestResult() {
		return new DFTTestResult();
	}
};

class DFTTestRun: public Test::TestRun {
private:
	string dft2lntRoot;
	string coralRoot;
	std::map<std::string,double> results;
	string compareBase;
public:
	DFTTestRun(Test::OutputFormatter* outputFormatter, MessageFormatter* messageFormatter, string dft2lntRoot, string coralRoot):
		TestRun(outputFormatter,messageFormatter),
		compareBase("coral"),
		dft2lntRoot(dft2lntRoot),
		coralRoot(coralRoot) {
			iterations.push_back("coral");
			iterations.push_back("dftcalc");
			reportColumns.push_back(Test::TestResultColumn("failprob"   , "P(fail)"    , "" , false));
			reportColumns.push_back(Test::TestResultColumn("states"     , "States"     , "" , true ));
			reportColumns.push_back(Test::TestResultColumn("transitions", "Transitions", "" , true ));
			reportColumns.push_back(Test::TestResultColumn("speedup"    , "Speedup"    , "" , false));
	}
	
	DFTTestResult* runDftcalc(DFTTest* test);
	DFTTestResult* runCoral(DFTTest* test);
	
	int handleSignal(int signal);
	
	virtual void fillDisplayMap(Test::TestSpecification* test, string timeStamp, string iteration, Test::TestResult* result, bool cached, std::map<std::string,std::string>& content, std::map<std::string,ConsoleWriter::Color>& colors);
	
	bool checkCached(DFTTest* test, string iteration, double verified, DFTTestResult& result);
	
	virtual Test::TestResult* runSpecific(Test::TestSpecification* testGeneric, const string& timeStamp, const string& iteration);
};

#endif