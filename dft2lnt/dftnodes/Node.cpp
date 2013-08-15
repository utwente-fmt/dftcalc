/*
 * Node.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 * @modified by Dennis Guck
 */

#include "Node.h"
#include <string>

namespace DFT {
namespace Nodes {
	
	const std::string Node::BasicEventStr("be");
	const std::string Node::GateAndStr("and");
	const std::string Node::GateOrStr("or");
	const std::string Node::GateWSPStr("wsp");
	const std::string Node::GatePAndStr("pand");
	const std::string Node::GateVotingStr("voting");
	const std::string Node::GateFDEPStr("fdep");
	const std::string Node::UnknownStr("XXX");
	const std::string Node::RepairUnitStr("ru");
	const std::string Node::RepairUnitFcfsStr("ru_f");

}
}
