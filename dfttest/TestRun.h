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
	
	/**
	 * Creates a new TestRun instance. The specified formatters are used to
	 * print output to console.
	 * @param outputFormatter The output formatter to format the test results
	 * @param messageFormatter The message formatter to use to output to console
	 */
	TestRun(Test::OutputFormatter* outputFormatter, MessageFormatter* messageFormatter):
		requestStopTest(false),
		requestStopSuite(false),
		outputFormatter(outputFormatter),
		messageFormatter(messageFormatter) {
			//reportColumns.push_back(TestResultColumn("iteration", "Iteration", false));
			reportColumns.push_back(Test::TestResultColumn("time"     , "Time"     , "s"   , true ));
			reportColumns.push_back(Test::TestResultColumn("memory"   , "Memory"   , "MiB" , true ));
	}
	
	/**
	 * Sets whether to hide the output or not.
	 * @param true/false: whether to hide the output or not.
	 */
	void setHideOutput(bool hideOutput) {this->hideOutput = hideOutput;}
	
	/**
	 * Returns whether to hide the output or not.
	 * @return true/false: whether to hide the output or not.
	 */
	bool getHideOutput() const {return hideOutput;}
	
	/**
	 * Execute the specified test and return the result.
	 * @param test The test specification to run.
	 * @param timeStamp The time the test is started.
	 * @param iteration The iteration to execute.
	 * @return The result of this test execution.
	 */
	virtual Test::TestResult* runSpecific(Test::TestSpecification* test, const std::string& timeStamp, const std::string& iteration) = 0;
	
	/**
	 * Run this test on the specified test.
	 * @param test The test to run.
	 */
	void run(Test::TestSpecification* test);
	
	/**
	 * Report the specified test has been started. Uses the current
	 * outputFormatter to display the start of the test.
	 * @param test The test in question.
	 */
	void reportTestStart(Test::TestSpecification* test);
	
	/**
	 * Report the specified test has been started. Uses the current
	 * outputFormatter to display the start of the test.
	 * @param test The test in question.
	 */
	void reportTestEnd(Test::TestSpecification* test);
	
	/**
	 * Report the result of the test of the iteration at the timestamp.
	 * @param test The test in question.
	 * @param iteration The iteration the result is from.
	 * @param timeStamp The time the test was started.
	 * @param testResult The result of the iteration.
	 * @param cached Whether this is a cached result or not.
	 */
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
	
	/**
	 * Fills the display maps CONTENT and COLORS with values from the specified
	 * TestResult, timestamp and iteration.
	 * Calls fillDisplayMap() with the same arguments.
	 * @param test The test specification that was executed
	 * @param timeStamp the start of the test
	 * @param iteration The iteration that was executed
	 * @param result The TestResult of which the values are filled into the maps
	 * @param cached Whether this is a cached result or a new result
	 * @param content The content display map to fill
	 * @param color The color display map to fill
	 */
	void fillDisplayMapBase(Test::TestSpecification* test, string timeStamp, string iteration, Test::TestResult* result, bool cached, std::map<std::string,std::string>& content, std::map<std::string,ConsoleWriter::Color>& colors);
	
	/**
	 * Fills the display maps CONTENT and COLORS with values from the front-end.
	 * Gets called by fillDisplayMapBase() with the same arguments.
	 * @param test The test specification that was executed
	 * @param timeStamp the start of the test
	 * @param iteration The iteration that was executed
	 * @param result The TestResult of which the values are filled into the maps
	 * @param cached Whether this is a cached result or a new result
	 * @param content The content display map to fill
	 * @param color The color display map to fill
	 */
	virtual void fillDisplayMap(Test::TestSpecification* test, string timeStamp, string iteration, Test::TestResult* result, bool cached, std::map<std::string,std::string>& content, std::map<std::string,ConsoleWriter::Color>& colors) {}
};

} // Namespace: Test

#endif