#ifndef ASTVISITOR_H
#define ASTVISITOR_H

#include "dft_ast.h"



namespace DFT {

template<typename TReturn=int, TReturn TReturnInit=0>
class ASTVisitor {
public:
	typedef void (*AggregateFunction)(TReturn& result, TReturn value);
protected:
	std::vector<DFT::AST::ASTNode*>* ast;
	Parser* parser;
	AggregateFunction f_aggregate;
public:

	ASTVisitor(std::vector<DFT::AST::ASTNode*>* ast, Parser* parser, AggregateFunction f_aggregate):
		ast(ast),
		parser(parser),
		f_aggregate(f_aggregate) {
	}
	
	virtual TReturn visit() {
		TReturn ret = TReturnInit;
		for(int i=0; i<ast->size(); ++i) {
			DFT::AST::ASTNode* node = ast->at(i);
			switch(node->getType()) {
			case DFT::AST::TopLevelType: {
				DFT::AST::ASTTopLevel* t = static_cast<DFT::AST::ASTTopLevel*>(node);
				//ret = visitTopLevel(t);
				f_aggregate(ret,visitTopLevel(t));
				break;
			}
			case DFT::AST::BasicEventType: {
				DFT::AST::ASTBasicEvent* be = static_cast<DFT::AST::ASTBasicEvent*>(node);
				f_aggregate(ret,visitBasicEvent(be));
				break;
			}
			case DFT::AST::GateType: {
				DFT::AST::ASTGate* g = static_cast<DFT::AST::ASTGate*>(node);
				f_aggregate(ret,visitGate(g));
				break;
			}
			case DFT::AST::PageType: {
				DFT::AST::ASTPage* p = static_cast<DFT::AST::ASTPage*>(node);
				f_aggregate(ret,visitPage(p));
				break;
			}
			default:
				parser->getCC()->reportErrorAt(node->getLocation(),"unexpected input");
				break;
			}
		}
		return ret;
	}
	virtual TReturn visitTopLevel(DFT::AST::ASTTopLevel* topLevel) {
		TReturn ret = TReturnInit;
		return ret;
	}
	virtual TReturn visitBasicEvent(DFT::AST::ASTBasicEvent* basicEvent) {
		TReturn ret = TReturnInit;
		return ret;
	}
	virtual TReturn visitGate(DFT::AST::ASTGate* gate) {
		TReturn ret = TReturnInit;
		return ret;
	}
	virtual TReturn visitPage(DFT::AST::ASTPage* page) {
		TReturn ret = TReturnInit;
		return ret;
	}
	virtual TReturn visitAttribute(DFT::AST::ASTAttribute* attribute) {
		TReturn ret = TReturnInit;
		return ret;
	}
	virtual TReturn visitAttrib(DFT::AST::ASTAttrib* attrib) {
		TReturn ret = TReturnInit;
		switch(attrib->getType()) {
		case DFT::AST::BEAttributeFloatType: {
			DFT::AST::ASTAttribFloat* af = static_cast<DFT::AST::ASTAttribFloat*>(attrib);
			ret = visitAttribFloat(af);
			break;
		}
		case DFT::AST::BEAttributeNumberType: {
			DFT::AST::ASTAttribNumber* an = static_cast<DFT::AST::ASTAttribNumber*>(attrib);
			ret = visitAttribNumber(an);
			break;
		}
		case DFT::AST::BEAttributeStringType: {
			DFT::AST::ASTAttribString* as = static_cast<DFT::AST::ASTAttribString*>(attrib);
			ret = visitAttribString(as);
			break;
		}
		default:
			parser->getCC()->reportErrorAt(attrib->getLocation(),"unexpected input");
			break;
		}
		return ret;
	}
	virtual TReturn visitAttribFloat(DFT::AST::ASTAttribFloat* af) {
		TReturn ret = TReturnInit;
		return ret;
	}
	virtual TReturn visitAttribNumber(DFT::AST::ASTAttribNumber* an) {
		TReturn ret = TReturnInit;
		return ret;
	}
	virtual TReturn visitAttribString(DFT::AST::ASTAttribString* as) {
		TReturn ret = TReturnInit;
		return ret;
	}
};

class ASTVisitorDefault: public ASTVisitor<> {
};

} // Namespace: DFT

#endif // ASTVISITOR_H
