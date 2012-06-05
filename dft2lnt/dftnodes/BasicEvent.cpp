/*
 * BasicEvent.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include <dftnodes/BasicEvent.h>
#include <string>

const std::string CalculationModeUNDEFINED   = "undefined";
const std::string CalculationModeEXPONENTIAL = "exponential";
const std::string CalculationModeWEIBULL     = "weibull";
const std::string CalculationModeAPH         = "aph";
const std::string& DFT::Nodes::BE::getCalculationModeStr(CalculationMode mode) {
	switch(mode) {
		case DFT::Nodes::BE::CalculationMode::EXPONENTIAL:
			return CalculationModeEXPONENTIAL;
		case DFT::Nodes::BE::CalculationMode::WEIBULL:
			return CalculationModeWEIBULL;
		case DFT::Nodes::BE::CalculationMode::APH:
			return CalculationModeAPH;
		default:
			return CalculationModeUNDEFINED;
	}
}

//std::string DFT::Nodes::BE::Attrib::Lambda("lambda");
