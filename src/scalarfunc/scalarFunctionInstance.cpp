/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Impementation for parameterizing a scalar function built with the standard scalar function parser of strus
/// \file scalarFunctionInstance.cpp
#include "scalarFunctionInstance.hpp"
#include "strus/errorBufferInterface.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace strus;

std::vector<std::string> ScalarFunctionInstance::getVariables() const
{
	try
	{
		return m_func->getVariables();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error fetching variable names of scalar function: %s"), *m_errorhnd, std::vector<std::string>());
}

std::size_t ScalarFunctionInstance::getNofArguments() const
{
	return m_nof_args;
}

void ScalarFunctionInstance::setVariableValue( const std::string& name, double value)
{
	try
	{
		m_valuear[ m_func->getVariableIndex( name)] = value;
	}
	CATCH_ERROR_MAP( _TXT("error setting scalar function variable value: %s"), *m_errorhnd);
}

double ScalarFunctionInstance::call( const double* args, std::size_t nofargs) const
{
	try
	{
		if (nofargs < m_nof_args)
		{
			throw strus::runtime_error( "too few arguments passed to scalar function");
		}
		std::size_t ip = 0;
		std::vector<double> stk;
		std::vector<double> reg;
		std::size_t idxreg = 0;

		for (;m_func->hasInstruction(ip); ++ip)
		{
			switch (m_func->opCode(ip))
			{
				case ScalarFunction::OpLdCnt:
				{
					idxreg = m_func->getIndexOperand( ip, stk.size());
					break;
				}
				case ScalarFunction::OpPush:
				{
					std::size_t validx = m_func->getIndexOperand( ip, m_valuear.size());
					stk.push_back( m_valuear[ validx]);
					break;
				}
				case ScalarFunction::OpArg:
				{
					std::size_t validx = m_func->getIndexOperand( ip, nofargs);
					stk.push_back( args[ validx]);
					break;
				}
				case ScalarFunction::OpSto:
				{
					if (stk.empty()) throw strus::runtime_error(_TXT("illegal stack operation"));
					reg.push_back( stk.back());
					stk.pop_back();
					break;
				}
				case ScalarFunction::OpRcl:
				{
					stk.insert( stk.end(), reg.begin(), reg.end());
					reg.clear();
					break;
				}
				case ScalarFunction::OpDup:
				{
					if (stk.empty()) throw strus::runtime_error(_TXT("illegal stack operation"));
					stk.push_back( stk.back());
					break;
				}
				case ScalarFunction::OpAdd:
				{
					if (stk.size() < 2) throw strus::runtime_error(_TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					stk.push_back( a1 + a2);
					break;
				}
				case ScalarFunction::OpSub:
				{
					if (stk.size() < 2) throw strus::runtime_error(_TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					stk.push_back( a1 - a2);
					break;
				}
				case ScalarFunction::OpDiv:
				{
					if (stk.size() < 2) throw strus::runtime_error(_TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					stk.push_back( a1 / a2);
					break;
				}
				case ScalarFunction::OpMul:
				{
					if (stk.size() < 2) throw strus::runtime_error(_TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					stk.push_back( a1 * a2);
					break;
				}
				case ScalarFunction::OpMin:
				{
					if (stk.size() < 2) throw strus::runtime_error(_TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					stk.push_back( a1 > a2 ? a2 : a1);
					break;
				}
				case ScalarFunction::OpMax:
				{
					if (stk.size() < 2) throw strus::runtime_error(_TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					stk.push_back( a1 > a2 ? a1 : a2);
					break;
				}
				case ScalarFunction::FuncUnary:
				{
					if (stk.size() < 1) throw strus::runtime_error(_TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -1];
					stk.resize( stk.size() -1);
					ScalarFunction::UnaryFunction func = m_func->getUnaryFunctionOperand( ip);
					stk.push_back( func( a1));
					break;
				}
				case ScalarFunction::FuncBinary:
				{
					if (stk.size() < 2) throw strus::runtime_error(_TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					ScalarFunction::BinaryFunction func = m_func->getBinaryFunctionOperand( ip);
					stk.push_back( func( a1, a2));
					break;
				}
				case ScalarFunction::FuncNary:
				{
					if (stk.size() < idxreg) throw strus::runtime_error(_TXT("illegal stack operation"));
					const double* arg = stk.data() + (stk.size() - idxreg);
					ScalarFunction::NaryFunction func = m_func->getNaryFunctionOperand( ip);
					double res = func( idxreg, arg);
					stk.resize( stk.size() - idxreg);
					stk.push_back( res);
					break;
				}
			}
		}
		return stk.size()?stk.back():0.0;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error executing scalar function: %s"), *m_errorhnd, 0.0);
}

std::string ScalarFunctionInstance::tostring() const
{
	try
	{
		std::ostringstream dmp;
		dmp << std::fixed << std::setprecision( 6);
	
		for (std::size_t ip=0; m_func->hasInstruction(ip); ++ip)
		{
			ScalarFunction::OpCode opCode = m_func->opCode(ip);
			dmp << "[" << ip << "] " << ScalarFunction::opCodeName( opCode);
			switch (opCode)
			{
				case ScalarFunction::OpLdCnt:
				{
					dmp << " " << m_func->getIndexOperand( ip);
					break;
				}
				case ScalarFunction::OpPush:
				{
					std::size_t validx = m_func->getIndexOperand( ip, m_valuear.size());
					double value = m_valuear[ validx];
					dmp << " " << value;
					break;
				}
				case ScalarFunction::OpArg:
				{
					std::size_t validx = m_func->getIndexOperand( ip);
					dmp << " " << validx;
					break;
				}
				case ScalarFunction::OpSto:
				case ScalarFunction::OpRcl:
				case ScalarFunction::OpDup:
				case ScalarFunction::OpAdd:
				case ScalarFunction::OpSub:
				case ScalarFunction::OpDiv:
				case ScalarFunction::OpMul:
				case ScalarFunction::OpMin:
				case ScalarFunction::OpMax:
				case ScalarFunction::FuncUnary:
				case ScalarFunction::FuncBinary:
				case ScalarFunction::FuncNary:
				{
					break;
				}
			}
			dmp << std::endl;
		}
		return dmp.str();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error building string representation of scalar function: %s"), *m_errorhnd, std::string());
}
