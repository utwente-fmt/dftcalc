#include "TestRun.h"

void Test::TestRun::run(TestSpecification* test) {
	
	// Running test '...'
	
	string timeStamp;
	TestResult* result;
	
	/* Get the current time for the timestamp of the test */
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	char buffer[100];
	strftime(buffer,100,"%Y-%m-%d %H:%M:%S",timeinfo);
	string timeStampStart = string(buffer);
	
	/* Report that the test is started */
	reportTestStart(test);
	
	/* Run every iteration for this test */
	for(string iteration: iterations) {
		bool cached = false;
		
		/* Check if there is already a proper result for this test for this
		 * iteration. Also check if the user only wants to see cached result
		 * or wants to force rerunning cached results.
		 */
		// Get last result
		std::pair<std::string,TestResult*> cachedResult = test->getLastValidResult(iteration);
		
		// If the user did not specify forced rerunning and the cached result
		// is valid, use the cached result
		if(!test->getParentSuite()->getForcedRunning() && cachedResult.second && cachedResult.second->isValid()) {
			cached = true;
			result = cachedResult.second;
			timeStamp = cachedResult.first;
		// Otherwise (thus regardless whether the result is valid), if the user
		// specified only showing cached results, use NULL as 'cached' result
		} else if(test->getParentSuite()->getUseCachedOnly()) {
			cachedResult = test->getLastResult(iteration);
			if(cachedResult.second) {
				cached = true;
				result = cachedResult.second;
				timeStamp = cachedResult.first;
			} else {
				cached = true;
				result = NULL;
				timeStamp = "";
			}
		
		// Otherwise, run the test, add the result, and write result to origin
		} else {
			timeStamp = timeStampStart;
			result = runSpecific(test,timeStamp,iteration);
			test->addResult(timeStamp,iteration,result);
			test->getParentSuite()->updateOrigin();
		}
		
		/* Report the result for this test for this iteration */
		reportTestResult(test,iteration,timeStamp,result,cached);
	}
	
	/* Report that the test has been ended */
	reportTestEnd(test);
	
}

void Test::TestRun::reportTestStart(TestSpecification* test) {
	// Clear the past results
	successes.clear();
	failures.clear();

	// Bail if output should not be displayed
	if(hideOutput) return;
	
	// Display test start
	outputFormatter->displayTestStart(this,test);
}

void Test::TestRun::reportTestEnd(TestSpecification* test) {
	// Bail if output should not be displayed
	if(hideOutput) return;
	
	// Display test end
	outputFormatter->displayTestEnd(this,test);
}

void Test::TestRun::reportTestResult(TestSpecification* test, const std::string& iteration, const string& timeStamp, TestResult* testResult, bool cached) {
	Test::ResultStatus resultStatus = Test::UNKNOWN;
	
	// Determine whether the result was a success or failure
	if(testResult) {
		resultStatus = test->verify(testResult);
		if(testResult->isValid() && resultStatus==Test::OK) {
			successes.push_back(iteration);
		} else if(testResult->isValid() && resultStatus==Test::FAILED) {
			failures.push_back(iteration);
		}
	}
	
	// Display test result
	outputFormatter->displayTestResult(this,test,timeStamp,iteration,testResult,cached,resultStatus);
}

void Test::TestRun::run(TestSuite& suite) {
	
	// Run all the tests in the suite
	outputFormatter->displaySuiteStart(this);
	for(TestSpecification* test: suite.getTests()) {
		
		// If there's a limit on the tests, check if this test is in the list
		bool ok = true;
		if(!suite.getLimitTests().empty()) {
			ok = false;
			for(TestSpecification* ltest: suite.getLimitTests()) {
				if(test == ltest) {
					ok = true;
				}
			}
		}
		
		// If green light, run the test
		if(ok) {
			run(test);
			suite.updateOrigin();
		}
		
		// If during the running of the test there was a request to stop the
		// suite, do so
		if(requestStopSuite) {
			requestStopSuite = false;
			break;
		}
	}
	outputFormatter->displaySuiteEnd(this);
}

void Test::TestRun::fillDisplayMapBase(TestSpecification* test, string timeStamp, string iteration, TestResult* result, bool cached, std::map<std::string,std::string>& content, std::map<std::string,ConsoleWriter::Color>& colors) {
	
	// Fill "time" with starts.time_monraw
	if(result->stats.time_monraw>0) {
//		stringstream ss_time;
//		ss_time.precision(10);
//		ss_time << result->stats.time_monraw << "s";
//		content["time"] = ss_time.str();
		char time[20];
		snprintf(time,20,"%.3f",result->stats.time_monraw);
		content["time"] = string(time);
	} else {
		content["time"] = "-";
	}
	
	// Fill "memory" with starts.mem_virtual
	if(result->stats.mem_virtual>0) {
//		stringstream ss_mem;
//		ss_mem << result->stats.mem_virtual/1024.0f;
//		content["memory"] = ss_mem.str();
		char mem[20];
		snprintf(mem,20,"%.3f",result->stats.mem_virtual/1024.0f);
		content["memory"] = string(mem);
	} else {
		content["memory"] = "-";
	}
	
	// Fill front-end specific things (subclasses of TestRun)
	fillDisplayMap(test,timeStamp,iteration,result,cached,content,colors);
}

