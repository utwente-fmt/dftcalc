#include "DFTCalculationResult.h"

static std::string round(std::string value, size_t digits, bool roundUp)
{
	/* Add a character if the decimal point is in the rounded-off
	 * part, to preserve the number of actual digits. */
	size_t dot = value.find('.');
	if (dot != std::string::npos && dot <= digits)
		digits++;
	if (digits >= value.length())
		return value;
	std::string ret = value.substr(0, digits);
	if (roundUp) {
		int carry = 1;
		size_t pos = ret.length() - 1;
		while (carry) {
			if (pos == 0) {
				ret = "1" + ret;
				carry = 0;
			} else if (ret[pos] == '.') {
				pos--;
			} else if (ret[pos] == '9') {
				ret[pos] = '0';
			} else {
				ret[pos]++;
				carry = 0;
			}
		}
	}
	return ret;
}

std::string DFT::DFTCalculationResultItem::valStr(size_t deltaDigits) const {
	if (exactString != "")
		return exactString;
	if (!exactBounds) {
		decnumber<> midVal = (lowerBound + upperBound) * 0.5;
		std::string middle = midVal.str();
		std::string exp = "";
		double margin = (double)(upperBound - lowerBound);
		if (middle.find('e') != std::string::npos) {
			exp = middle.substr(middle.find('e'));
			middle = middle.substr(0, middle.find('e'));
		}
		size_t wantedDigits = -std::log10(margin / (double)midVal);
		if (exp == "") {
			size_t i = 0;
			while (middle[i] == '0' || middle[i] == '.') {
				if (middle[i] == '0')
					wantedDigits++;
				i++;
			}
		}
		size_t dot = middle.find('.');
		if (dot < wantedDigits)
			wantedDigits++;
		middle = middle.substr(0, wantedDigits);
		return middle + exp + " (" + lowerBound.str() + " - " + upperBound.str() + ")";
	}
	if (lowerBound == upperBound)
		return lowerBound.str();
	std::string lower = lowerBound.str(), upper = upperBound.str();
	/* TODO: Implement proper rounding for negative numbers (i.e.,
	 * increase the LSD of the negative lower bound and leave the
	 * upper bound alone if it is also negative.
	 * Not a priority since nothing DFTCalc calculates currently
	 * goes negative.
	 */
	if (lower[0] == '-' || upper[0] == '-')
		deltaDigits = SIZE_MAX;
	std::string ret = "";
	std::string lexp = "", uexp = "";
	if (lower.find('e') != std::string::npos) {
		lexp = lower.substr(lower.find('e'));
		lower = lower.substr(0, lower.find('e'));
	}
	if (upper.find('e') != std::string::npos) {
		uexp = upper.substr(upper.find('e'));
		upper = upper.substr(0, upper.find('e'));
	}

	/* If the value is given in scientific notation but does not
	 * contain a decimal point, add one so that rounding works
	 * properly.
	 */
	if (lower.find('.') == std::string::npos && lexp != "") {
		long e = std::stol(lexp.substr(1, lexp.length() - 1));
		e += lower.length() - 1;
		lower += '.';
		for (size_t i = lower.length() - 2; i > 0; i--) {
			lower[i + 1] = lower[i];
			lower[i] = '.';
		}
		if (e)
			lexp = "e" + std::to_string(e);
	}
	if (upper.find('.') == std::string::npos && uexp != "") {
		long e = std::stol(uexp.substr(1, uexp.length() - 1));
		e += upper.length() - 1;
		upper += '.';
		for (size_t i = upper.length() - 2; i > 0; i--) {
			upper[i + 1] = upper[i];
			upper[i] = '.';
		}
		if (e)
			uexp = "e" + std::to_string(e);
	}

	/* Different exponents: Just don't merge.
	 * (We could add a special case allowing ranges like [9.9, 10.0]
	 * or [0.99, 1]e-7)
	 */
	if (lexp != uexp) {
		lower = round(lower, deltaDigits, 0);
		upper = round(upper, deltaDigits, 1);
		return '[' + lower + lexp + "; " + upper + uexp + ']';
	}
	while (!lower.empty() && lower[0] == upper[0]) {
		ret += lower[0];
		lower = lower.substr(1);
		upper = upper.substr(1);
	}
	lower = round(lower, deltaDigits, 0);
	upper = round(upper, deltaDigits, 1);
	if (!lower.empty() || !upper.empty()) {
		ret += '[';
		ret += lower;
		ret += "; ";
		ret += upper;
		ret += ']';
	}
	ret += lexp;
	return ret;
}

const YAML::Node& operator>>(const YAML::Node& node, DFT::DFTCalculationResult& result) {
	if(const YAML::Node itemNode = node["failProbs"]) {
		std::vector<DFT::DFTCalculationResultItem> failProbs;
		itemNode >> failProbs;
		result.failProbs = failProbs;
	}
	if(const YAML::Node itemNode = node["stats"]) {
		Shell::RunStatistics stats;
		itemNode >> stats;
		result.stats = stats;
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::DFTCalculationResult& result) {
	out << YAML::BeginMap;
	out << YAML::Key << "failProbs"  << YAML::Value << result.failProbs;
	// for dfttest, when we have one item, show it under the key that dfttest expects
	// dfttest currently only has support for the "failProb" key, not for "failProbs"
	for(auto it: result.failProbs) {
		out << YAML::Key << "failProb"  << YAML::Value << it.lowerBound.str();
		break;
	}
	out << YAML::Key << "stats"  << YAML::Value << result.stats;
	out << YAML::EndMap;
	return out;
}

const YAML::Node& operator>>(const YAML::Node& node, map<std::string,DFT::DFTCalculationResult>& resultMap) {
	for(YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
		std::string dft;
		DFT::DFTCalculationResult result;
		dft = it->first.as<std::string>();
		it->second >> result;
		resultMap.insert(std::pair<std::string,DFT::DFTCalculationResult>(dft,result));
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const map<std::string,DFT::DFTCalculationResult>& resultMap) {
	out << YAML::BeginMap;
	for(auto it: resultMap) {
		out << YAML::Key   << it.first;
		out << YAML::Value << it.second;
	}
	out << YAML::EndMap;
	return out;
}

const YAML::Node& operator>>(const YAML::Node& node, DFT::DFTCalculationResultItem& result) {
	if(const YAML::Node itemNode = node["missionTime"]) {
		result.missionTime = itemNode.as<std::string>();
	}
	if(const YAML::Node itemNode = node["mrmcCommand"]) {
		result.mrmcCommand = itemNode.as<std::string>();
	}
	if(const YAML::Node itemNode = node["lowerBound"]) {
		result.lowerBound = decnumber<>(itemNode.as<std::string>());
	}
	if(const YAML::Node itemNode = node["upperBound"]) {
		result.upperBound = decnumber<>(itemNode.as<std::string>());
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::DFTCalculationResultItem& result) {
	out << YAML::BeginMap;
	out << YAML::Key << "missionTime"  << YAML::Value << result.missionTime;
	out << YAML::Key << "mrmcCommand"  << YAML::Value << result.mrmcCommand;
	out << YAML::Key << "lowerBound"  << YAML::Value << result.lowerBound.str();
	out << YAML::Key << "upperBound"  << YAML::Value << result.upperBound.str();
	out << YAML::EndMap;
	return out;
}

const YAML::Node& operator>>(const YAML::Node& node, vector<DFT::DFTCalculationResultItem>& resultVector) {
	for(YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
		DFT::DFTCalculationResultItem result;
		*it >> result;
		resultVector.push_back(result);
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const vector<DFT::DFTCalculationResultItem>& resultVector) {
	out << YAML::BeginSeq;
	for(auto it: resultVector) {
		out << it;
	}
	out << YAML::EndSeq;
	return out;
}
