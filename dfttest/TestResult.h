namespace Test {
	class TestResult;
	class TestResultColumn;
}

#ifndef TESTRESULT_H
#define TESTRESULT_H

#include <string>
#include "Shell.h"
#include "yaml-cpp/yaml.h"

namespace Test {

enum ResultStatus {
	UNKNOWN = 0,
	OK = 1,
	FAILED,
	VERIFIEDOK,
	ResultStatusCount
};

class TestResult {
public:
	Shell::RunStatistics stats;

	TestResult() {
	}
	
	virtual void readYAMLNodeSpecific(const YAML::Node& node) {}
	virtual void writeYAMLNodeSpecific(YAML::Emitter& out) const {}
	
	const YAML::Node& readYAMLNode(const YAML::Node& node);
	
	YAML::Emitter& writeYAMLNode(YAML::Emitter& out) const;
	
	virtual bool isValid() { return true; }
	virtual bool isEqual(TestResult* other) { return true; }
};

class TestResultColumn {
public:
	std::string id;
	std::string name;
	bool alignRight;
	TestResultColumn():
		id(""),
		name(""),
		alignRight(false) {
	}
	TestResultColumn(const std::string id, const std::string name, bool alignRight):
		id(id),
		name(name),
		alignRight(alignRight) {
	}
};

} // Namespace: Test

#endif