namespace Test {
	class OutputFormatter;
	class OutputFormatterNice;
}

#ifndef TESTOUTPUT_H
#define TESTOUTPUT_H

#include <string>
#include "TestResult.h"
#include "TestRun.h"

namespace Test {

enum OutputMode {
	OutputNone = 0,
	OutputNice = 1,
	OutputLaTeX,
	OutputCSV,
	OutputModeCount
};


class OutputFormatter {
public:
	virtual void displaySuiteStart(TestRun* testRun) = 0;
	virtual void displaySuiteEnd(TestRun* testRun) = 0;
	virtual void displayTestStart(TestRun* testRun, TestSpecification* test) = 0;
	virtual void displayTestEnd(TestRun* testRun, TestSpecification* test) = 0;
	virtual void displayTestResult(TestRun* testRun, TestSpecification* test, const string& timeStamp, const string& iteration, TestResult* result, bool cached, const ResultStatus& resultStatus) = 0;
};

class OutputFormatterNice: public OutputFormatter {
	virtual void displaySuiteStart(TestRun* testRun);
	virtual void displaySuiteEnd(TestRun* testRun);
	virtual void displayTestStart(TestRun* testRun, TestSpecification* test);
	virtual void displayTestEnd(TestRun* testRun, TestSpecification* test);
	virtual void displayTestResult(TestRun* testRun, TestSpecification* test, const string& timeStamp, const string& iteration, TestResult* result, bool cached, const ResultStatus& resultStatus);
};

class OutputFormatterLaTeX: public OutputFormatter {
	virtual void displaySuiteStart(TestRun* testRun);
	virtual void displaySuiteEnd(TestRun* testRun);
	virtual void displayTestStart(TestRun* testRun, TestSpecification* test);
	virtual void displayTestEnd(TestRun* testRun, TestSpecification* test);
	virtual void displayTestResult(TestRun* testRun, TestSpecification* test, const string& timeStamp, const string& iteration, TestResult* result, bool cached, const ResultStatus& resultStatus);
	std::string escapeUnderscore(const std::string& str) {
		std::string n;
		n.reserve(str.length()+1);
		for(char c: str) {
			if(c=='_') n += '\\';
			n += c;
		}
		return n;
	}
};

} // Namespace: Test

#endif