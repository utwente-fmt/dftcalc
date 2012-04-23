#include "TestOutput.h"

void Test::OutputFormatterNice::displaySuiteStart(TestRun* testRun) {
}

void Test::OutputFormatterNice::displaySuiteEnd(TestRun* testRun) {
}

void Test::OutputFormatterNice::displayTestStart(TestRun* testRun, TestSpecification* test) {
	ConsoleWriter& consoleWriter = testRun->messageFormatter->getConsoleWriter();
	
	// Notification prefix
	consoleWriter << ConsoleWriter::Color::BlueBright << "::" << "  ";
	
	//
	consoleWriter << "Test " << ConsoleWriter::Color::WhiteBright << test->getFullname();
	
	consoleWriter << consoleWriter.applypostfix;
	
	consoleWriter << "    ";
		consoleWriter << ConsoleWriter::Color::WhiteBright;
		consoleWriter.outlineLeftNext(12,' ');
		consoleWriter << ConsoleWriter::_push << "Iteration" << ConsoleWriter::_pop;
		consoleWriter << ConsoleWriter::Color::Reset << " ¦ ";
	for(const TestResultColumn& column: testRun->reportColumns) {
		consoleWriter << ConsoleWriter::Color::WhiteBright;
		consoleWriter.outlineLeftNext(12,' ');
		consoleWriter << ConsoleWriter::_push << column.name;
		if(!column.unit.empty()) {
			consoleWriter << " (" << column.unit << ")";
		}
		consoleWriter << ConsoleWriter::_pop;
		consoleWriter << ConsoleWriter::Color::Reset << " ¦ ";
	}
		consoleWriter.appendPostfix();
	
	for(const std::pair<std::string,TestResult*>& vtest: test->getVerifiedResults()) {
		displayTestResult(testRun,test,"",vtest.first,vtest.second,false,Test::VERIFIEDOK);
	}
}
void Test::OutputFormatterNice::displayTestEnd(TestRun* testRun, TestSpecification* test) {
	ConsoleWriter& consoleWriter = testRun->messageFormatter->getConsoleWriter();
	
	consoleWriter << " ";// << ConsoleWriter::Color::WhiteBright << ">" << ConsoleWriter::Color::Reset << "  ";
	consoleWriter << "Test ";
	if(testRun->failures.empty()) {
		consoleWriter << ConsoleWriter::Color::GreenBright << "OK";
	} else if(testRun->successes.empty()) {
		consoleWriter << ConsoleWriter::Color::RedBright << "FAILED";
	} else {
		bool hadFirst = false;
		consoleWriter << ConsoleWriter::Color::Yellow << "MIXED";
		consoleWriter << ConsoleWriter::Color::Reset << " [";
		for(string s: testRun->successes) {
			if(hadFirst) consoleWriter << ConsoleWriter::Color::Reset << ",";
			consoleWriter << ConsoleWriter::Color::GreenBright << s;
			hadFirst = true;
		}
		for(string s: testRun->failures) {
			if(hadFirst) consoleWriter << ConsoleWriter::Color::Reset << ",";
			consoleWriter << ConsoleWriter::Color::RedBright << s;
			hadFirst = true;
		}
		consoleWriter << ConsoleWriter::Color::Reset << "]";
	}
	consoleWriter << ConsoleWriter::Color::Reset;
	consoleWriter.appendPostfix();
}
void Test::OutputFormatterNice::displayTestResult(TestRun* testRun, TestSpecification* test, const string& timeStamp, const string& iteration, TestResult* result, bool cached, const Test::ResultStatus& resultStatus) {
	std::map<std::string,std::string> content;
	std::map<std::string,ConsoleWriter::Color> colors;
	if(result) {
		testRun->fillDisplayMapBase(test,timeStamp,iteration,result,cached,content,colors);
	}
	
	ConsoleWriter& consoleWriter = testRun->messageFormatter->getConsoleWriter();
	
	// Prefix
	switch(resultStatus) {
		case OK:
			consoleWriter << " " << ConsoleWriter::Color::GreenBright << "o" << ConsoleWriter::Color::Reset << "  ";
			break;
		case FAILED:
			consoleWriter << " " << ConsoleWriter::Color::Red << "x" << ConsoleWriter::Color::Reset << "  ";
			break;
		case UNKNOWN:
			consoleWriter << " " << ConsoleWriter::Color::Yellow << "-" << ConsoleWriter::Color::Reset << "  ";
			break;
		case VERIFIEDOK:
		default:
			consoleWriter << " " << ConsoleWriter::Color::Reset << " " << ConsoleWriter::Color::Reset << "  ";
			break;
	}
	
	{
		if(resultStatus==VERIFIEDOK)
			consoleWriter << ConsoleWriter::Color::GreenBright;
		else
			consoleWriter << ConsoleWriter::Color::Reset;
		consoleWriter.outlineLeftNext(11,' ');
		consoleWriter << ConsoleWriter::_push;
		consoleWriter << iteration;
		consoleWriter << ConsoleWriter::_pop;
	}
	
	if(cached) {
		consoleWriter << ConsoleWriter::Color::Cyan;
		consoleWriter << "c";
	} else {
		consoleWriter << ConsoleWriter::Color::Reset;
		consoleWriter << " ";
	}
	consoleWriter << ConsoleWriter::Color::Reset << " ¦ ";
	
	for(const TestResultColumn& column: testRun->reportColumns) {
		if(resultStatus==VERIFIEDOK)
			consoleWriter << ConsoleWriter::Color::GreenBright;
		else {
			consoleWriter << colors[column.id];
		}
		if(column.alignRight)
			consoleWriter.outlineRightNext(12,' ');
		else
			consoleWriter.outlineLeftNext(12,' ');
		consoleWriter << ConsoleWriter::_push << content[column.id] << ConsoleWriter::_pop;
		consoleWriter << ConsoleWriter::Color::Reset << " ¦ ";
	}
	consoleWriter.appendPostfix();
}

void Test::OutputFormatterLaTeX::displaySuiteStart(TestRun* testRun) {
	ConsoleWriter& consoleWriter = testRun->messageFormatter->getConsoleWriter();
	
		consoleWriter << "& ";
		consoleWriter.outlineLeftNext(12,' ');
		consoleWriter << ConsoleWriter::_push << "Iteration" << ConsoleWriter::_pop;
	for(const TestResultColumn& column: testRun->reportColumns) {
		consoleWriter << " & ";
		consoleWriter.outlineLeftNext(12,' ');
		consoleWriter << ConsoleWriter::_push << escapeUnderscore(column.name);
		if(!column.unit.empty()) {
			consoleWriter << " (" << escapeUnderscore(column.unit) << ")";
		}
		consoleWriter << ConsoleWriter::_pop;
	}
	consoleWriter << " \\\\" << consoleWriter.applypostfix;
	consoleWriter << "\\testbetween" << consoleWriter.applypostfix;
}

void Test::OutputFormatterLaTeX::displaySuiteEnd(TestRun* testRun) {
}

void Test::OutputFormatterLaTeX::displayTestStart(TestRun* testRun, TestSpecification* test) {
	ConsoleWriter& consoleWriter = testRun->messageFormatter->getConsoleWriter();
	
	consoleWriter << "\\testbetween" << consoleWriter.applypostfix;
	
	//
	consoleWriter << "\\testname{" << escapeUnderscore(test->getShortName()) << "}" << consoleWriter.applypostfix;
	
	for(const std::pair<std::string,TestResult*>& vtest: test->getVerifiedResults()) {
		displayTestResult(testRun,test,"",vtest.first,vtest.second,false,Test::VERIFIEDOK);
	}
}
void Test::OutputFormatterLaTeX::displayTestEnd(TestRun* testRun, TestSpecification* test) {
	ConsoleWriter& consoleWriter = testRun->messageFormatter->getConsoleWriter();
	
}

void Test::OutputFormatterLaTeX::displayTestResult(TestRun* testRun, TestSpecification* test, const string& timeStamp, const string& iteration, TestResult* result, bool cached, const Test::ResultStatus& resultStatus) {
	std::map<std::string,std::string> content;
	std::map<std::string,ConsoleWriter::Color> colors;
	if(result) {
		testRun->fillDisplayMapBase(test,timeStamp,iteration,result,cached,content,colors);
	}
	
	ConsoleWriter& consoleWriter = testRun->messageFormatter->getConsoleWriter();
	
	{
		consoleWriter << "& ";
		consoleWriter << "\\testcoliteration{" << escapeUnderscore(iteration) << "}";
	}
	
	for(const TestResultColumn& column: testRun->reportColumns) {
		consoleWriter << " & ";
		consoleWriter << "\\testcol" << escapeUnderscore(column.id) << "{" << escapeUnderscore(content[column.id]) << "}";
	}
	consoleWriter << " \\\\" << consoleWriter.applypostfix;
	consoleWriter << "\\testbetween" << consoleWriter.applypostfix;
}
