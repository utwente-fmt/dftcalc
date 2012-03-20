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
	virtual void displayTestStart(TestRun* testRun, TestSpecification* test) = 0;
	virtual void displayTestEnd(TestRun* testRun, TestSpecification* test) = 0;
	virtual void displayTestResult(TestRun* testRun, TestSpecification* test, const string& timeStamp, const string& iteration, TestResult* result, bool cached, const ResultStatus& resultStatus) = 0;
};

class OutputFormatterNice: public OutputFormatter {
	void displayTestStart(TestRun* testRun, TestSpecification* test);
	void displayTestEnd(TestRun* testRun, TestSpecification* test);
	void displayTestResult(TestRun* testRun, TestSpecification* test, const string& timeStamp, const string& iteration, TestResult* result, bool cached, const ResultStatus& resultStatus);
};

} // Namespace: Test

#endif