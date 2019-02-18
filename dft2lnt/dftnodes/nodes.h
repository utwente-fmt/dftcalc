/*
 * nodes.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg extended by Dennis Guck
 */

#ifndef NODES_H
#define NODES_H

// nodes.h includes all nodes
#include "dftnodes/Node.h"
#include "dftnodes/BasicEvent.h"
#include "dftnodes/Gate.h"
#include "dftnodes/GateAnd.h"
#include "dftnodes/GateOr.h"
#include "dftnodes/GateVoting.h"
#include "dftnodes/GateWSP.h"
#include "dftnodes/GatePAnd.h"
#include "dftnodes/GatePor.h"
#include "dftnodes/GateSAnd.h"
#include "dftnodes/GateSEQ.h"
#include "dftnodes/GateFDEP.h"
#include "dftnodes/RepairUnit.h"
#include "dftnodes/Inspection.h"
#include "dftnodes/Replacement.h"

#endif // NODES_H
