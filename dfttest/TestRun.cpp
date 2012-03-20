#include "TestRun.h"

void Test::TestRun::run(TestSpecification* test) {
	
	// Running test '...'
	
	string timeStamp;
	TestResult* result;
	
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	char buffer[100];
	strftime(buffer,100,"%Y-%m-%d %H:%M:%S",timeinfo);
	string timeStampStart = string(buffer);
	
	reportTestStart(test);
	for(string iteration: iterations) {
		bool cached = false;
		std::pair<std::string,TestResult*> cachedResult = test->getLastResult(iteration);
		if(!test->getParentSuite()->getForcedRunning() && cachedResult.second && cachedResult.second->isValid()) {
			cached = true;
			result = cachedResult.second;
			timeStamp = cachedResult.first;
		} else if(test->getParentSuite()->getUseCachedOnly()) {
			cached = true;
			result = NULL;
			timeStamp = "";
		} else {
			timeStamp = timeStampStart;
			result = runSpecific(test,timeStamp,iteration);
			test->addResult(timeStamp,iteration,result);
			test->getParentSuite()->updateOrigin();
		}
		reportTestResult(test,iteration,timeStamp,result,cached);
	}
	reportTestEnd(test);
	
	// Compare results
}

void Test::TestRun::reportTestStart(TestSpecification* test) {
		successes.clear();
		failures.clear();

	// Bail if output should not be displayed
	if(hideOutput) return;
	outputFormatter->displayTestStart(this,test);
}

void Test::TestRun::reportTestEnd(TestSpecification* test) {
	// Bail if output should not be displayed
	if(hideOutput) return;
	outputFormatter->displayTestEnd(this,test);
}

void Test::TestRun::reportTestResult(TestSpecification* test, const std::string& iteration, const string& timeStamp, TestResult* testResult, bool cached) {
	Test::ResultStatus resultStatus = Test::UNKNOWN;
	if(testResult) {
		resultStatus = test->verify(testResult);
		if(testResult->isValid() && resultStatus==Test::OK) {
			successes.push_back(iteration);
		} else if(testResult->isValid() && resultStatus==Test::FAILED) {
			failures.push_back(iteration);
		}
	}
	outputFormatter->displayTestResult(this,test,timeStamp,iteration,testResult,cached,resultStatus);
}

void Test::TestRun::run(TestSuite& suite) {
	
	// Run all the tests in the suite
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
}

void Test::TestRun::fillDisplayMapBase(TestSpecification* test, string timeStamp, string iteration, TestResult* result, bool cached, std::map<std::string,std::string>& content, std::map<std::string,ConsoleWriter::Color>& colors) {
	if(result->stats.time_monraw>0) {
		stringstream ss_time;
		ss_time.precision(10);
		ss_time << result->stats.time_monraw << "s";
		content["time"] = ss_time.str();
	} else {
		content["time"] = "-";
	}
	if(result->stats.mem_virtual>0) {
		stringstream ss_mem;
		ss_mem << result->stats.mem_virtual << "kiB";
		content["memory"] = ss_mem.str();
	} else {
		content["memory"] = "-";
	}
	fillDisplayMap(test,timeStamp,iteration,result,cached,content,colors);
}

