/*
 * files.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include "files.h"
#include <string>

const std::string DFT::Files::Unknown             ("");
const std::string DFT::Files::BasicEvent          ("be_cold");
const std::string DFT::Files::GateAnd             ("and");
const std::string DFT::Files::GateOr              ("or");
const std::string DFT::Files::GateVoting          ("voting");

const std::string DFT::FileExtensions::DFT        ("dft");
const std::string DFT::FileExtensions::LOTOS      ("lotos");
const std::string DFT::FileExtensions::LOTOSNT    ("lnt");
const std::string DFT::FileExtensions::BCG        ("bcg");
const std::string DFT::FileExtensions::SVL        ("svl");
const std::string DFT::FileExtensions::EXP        ("exp");

