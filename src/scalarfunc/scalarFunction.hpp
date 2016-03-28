/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation for building the virtual machine of a scalar function as virtual machine
/// \file scalarFunction.hpp
#ifndef _STRUS_SCALAR_FUNCTION_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_SCALAR_FUNCTION_IMPLEMENTATION_HPP_INCLUDED
#include "strus/scalarFunctionInterface.hpp"
#include "strus/scalarFunctionParserInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <string>
#include <vector>
#include <map>
#include <limits>
#include <stdint.h>

namespace strus
{
/// \brief Forward declaration
class ScalarFunctionInstanceInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Implementation for building the virtual machine of a scalar function as virtual machine
class ScalarFunction
	:public ScalarFunctionInterface
{
public:
	explicit ScalarFunction( ErrorBufferInterface* errorhnd_);

	virtual ~ScalarFunction(){}

	typedef ScalarFunctionParserInterface::UnaryFunction UnaryFunction;
	typedef ScalarFunctionParserInterface::BinaryFunction BinaryFunction;
	typedef ScalarFunctionParserInterface::NaryFunction NaryFunction;

	enum OpCode
	{
		OpLdCnt,	//< Load a counter for the next operation (FuncNary)
		OpPush,		//< Push a numeric value on the stack
		OpArg,		//< Push an argument on the stack
		OpNeg,		//< Replace the topmost element on the stack by its negation
		OpAdd,		//< Add two topmost values removed from the stack and put result back on the stack
		OpSub,		//< Subtract operation of two topmost values removed from the stack and put result back on the stack
		OpDiv,		//< Divide operation of two topmost values removed from the stack and put result back on the stack
		OpMul,		//< Multiply two topmost values removed from the stack and put result back on the stack
		OpMin,		//< Minimum operation of two topmost values removed from the stack and put result back on the stack
		OpMax,		//< Maximum operation of two topmost values removed from the stack and put result back on the stack
		FuncUnary,	//< Unary function call of the topmost value removed from the stack and put result back on the stack
		FuncBinary,	//< Binary function call of the two topmost values removed from the stack and put result back on the stack
		FuncNary	//< N-ary function call of the N topmost values removed from the stack and put result back on the stack. The N is loaded with the preceeding OpLdCnt operation.
	};
	static const char* opCodeName( OpCode opCode)
	{
		static const char* ar[] = {"LdCnt", "Push", "Arg", "Neg", "Add", "Sub", "Div", "Mul", "Min", "Max", "FuncUnary", "FuncBinary", "FuncNary"};
		return ar[ opCode];
	}

	/// \brief Add a push variable operation to the execution code of the scalar function
	/// \param[in] name name of the variable
	void addOpPushVariable( const std::string& name);

	/// \brief Add a push argument operation to the execution code of the scalar function
	/// \param[in] name name of the variable
	void addOpPushArgument( std::size_t argindex);

	/// \brief Add a push constant operation to the execution code of the scalar function
	/// \param[in] value value of the constant
	void addOpPushConstant( double value);

	/// \brief Add an arithmetic operation to the execution code of the scalar function
	/// \param[in] op opcode of the arithmetic operation
	void addOp( const OpCode& op);

	/// \brief Add a function call of an unary function to the execution code of the scalar function
	/// \param[in] func function call added
	void addOpUnaryFunctionCall( UnaryFunction func);

	/// \brief Add a function call of a binary function to the execution code of the scalar function
	/// \param[in] func function call added
	void addOpBinaryFunctionCall( BinaryFunction func);

	/// \brief Add a function call of an N-ary function to the execution code of the scalar function
	/// \param[in] nofargs number of arguments of the function call added
	/// \param[in] func function added
	void addOpNaryFunctionCall( std::size_t nofargs, NaryFunction func);

	/// \brief Reverse lookup of the name of a variable from a value index, if it exists
	/// \return the name of the variable or NULL, if it does not exist
	const char* reverseLookupVariableName( std::size_t validx) const;

public://ScalarFunctionInstance

	/// \brief Get the current opcode (at instruction pointer)
	OpCode opCode( std::size_t ip) const
	{
		return (OpCode)(m_instructionar[ ip] >> InstructionOpodeShift);
	}
	/// \brief Find out if there are more instructions to execute at an instruction pointer address
	bool hasInstruction( std::size_t ip) const
	{
		return m_instructionar.size() > ip;
	}
	std::size_t getIndexOperand( std::size_t ip, std::size_t endval=std::numeric_limits<std::size_t>::max()) const
	{
		std::size_t rt = m_instructionar[ ip] &~ (0x1F << InstructionOpodeShift);
		if (rt >= endval)
		{
			throw strus::runtime_error(_TXT("illegal operand in instruction"));
		}
		return rt;
	}
	BinaryFunction getBinaryFunctionOperand( std::size_t ip) const
	{
		std::size_t rtidx = m_instructionar[ ip] &~ (0x1F << InstructionOpodeShift);
		if (rtidx >= m_binfuncar.size())
		{
			throw strus::runtime_error(_TXT("illegal operand in instruction"));
		}
		return m_binfuncar[ rtidx];
	}
	UnaryFunction getUnaryFunctionOperand( std::size_t ip) const
	{
		std::size_t rtidx = m_instructionar[ ip] &~ (0x1F << InstructionOpodeShift);
		if (rtidx >= m_unfuncar.size())
		{
			throw strus::runtime_error(_TXT("illegal operand in instruction"));
		}
		return m_unfuncar[ rtidx];
	}
	NaryFunction getNaryFunctionOperand( std::size_t ip) const
	{
		std::size_t rtidx = m_instructionar[ ip] &~ (0x1F << InstructionOpodeShift);
		if (rtidx >= m_nfuncar.size())
		{
			throw strus::runtime_error(_TXT("illegal operand in instruction"));
		}
		return m_nfuncar[ rtidx];
	}

	/// \brief Get the internal value index of a variable
	std::size_t getVariableIndex( const std::string& name) const;

public:
	virtual std::vector<std::string> getVariables() const;
	virtual std::size_t getNofArguments() const;

	virtual ScalarFunctionInstanceInterface* createInstance() const;
	virtual std::string tostring() const;

private:
	void pushInstruction( const OpCode& op, unsigned int operand);

private:
	ErrorBufferInterface* m_errorhnd;
	enum {InstructionOpodeShift=27};
	typedef uint32_t Instruction;
	std::vector<Instruction> m_instructionar;
	std::vector<UnaryFunction> m_unfuncar;
	std::vector<BinaryFunction> m_binfuncar;
	std::vector<NaryFunction> m_nfuncar;
	std::vector<double> m_valuear;
	std::size_t m_nofargs;

	typedef std::map<std::string,std::size_t> VariableMap;
	VariableMap m_variablemap;
};

}// namespace
#endif

