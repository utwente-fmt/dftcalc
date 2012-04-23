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

/**
 * Results from the execution of an interation of a test
 */
class TestResult {
public:
	/// The stats hold resource usage and time elapsed
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

/**
 * Describes a single test result column.
 */
class TestResultColumn {
public:
	/// A unique ID to distinguish the column (e.g. in maps)
	std::string id;
	
	/// The name of the column that should be displayed at the top
	std::string name;
	
	/// The unit of the values in this column
	std::string unit;
	
	/// Whether to align the results left or right
	bool alignRight;
	TestResultColumn():
		id(""),
		name(""),
		alignRight(false) {
	}
	TestResultColumn(const std::string id, const std::string name, const std::string unit, bool alignRight):
		id(id),
		name(name),
		unit(unit),
		alignRight(alignRight) {
	}
};

} // Namespace: Test

#endif