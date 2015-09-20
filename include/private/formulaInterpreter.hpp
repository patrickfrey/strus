/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
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

namespace strus
{

/// \class FormulaInterpreter
/// \brief Parser for formulas used by weighting schemes
class FormulaInterpreter
{
public:
private:
	///\brief Function for mapping a variable. Returns -1, if variable not defined
	typedef int (*DimMap)( void* ctx, const char* type);
	///\brief Function for mapping a variable. Returns NAN, if variable not defined
	typedef double (*VariableMap)( void* ctx, const char* type, unsigned int idx);
	///\brief Binary function of the two topmost elements of the stack
	typedef double (*BinaryFunction)( double arg1, double arg2);
	///\brief Binary function of the topmost element of the stack
	typedef double (*UnaryFunction)( double arg);

	class Context
	{
	public:
		Context( DimMap func)
			:m_dimmap(func){}

		void defineVariableMap( const std::string& name, VariableMap func);
		VariableMap getVariableMap( const std::string& name) const;

		void defineUnaryFunction( const std::string& name, UnaryFunction func);
		UnaryFunction getUnaryFunction( const std::string& name) const;

		void defineBinaryFunction( const std::string& name, BinaryFunction func);
		BinaryFunction getBinaryFunction( const std::string& name) const;

		DimMap getDimMap() const;

	private:
		DimMap m_dimmap;
		std::map<std::string,VariableMap> m_varmap;
		std::map<std::string,UnaryFunction> m_unaryfuncmap;
		std::map<std::string,BinaryFunction> m_binaryfuncmap;
	};

public:
	FormulaInterpreter( const FormulaInterpreter& o)
		:m_program(o.m_program),m_strings(o.m_strings),m_dimmap(o.m_dimmap){}

	FormulaInterpreter( const Context& context, const std::string& source);

	double run( void* ctx) const;

private:
	void parseVariableExpression( const Context& context, std::string::const_iterator& si, const std::string::const_iterator& se);
	void parseFunctionCall( const Context& context, const std::string& funcname, std::string::const_iterator& si, const std::string::const_iterator& se);
	void parseOperand( const Context& context, std::string::const_iterator& si, const std::string::const_iterator& se);
	unsigned int parseSubExpression( const Context& context, std::string::const_iterator& si, const std::string::const_iterator& se, char eb);
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
	struct OpStruct
	{
		OpCode opCode;
		union
		{
			double value;
			unsigned int idx;
			VariableMap variableMap;
			UnaryFunction unaryFunction;
			BinaryFunction binaryFunction;
		} operand;
		OpStruct( OpCode opCode_)
			:opCode(opCode_)
		{
			operand.value = 0.0;
		}
		OpStruct( OpCode opCode_, double value_)
			:opCode(opCode_)
		{
			operand.value = value_;
		}
		OpStruct( OpCode opCode_, VariableMap VariableMap_)
			:opCode(opCode_)
		{
			operand.variableMap = VariableMap_;
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
	};

private:
	std::vector<OpStruct> m_program;
	std::string m_strings;
	DimMap m_dimmap;
};

}//namespace
#endif

