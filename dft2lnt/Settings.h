#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <unordered_map>

class SettingsValue: public std::string {
public:
	SettingsValue() {
	}
	SettingsValue(const std::string& str): std::string(str) {
	}
	SettingsValue& operator=(const std::string& str) {
		*this = SettingsValue(str);
		return *this;
	}
	
	inline bool isOn() const {
		return (*this)=="1" || (*this)=="true" || (*this)=="T" || (*this)=="on";
	}
	inline bool isOff() const {
		return (*this)=="0" || (*this)=="false" || (*this)=="F" || (*this)=="off";
	}
	
	operator bool() {
		return isOn();
	}
};

class Settings: public std::unordered_map<std::string,SettingsValue> {
public:
	enum class Switch {
		UNDEFINED = 0,
		ON = 1,
		OFF = 2,
		NEITHER = 3,
		NUMBER_OF
	};
public:
	Switch asSwitch(const std::string& setting) {
		auto it = find(setting);
		if(it != end()) {
			const SettingsValue& value = it->second;
			if(value.isOn()) {
				return Switch::ON;
			} else if(value.isOff()) {
				return Switch::OFF;
			} else {
				return Switch::NEITHER;
			}
		} else {
			return Switch::UNDEFINED;
		}
	}
};

#endif