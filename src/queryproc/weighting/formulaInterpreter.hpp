/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_FORMULA_INTERPRETER_HPP_INCLUDED
#define _STRUS_FORMULA_INTERPRETER_HPP_INCLUDED
#include <string>
#include <vector>
#include <map>
#include <ostream>
#include <stdint.h>

namespace strus
{

/// \class FormulaInterpreter
/// \brief Parser for formulas used by weighting schemes
class FormulaInterpreter
{
public:
	struct IteratorSpec
	{
		int typeidx;
		unsigned int size;

		bool defined() const
		{
			return typeidx >= 0;
		}
		IteratorSpec()
			:typeidx(-1),size(0){}
		IteratorSpec( const IteratorSpec& o)
			:typeidx(o.typeidx),size(o.size){}
		IteratorSpec( unsigned int typeidx_, unsigned int size_)
			:typeidx(typeidx_),size(size_){}
	};

	///\brief Function for getting an iterator specification for iteration on a set
	typedef IteratorSpec (*IteratorMap)( void* ctx, const char* type);
	///\brief Function for mapping a variable. Returns NAN, if variable not defined
	typedef double (*VariableFunction)( void* ctx, int typeidx, unsigned int idx);
	///\brief Binary function of the two topmost elements of the stack
	typedef double (*BinaryFunction)( double arg1, double arg2);
	///\brief Binary function of the topmost element of the stack
	typedef double (*UnaryFunction)( double arg);

	struct VariableMap
	{
		VariableFunction function;
		int idx;

		VariableMap( VariableFunction function_, int idx_=-1)
			:function(function_),idx(idx_){}
		VariableMap( const VariableMap& o)
			:function(o.function),idx(o.idx){}
		VariableMap()
			:function(0),idx(-1){}
	};

	class FunctionMap
	{
	public:
		explicit FunctionMap( IteratorMap func)
			:m_iteratorMap(func){}

		void defineVariableMap( const std::string& name, VariableMap func);
		VariableMap getVariableMap( const std::string& name) const;

		void defineUnaryFunction( const std::string& name, UnaryFunction func);
		UnaryFunction getUnaryFunction( const std::string& name) const;
		const std::string& getUnaryFunctionName( UnaryFunction func) const;

		void defineBinaryFunction( const std::string& name, BinaryFunction func);
		BinaryFunction getBinaryFunction( const std::string& name) const;
		const std::string& getBinaryFunctionName( BinaryFunction func) const;

		IteratorMap getIteratorMap() const;
		std::string tostring() const;

	private:
		IteratorMap m_iteratorMap;
		std::map<std::string,VariableMap> m_varmap;
		std::map<std::string,UnaryFunction> m_unaryfuncmap;
		std::map<std::string,BinaryFunction> m_binaryfuncmap;
		std::map<uintptr_t,std::string> m_namemap;
	};

public:
	FormulaInterpreter( const FormulaInterpreter& o)
		:m_program(o.m_program),m_strings(o.m_strings),m_funcmap(o.m_funcmap),m_iteratorMap(o.m_iteratorMap){}

	FormulaInterpreter( const FunctionMap& functionMap, const std::string& source);

	double run( void* ctx) const;
	void print( std::ostream& out) const;

private:
	void parseVariableExpression( const FunctionMap& functionMap, std::string::const_iterator& si, const std::string::const_iterator& se);
	void parseFunctionCall( const FunctionMap& functionMap, const std::string& funcname, std::string::const_iterator& si, const std::string::const_iterator& se);
	void parseOperand( const FunctionMap& functionMap, std::string::const_iterator& si, const std::string::const_iterator& se);
	unsigned int parseSubExpression( const FunctionMap& functionMap, std::string::const_iterator& si, const std::string::const_iterator& se, char eb);
	unsigned int allocVariable( const std::string& name);

private:
	enum OpCode {
		OpMark,				///< Set the current IP as the point to jump to with OpAgain
		OpLoop,				///< Push a new loop context (argument loop variable reference operand.idx)
		OpAgain,			///< Increment loop counter, check if still in range of the current foreach clause and jump to the last OpMark if yes. Clean current foreach context and fetch the following operation if not (no argument)
		OpPushConst,			///< Push a constant on the stack (argument operand.value)
		OpPushVar,			///< Push a variable on the stack (argument variable reference operand.idx)
		OpPushDim,			///< Push the dimension of an array on the stack (argument variable reference operand.idx)
		OpUnaryFunction,		///< Call an unary function with the topmost element on the stack, pop the topmost element from the stack and push the result on the stack
		OpBinaryFunction		///< Call a binary function with the two topmost elements on the stack, pop the argument elements from the stack and push the result on the stack
	};
	static const char* opCodeName( OpCode i)
	{
		static const char* ar[] = {"mark","loop","again","push const","push var","push dim","unary function","binary function",0};
		return ar[ (unsigned int)i];
	}

	struct OpStruct
	{
		OpCode opCode;
		union
		{
			double value;
			unsigned int idx;
			UnaryFunction unaryFunction;
			BinaryFunction binaryFunction;
		} operand;
		OpStruct( OpCode opCode_)
			:opCode(opCode_)
		{
			operand.value = 0.0;
		}
		OpStruct( OpCode opCode_, int value_)
			:opCode(opCode_)
		{
			operand.idx = value_;
		}
		OpStruct( OpCode opCode_, double value_)
			:opCode(opCode_)
		{
			operand.value = value_;
		}
		OpStruct( OpCode opCode_, UnaryFunction unaryFunction_)
			:opCode(opCode_)
		{
			operand.unaryFunction = unaryFunction_;
		}
		OpStruct( OpCode opCode_, BinaryFunction binaryFunction_)
			:opCode(opCode_)
		{
			operand.binaryFunction = binaryFunction_;
		}
		OpStruct( const OpStruct& o)
			:opCode(o.opCode),operand(o.operand)
		{}
		void print( std::ostream& out, const std::string& strings, const std::vector<VariableMap>& variablear, const FunctionMap& funcmap) const;
	};

private:
	std::vector<OpStruct> m_program;
	std::string m_strings;
	std::vector<VariableMap> m_variablear;
	FunctionMap m_funcmap;
	IteratorMap m_iteratorMap;
};

}//namespace
#endif

