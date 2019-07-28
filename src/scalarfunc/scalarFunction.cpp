/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation for building the virtual machine of a scalar function as virtual machine
/// \file scalarFunction.cpp
#include "scalarFunction.hpp"
#include "scalarFunctionInstance.hpp"
#include "strus/errorBufferInterface.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <memory>

using namespace strus;

ScalarFunction::ScalarFunction( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_),m_nofargs(0){}

bool ScalarFunction::isLinearComb( std::vector<double>& values) const
{
	std::size_t ip = 0;
	std::size_t aidx = 0;
	if (!m_variablemap.empty()) return false;
	values.clear();

	if (ip+2 < m_instructionar.size() && opCode(ip) == OpPush && opCode(ip+1) == OpArg && indexOperand(ip+1) == aidx && opCode(ip+2) == OpMul)
	{
		values.push_back( m_valuear[indexOperand(ip)]);
		aidx++;
		ip += 3;
	}
	else if (ip < m_instructionar.size() && opCode(ip) == OpArg && indexOperand(ip) == aidx)
	{
		values.push_back( 1.0);
		aidx++;
		ip += 1;
	}
	else
	{
		return false;
	}
	while (ip < m_instructionar.size())
	{
		if (ip+3 < m_instructionar.size() && opCode(ip) == OpPush && opCode(ip+1) == OpArg && indexOperand(ip+1) == aidx && opCode(ip+2) == OpMul && opCode(ip+3) == OpAdd)
		{
			values.push_back( m_valuear[indexOperand(ip)]);
			aidx++;
			ip+=4;
		}
		else if (ip+1 < m_instructionar.size() && opCode(ip) == OpArg && indexOperand(ip) == aidx && opCode(ip+1) == OpAdd)
		{
			values.push_back( 1.0);
			aidx++;
			ip+=2;
		}
		else
		{
			return false;
		}
	}
	return ip == m_instructionar.size();
}

void ScalarFunction::pushInstruction( const OpCode& op, unsigned int operand)
{
	if (operand >= (1 << InstructionOpodeShift))
	{
		throw strus::runtime_error( "%s",  _TXT( "operand out of range in scalar function"));
	}
	Instruction instr = ((Instruction)op << InstructionOpodeShift) + operand;
	m_instructionar.push_back( instr);
}

void ScalarFunction::addOpPushVariable( const std::string& name_)
{
	VariableMap::const_iterator vi = m_variablemap.find( name_);
	unsigned int validx;
	if (vi == m_variablemap.end())
	{
		validx = m_valuear.size();
		m_variablemap[ name_] = validx;
		m_valuear.push_back(0.0);
	}
	else
	{
		validx = vi->second;
	}
	pushInstruction( OpPush, validx);
}

void ScalarFunction::addOpPushArgument( unsigned int argindex)
{
	if (m_nofargs <= argindex)
	{
		m_nofargs = argindex+1;
	}
	pushInstruction( OpArg, argindex);
}

void ScalarFunction::addOpPushConstant( double value)
{
	unsigned int validx = m_valuear.size();
	m_valuear.push_back( value);
	pushInstruction( OpPush, validx);
}

void ScalarFunction::addOp( const OpCode& op)
{
	pushInstruction( op, 0);
}

void ScalarFunction::addOpUnaryFunctionCall( UnaryFunction func)
{
	unsigned int validx = m_unfuncar.size();
	m_unfuncar.push_back( func);
	pushInstruction( FuncUnary, validx);
}

void ScalarFunction::addOpBinaryFunctionCall( BinaryFunction func)
{
	unsigned int validx = m_binfuncar.size();
	m_binfuncar.push_back( func);
	pushInstruction( FuncBinary, validx);
}

void ScalarFunction::addOpNaryFunctionCall( unsigned int nofargs, NaryFunction func)
{
	pushInstruction( OpLdCnt, nofargs);
	unsigned int validx = m_nfuncar.size();
	m_nfuncar.push_back( func);
	pushInstruction( FuncNary, validx);
}

const char* ScalarFunction::reverseLookupVariableName( std::size_t validx) const
{
	VariableMap::const_iterator vi = m_variablemap.begin(), ve = m_variablemap.end();
	for (; vi != ve; ++vi)
	{
		if (vi->second == validx) return vi->first.c_str();
	}
	return 0;
}

std::size_t ScalarFunction::getVariableIndex( const std::string& name_) const
{
	VariableMap::const_iterator vi = m_variablemap.find( name_);
	if (vi == m_variablemap.end()) throw strus::runtime_error("variable '%s' is not defined in scalar function", name_.c_str());
	return vi->second;
}

std::vector<std::string> ScalarFunction::getVariables() const
{
	std::vector<std::string> rt;
	VariableMap::const_iterator vi = m_variablemap.begin(), ve = m_variablemap.end();
	for (; vi != ve; ++vi)
	{
		rt.push_back( vi->first);
	}
	return rt;
}

unsigned int ScalarFunction::getNofArguments() const
{
	return m_nofargs;
}

void ScalarFunction::setDefaultVariableValue( const std::string& name_, double value)
{
	try
	{
		m_valuear[ getVariableIndex( name_)] = value;
	}
	CATCH_ERROR_MAP( _TXT("error setting default variable value of scalar function: %s"), *m_errorhnd);
}

ScalarFunctionInstanceInterface* ScalarFunction::createInstance() const
{
	try
	{
		return new ScalarFunctionInstance( this, m_valuear, m_nofargs, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating instance of scalar function: %s"), *m_errorhnd, 0);
}

StructView ScalarFunction::view() const
{
	try
	{
		StructView rt;
		for (std::size_t ip=0; ip<m_instructionar.size(); ++ip)
		{
			StructView op;
			ScalarFunction::OpCode oc = opCode(ip);
			op( "ip", (int)ip)( "opcode", ScalarFunction::opCodeName( oc));
			switch (oc)
			{
				case ScalarFunction::OpLdCnt:
				{
					op( "arg", (int)getIndexOperand( ip));
					break;
				}
				case ScalarFunction::OpPush:
				{
					std::size_t validx = getIndexOperand( ip, m_valuear.size());
					double value = m_valuear[ validx];
					if (value == 0.0)
					{
						const char* varname = reverseLookupVariableName( validx);
						if (varname)
						{
							op( "argvar", varname);
						}
						else
						{
							op( "arg", value);
						}
					}
					else
					{
						op( "arg", value);
					}
					break;
				}
				case ScalarFunction::OpArg:
				{
					std::size_t validx = getIndexOperand( ip, m_nofargs);
					op( "arg", (int)validx);
					break;
				}
				case ScalarFunction::OpNeg:
				case ScalarFunction::OpAdd:
				case ScalarFunction::OpSub:
				case ScalarFunction::OpDiv:
				case ScalarFunction::OpMul:
				case ScalarFunction::FuncUnary:
				case ScalarFunction::FuncBinary:
				case ScalarFunction::FuncNary:
				{
					break;
				}
			}
			rt( op);
		}
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error building introspection structure of scalar function: %s"), *m_errorhnd, StructView());
}

