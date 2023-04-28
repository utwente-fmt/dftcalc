/*
 * ASTValidator.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg and extended by Dennis Guck
 */

#ifndef ASTVALIDATOR_H
#define ASTVALIDATOR_H

#include <vector>
#include <iostream>

#include "dft_ast.h"
#include "dft_parser.h"
#include "ASTVisitor.h"
#include "ASTDFTBuilder.h"

namespace DFT {

/**
 * This class handles the validation of an AST.
 * It will validate the following concepts:
 *   - All references are to existing nodes
 */
class ASTValidator: public DFT::ASTVisitor<int,true> {
private:
	std::vector<std::string> definedNodes;
	static void f_aggregate(int& result, int value) {
		result = result && value;
	}
public:

	/**
	 * Constructs a new ASTValidator using the specified
	 * AST and CompilerContext.
	 * Call validate() to start the validation process.
	 */
	ASTValidator(DFT::AST::ASTNodes* ast, CompilerContext* cc):
		//ASTVisitor(ast,cc,[](int& ret, int val){ret = ret && val;}) {
		ASTVisitor<int,true>(ast,cc,&f_aggregate) {
	}

	/**
	 */
	 virtual void aggregate(int& result, const int& value) {
		 result = result && value;
	 }

	/**
	 * Builds a list of defined DFT nodes in the AST specified in the
	 * constructor. The definedNodes member will be overridden.
	 */
	void buildDefinedNodesList() {

		// Bail if AST not set
		if(!ast) return;
		
		// Clear the list
		definedNodes.clear();
		
		// Go through all the AST nodes in a linear fashion
		for(size_t i=0; i<ast->size(); ++i) {
			DFT::AST::ASTNode* node = ast->at(i);
			assert(node);
			switch(node->getType()) {
				
			// TopLevel references a node
			case DFT::AST::TopLevelType: {
				break;
			}

			// A BasicEvent ASTNode defines a DFT node
			case DFT::AST::BasicEventType: {
				DFT::AST::ASTBasicEvent* be = static_cast<DFT::AST::ASTBasicEvent*>(node);
				//std::cout << "Defined: " << basicEvent->getName() << std::endl;
				assert(be);
				assert(be->getName());
				definedNodes.push_back(be->getName()->getString());
				break;
			}

			// Any Gate ASTNode defines a DFT node
			// Any Gate can reference multiple nodes
			case DFT::AST::GateType: {
				DFT::AST::ASTGate* g = static_cast<DFT::AST::ASTGate*>(node);
				definedNodes.push_back(g->getName()->getString());
				break;
			}
			
			// A Page ASTNode references a node
			case DFT::AST::PageType: {
				break;
			}
			default:
				break;
			}
		}
	}
	
	/**
	 * Starts the validation process of the AST specified in the
	 * constructor. Returns whether the AST is deemed valid or not.
	 * Returns true if it is valid, false otherewise.
	 */
	int validate() {
		
		// Invalid if AST not set
		if(!ast) return false;
		
		buildDefinedNodesList();

		if (definedNodes.size() == 0) {
			std::cout << "No nodes defined (nada niente nix nenio). Empty input file?!?!?" << std::endl;
			return false;
		}
		
		int valid = true;
		
		valid = ASTVisitor<int,true>::visit() ? valid : false;
		
		return valid;
	}
	
/*	int checkReferences() {
		int valid = true;
		std::vector<std::string> unknownNodes = referencedNodes;

//		std::cout << "Referenced nodes:";
//		for(int i=0;i<unknownNodes.size();++i) {
//			std::cout << " " << unknownNodes.at(i);
//		}
//		std::cout << std::endl;
		
		for(int i=definedNodes.size();i--;) {
		//	std::cout << "== Removing: " << definedNodes.at(i) << std::endl;
			std::vector<std::string>::iterator it = std::find(unknownNodes.begin(),unknownNodes.end(),definedNodes.at(i));
			if(it != unknownNodes.end()) unknownNodes.erase(it);
		}
		for(int i=0;i<unknownNodes.size();++i) {
			parser->getCC()->reportErrorAt(unknownNodes.at(i),"undefined node: ")
			std::cout << "Undefined node: " << unknownNodes.at(i) << std::endl;
		}
		valid = unknownNodes.size()==0 ? valid : false;

		return valid;
	}
*/

	virtual int visitTopLevel(DFT::AST::ASTTopLevel* topLevel) {
		int valid = true;
		
		valid = ASTVisitor<int,true>::visitTopLevel(topLevel) ? valid : false ;
		
		std::vector<std::string>::iterator it = std::find(definedNodes.begin(),definedNodes.end(),topLevel->getTopNode()->getString());
		if(it == definedNodes.end()) {
			valid = false;
			cc->reportErrorAt(topLevel->getTopNode()->getLocation(),"undefined node referenced: " + topLevel->getTopNode()->getString());
		}
		return valid;
	}
	virtual int visitBasicEvent(DFT::AST::ASTBasicEvent* basicEvent) {
		int valid = true;
		valid = ASTVisitor<int,true>::visitBasicEvent(basicEvent) ? valid : false ;
		return valid;
	}
	virtual int visitGate(DFT::AST::ASTGate* gate) {
		int valid = true;

		if(!ASTDFTBuilderPass1::buildGateTest(gate)) {
			valid = false;
			cc->reportErrorAt(gate->getLocation(),"unsupported gate type: " + gate->getGateType()->getString());
		}

		valid = ASTVisitor<int,true>::visitGate(gate) ? valid : false ;

		std::vector<DFT::AST::ASTIdentifier*>* children = gate->getChildren();
		for(int i=children->size();i--;) {
			std::vector<std::string>::iterator it = std::find(definedNodes.begin(),definedNodes.end(),children->at(i)->getString());
			if(it == definedNodes.end()) {
				valid = false;
				cc->reportErrorAt(children->at(i)->getLocation(),"undefined node referenced: " + children->at(i)->getString());
			}
		}
		return valid;
	}
	virtual int visitPage(DFT::AST::ASTPage* page) {
		int valid = true;

		valid = ASTVisitor<int,true>::visitPage(page) ? valid : false ;

		std::vector<std::string>::iterator it = std::find(definedNodes.begin(),definedNodes.end(),page->getNodeName()->getString());
		if(it == definedNodes.end()) {
			valid = false;
			cc->reportErrorAt(page->getNodeName()->getLocation(),"undefined node referenced: " + page->getNodeName()->getString());
		}
		return valid;
	}
	virtual int visitAttrib(DFT::AST::ASTAttrib* attr) {
		int valid = true;
		valid = ASTVisitor<int,true>::visitAttrib(attr) ? valid : false;
		return valid;
	}
	
	virtual int visitAttribute(DFT::AST::ASTAttribute* attribute) {
		int valid = true;
		DFT::AST::ASTAttrib* value = attribute->getValue();
		
		switch(attribute->getLabel()) {
			case DFT::Nodes::BE::AttrLabelLambda:
				if(!value) {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"lambda label without value");
					break;
				}
				if(value->isFloat()) {
					float v = static_cast<DFT::AST::ASTAttribFloat*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative lambda");
					}
				} else if(value->isNumber()) {
					int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative lambda");
					}
				} else {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"lambda label needs float value");
				}
				break;
			case DFT::Nodes::BE::AttrLabelDorm:
				if(!value) {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"dorm label without value");
					break;
				}
				if(value->isFloat()) {
					float v = static_cast<DFT::AST::ASTAttribFloat*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative dormancy factor");
					}
				} else if(value->isNumber()) {
					int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative dormancy factor");
					}
				} else {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"dorm label needs float value");
				}
				break;
            // check for new maintain label
            case DFT::Nodes::BE::AttrLabelMaintain:
                if(!value) {
                    valid = false;
                    cc->reportErrorAt(attribute->getLocation(),"maintain label without value");
                    break;
                }
                if(value->isFloat()) {
                    float v = static_cast<DFT::AST::ASTAttribFloat*>(value)->getValue();
                    if(v < 0) {
                        valid = false;
                        cc->reportErrorAt(attribute->getLocation(),"negative maintain");
                    }
                } else if(value->isNumber()) {
                    int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
                    if(v < 0) {
                        valid = false;
                        cc->reportErrorAt(attribute->getLocation(),"negative maitntain");
                    }
                } else {
                    valid = false;
                    cc->reportErrorAt(attribute->getLocation(),"maintain label needs float value");
                }
                break;

			// check for new repair label
			case DFT::Nodes::BE::AttrLabelRepair:
				if(!value) {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"repair label without value");
					break;
				}
				if(value->isFloat()) {
					float v = static_cast<DFT::AST::ASTAttribFloat*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative repair");
					}
				} else if(value->isNumber()) {
					int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative repair");
					}
				} else {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"repair label needs float value");
				}
                break;
			// check for new priority label
			case DFT::Nodes::BE::AttrLabelPrio:
				if(!value) {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"priority label without value");
					break;
				}
				if(value->isNumber()) {
					int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative priority");
					}
				} else if(value->isFloat()) {
					int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative priority");
					}
				} else {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"priority label needs integer value");
				}
                break;
            // check for APH label
			case DFT::Nodes::BE::AttrLabelAph:
				if(!value) {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"aph label without value");
					break;
				}
				if(value->isString()) {
					DFT::AST::ASTIdentifier* id = static_cast<DFT::AST::ASTAttribString*>(value)->getValue();
					File fileToEmbed(id->getString());
                                        //std::cerr << "aph orig file='" << fileToEmbed.getFileRealPath() << "'" << std::endl;
                                        File dftFile(attribute->getLocation().getFileName());
                                        fileToEmbed.insertPathToPrefix(dftFile.getPathTo());
 
					if (!FileSystem::exists(fileToEmbed)) {
						valid = false;
						//std::cerr << "aph distr file dirname='" << fileToEmbed.getPathTo() << "'" << std::endl;
						cc->reportErrorAt(attribute->getLocation(),"aph distribution file does not exist: \""+id->getString()+"\"");
					} else {
						//std::cerr << "aph distr file dirname='" << fileToEmbed.getPathTo() << "'" << std::endl;
					}
				} else {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"aph label needs string value: name of .bcg distribution file to embed");
				}
				break;
            // check for PHASES label
            case DFT::Nodes::BE::AttrLabelPhases:
                if(!value) {
                    valid = false;
                    cc->reportErrorAt(attribute->getLocation(),"phases label without value");
                    break;
                }
                if(value->isNumber()) {
                    int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
                    if(v < 1) {
                        valid = false;
                        cc->reportErrorAt(attribute->getLocation(),"no valid phases specified");
                    }
                } else if(value->isFloat()) {
                    int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
                    if(v < 0) {
                        valid = false;
                        cc->reportErrorAt(attribute->getLocation(),"no valid phases specified");
                    }
                } else {
                    valid = false;
                    cc->reportErrorAt(attribute->getLocation(),"phases label needs integer value");
                }
                break;
			// check for INTERVAL label
            case DFT::Nodes::BE::AttrLabelInterval:
                if(!value) {
                    valid = false;
                    cc->reportErrorAt(attribute->getLocation(),"interval label without value");
                    break;
                }
                if(value->isNumber()) {
                    int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
                    if(v < 1) {
                        valid = false;
                        cc->reportErrorAt(attribute->getLocation(),"no valid interval specified");
                    }
                } else if(value->isFloat()) {
                    int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
                    if(v < 0) {
                        valid = false;
                        cc->reportErrorAt(attribute->getLocation(),"no valid interval specified");
                    }
                } else {
                    valid = false;
                    cc->reportErrorAt(attribute->getLocation(),"interval label needs integer value");
                }
                break;
			case DFT::Nodes::BE::AttrLabelProb:
				if(!value) {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"prob label without value");
					break;
				}
				if(value->isFloat()) {
					float v = static_cast<DFT::AST::ASTAttribFloat*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative prob");
					}
					if(v > 1) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"prob greater than 1");
					}
				} else if(value->isNumber()) {
					int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative prob");
					}
					if(v > 1) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"prob greater than 1");
					}
				} else {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"prob label needs float value");
				}
				break;

			case DFT::Nodes::BE::AttrLabelRes:
				if(!value) {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"res label without value");
					break;
				}
				if(value->isFloat()) {
					float v = static_cast<DFT::AST::ASTAttribFloat*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative res");
					}
					if(v > 1) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"res greater than 1");
					}
				} else if(value->isNumber()) {
					int v = static_cast<DFT::AST::ASTAttribNumber*>(value)->getValue();
					if(v < 0) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"negative res");
					}
					if(v > 1) {
						valid = false;
						cc->reportErrorAt(attribute->getLocation(),"res greater than 1");
					}
				} else {
					valid = false;
					cc->reportErrorAt(attribute->getLocation(),"res label needs float value");
				}
				break;
			case DFT::Nodes::BE::AttrLabelRate:
			case DFT::Nodes::BE::AttrLabelShape:
			case DFT::Nodes::BE::AttrLabelMean:
			case DFT::Nodes::BE::AttrLabelStddev:
			case DFT::Nodes::BE::AttrLabelCov:
			case DFT::Nodes::BE::AttrLabelRepl:

			case DFT::Nodes::BE::AttrLabelNone:
			case DFT::Nodes::BE::AttrLabelOther:
			default:
				//valid = false;
				cc->reportWarningAt(attribute->getLocation(),"unsupported label: `" + attribute->getString() + "', ignoring");
				break;
		}
		
		valid = ASTVisitor<int,true>::visitAttribute(attribute) ? valid : false;
		return valid;
	}
	virtual int visitAttribFloat(DFT::AST::ASTAttribFloat* af) {
		int valid = true;
		valid = ASTVisitor<int,true>::visitAttribFloat(af) ? valid : false;
		return valid;
	}
	virtual int visitAttribNumber(DFT::AST::ASTAttribNumber* an) {
		int valid = true;
		valid = ASTVisitor<int,true>::visitAttribNumber(an) ? valid : false;
		return valid;
	}
	virtual int visitAttribString(DFT::AST::ASTAttribString* as) {
		int valid = true;
		valid = ASTVisitor<int,true>::visitAttribString(as) ? valid : false;
		return valid;
	}
};

} // Namespace: DFT

#endif // ASTVALIDATOR_H
