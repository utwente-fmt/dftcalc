/*
 * ASTVisitor.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#ifndef ASTVISITOR_H
#define ASTVISITOR_H

#include "dft_ast.h"



namespace DFT {

/**
 * This class describes a visitor pattern for the AST.
 * It provides means for a subclass to parse an AST without having to
 * reimplement the structure of the AST.
 * The template variable TReturn specifies the datatype that is returned by
 * all visit*() methods. The template variable TReturnInit specifies the
 * initial value of what visit*() methods return, before any aggregation.
 */
template<typename TReturn=int, TReturn TReturnInit=0>
class ASTVisitor {
public:

	/**
	 * The function prototype of an aggregator that can be specified in order
	 * to aggregate return values of multiple children into one return value
	 * to be returned by the parent.
	 */
	typedef void (*AggregateFunction)(TReturn& result, TReturn value);

protected:
	DFT::AST::ASTNodes* ast;
	CompilerContext* cc;
	AggregateFunction f_aggregate;
	virtual void aggregate(TReturn& result, const TReturn& value) {
		result = value;
	}
public:

	/**
	 * Constructs a new ASTVisitor instance using the specified
	 * AST, CompilerContext and AggregateFunction.
	 * The AggregateFunction is called whenever a parent visits a child node
	 * of his, providing the means to aggregate the return value of
	 * the child's visit*() method calls. Then the parent returns the
	 * aggregated value.
	 * The template variable TReturnInit specifies the initial value of the
	 * return value, before any aggregation.
	 * Example:
	 *   TReturn     = int
	 *   TReturnInit = 0
	 *   f_aggregate = [](int& ret, int val){ret = ret + val;}
	 * Consider a parent node P with two children nodes A and B. A call to
	 * visitParent(P) would roughly go like this:
	 *   int ret = 0;
	 *   ret = f_aggregate(ret,visitChild(A)); // is: ret = ret + visitChild(A)
	 *   ret = f_aggregate(ret,visitChild(B)); // is: ret = ret + visitChild(B)
	 *   return ret;
	 * Thus, if visitChild(A) and visitChild(B) would return respectively
	 * 3 and 5, visitParent(P) would return 8.
	 * Call visit() to start the parsing of the AST. This will follow the AST
	 * and call the overridden visit*() methods. To continue the parsing from
	 * visitFoo(...), call ASTVisitor::visitFoo(...).
	 */
	ASTVisitor(DFT::AST::ASTNodes* ast, CompilerContext* cc, AggregateFunction f_aggregate):
		ast(ast),
		cc(cc),
		f_aggregate(f_aggregate) {
	}
	
	/**
	 * Starts the visiting of the AST specified in the constructor.
	 * This will follow the AST and call the overridden visit*() methods.
	 * To continue the parsing from visitFoo(...),
	 * call ASTVisitor::visitFoo(...).
	 */
	virtual TReturn visit() {
		TReturn ret = TReturnInit;
		for(int i=0; i<ast->size(); ++i) {
			DFT::AST::ASTNode* node = ast->at(i);
			switch(node->getType()) {
			case DFT::AST::TopLevelType: {
				DFT::AST::ASTTopLevel* t = static_cast<DFT::AST::ASTTopLevel*>(node);
				//ret = visitTopLevel(t);
				aggregate(ret,visitTopLevel(t));
				break;
			}
			case DFT::AST::BasicEventType: {
				DFT::AST::ASTBasicEvent* be = static_cast<DFT::AST::ASTBasicEvent*>(node);
				aggregate(ret,visitBasicEvent(be));
				break;
			}
			case DFT::AST::GateType: {
				DFT::AST::ASTGate* g = static_cast<DFT::AST::ASTGate*>(node);
				aggregate(ret,visitGate(g));
				break;
			}
			case DFT::AST::PageType: {
				DFT::AST::ASTPage* p = static_cast<DFT::AST::ASTPage*>(node);
				aggregate(ret,visitPage(p));
				break;
			}
			default:
				cc->reportErrorAt(node->getLocation(),"unexpected input");
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
		std::vector<DFT::AST::ASTAttribute*>::iterator it = basicEvent->getAttributes()->begin();
		for(; it!=basicEvent->getAttributes()->end(); ++it) {
			aggregate(ret,visitAttribute(*it));
		}
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
		
		// Float attribute
		case DFT::AST::BEAttributeFloatType: {
			DFT::AST::ASTAttribFloat* af = static_cast<DFT::AST::ASTAttribFloat*>(attrib);
			ret = visitAttribFloat(af);
			break;
		}

		// Integer attribute
		case DFT::AST::BEAttributeNumberType: {
			DFT::AST::ASTAttribNumber* an = static_cast<DFT::AST::ASTAttribNumber*>(attrib);
			ret = visitAttribNumber(an);
			break;
		}

		// String attribute
		case DFT::AST::BEAttributeStringType: {
			DFT::AST::ASTAttribString* as = static_cast<DFT::AST::ASTAttribString*>(attrib);
			ret = visitAttribString(as);
			break;
		}

		// Unknown attribute
		default:
			cc->reportErrorAt(attrib->getLocation(),"unexpected input");
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
