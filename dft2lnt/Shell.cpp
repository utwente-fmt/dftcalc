/*
 * Shell.cpp
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#include "Shell.h"

MessageFormatter* Shell::messageFormatter = NULL;

const YAML::Node& operator>>(const YAML::Node& node, Shell::RunStatistics& stats) {
	if(const YAML::Node* itemNode = node.FindValue("time_monraw")) {
		*itemNode >> stats.time_monraw;
	}
	if(const YAML::Node* itemNode = node.FindValue("time_user")) {
		*itemNode >> stats.time_user;
	}
	if(const YAML::Node* itemNode = node.FindValue("time_system")) {
		*itemNode >> stats.time_system;
	}
	if(const YAML::Node* itemNode = node.FindValue("time_elapsed")) {
		*itemNode >> stats.time_elapsed;
	}
	if(const YAML::Node* itemNode = node.FindValue("mem_virtual")) {
		*itemNode >> stats.mem_virtual;
	}
	if(const YAML::Node* itemNode = node.FindValue("mem_resident")) {
		*itemNode >> stats.mem_resident;
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const Shell::RunStatistics& stats) {
	out << YAML::BeginMap;
	if(stats.time_monraw>0)  out << YAML::Key << "time_monraw"  << YAML::Value << stats.time_monraw;
	if(stats.time_user>0)    out << YAML::Key << "time_user"    << YAML::Value << stats.time_user;
	if(stats.time_system>0)  out << YAML::Key << "time_system"  << YAML::Value << stats.time_system;
	if(stats.time_elapsed>0) out << YAML::Key << "time_elapsed" << YAML::Value << stats.time_elapsed;
	if(stats.mem_virtual>0)  out << YAML::Key << "mem_virtual"  << YAML::Value << stats.mem_virtual;
	if(stats.mem_resident>0) out << YAML::Key << "mem_resident" << YAML::Value << stats.mem_resident;
	out << YAML::EndMap;
	return out;
}