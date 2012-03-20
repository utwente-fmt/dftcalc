namespace Test {
	class TestRun;
}

#ifndef TESTRUN_H
#define TESTRUN_H

#include <vector>
#include <string>
#include "MessageFormatter.h"
#include "test.h"

namespace Test {

class TestRun {
public:
	static const int VERBOSITY_EXECUTIONS = 1;
public:
	Test::OutputFormatter* outputFormatter;
	MessageFormatter* messageFormatter;
	bool hideOutput;
	bool requestStopTest;
	bool requestStopSuite;
	std::vector<std::string> iterations;
	std::vector<Test::TestResultColumn> reportColumns;
	std::vector<std::string> successes;
	std::vector<std::string> failures;
public:
	
	TestRun(Test::OutputFormatter* outputFormatter, MessageFormatter* messageFormatter):
		requestStopTest(false),
		requestStopSuite(false),
		outputFormatter(outputFormatter),
		messageFormatter(messageFormatter) {
			//reportColumns.push_back(TestResultColumn("iteration", "Iteration", false));
			reportColumns.push_back(Test::TestResultColumn("time"     , "Time"     , true ));
			reportColumns.push_back(Test::TestResultColumn("memory"   , "Memory"   , true ));
	}
	
	
	void setHideOutput(bool hideOutput) {this->hideOutput = hideOutput;}
	bool getHideOutput() const {return hideOutput;}
	
	virtual Test::TestResult* runSpecific(Test::TestSpecification* test, const std::string& timeStamp, const std::string& iteration) = 0;
	
	/**
	 * Run this test on the specified test.
	 * @param test The test to run.
	 */
	void run(Test::TestSpecification* test);
	
	void reportTestStart(Test::TestSpecification* test);
	void reportTestEnd(Test::TestSpecification* test);
	void reportTestResult(Test::TestSpecification* test, const std::string& iteration, const std::string& timeStamp, Test::TestResult* testResult, bool cached);
	
	/**
	 * Run this test on all the tests in the specified test suite.
	 * If the test suite has a limiting list of tests, only those tests will be
	 * performed.
	 * @param suite The tests in this suite will be run.
	 */
	void run(Test::TestSuite& suite);
	
	int betweenIterations() {
		return requestStopTest;
	}
	
	void fillDisplayMapBase(Test::TestSpecification* test, string timeStamp, string iteration, Test::TestResult* result, bool cached, std::map<std::string,std::string>& content, std::map<std::string,ConsoleWriter::Color>& colors);
	virtual void fillDisplayMap(Test::TestSpecification* test, string timeStamp, string iteration, Test::TestResult* result, bool cached, std::map<std::string,std::string>& content, std::map<std::string,ConsoleWriter::Color>& colors) {}
};

} // Namespace: Test

#endif