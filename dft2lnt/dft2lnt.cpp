/*
 * dft2lnt.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include <stdio.h>

#include "dft2lnt.h"

const std::string DFT2LNT::LNTSUBROOT ("/share/dft2lnt/lntnodes");
const std::string DFT2LNT::BCGSUBROOT ("/share/dft2lnt/bcgnodes");
const std::string DFT2LNT::TESTSUBROOT("/share/dft2lnt/tests");
