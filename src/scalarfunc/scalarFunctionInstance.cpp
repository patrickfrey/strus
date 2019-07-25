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
#include "strus/base/math.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG

void ScalarFunctionInstance::setVariableValue( const std::string& name, double value)
{
	try
	{
		m_valuear[ m_func->getVariableIndex( name)] = value;
	}
	CATCH_ERROR_MAP( _TXT("error setting scalar function variable value: %s"), *m_errorhnd);
}

double ScalarFunctionInstance::call( const double* args, unsigned int nofargs) const
{
	try
	{
		if (nofargs < m_nof_args)
		{
			throw strus::runtime_error( _TXT("too few arguments passed to scalar function (%u < %u)"), (unsigned int)nofargs, (unsigned int)m_nof_args);
		}
		std::size_t ip = 0;
		std::vector<double> stk;
		std::size_t idxreg = 0;

		for (;m_func->hasInstruction(ip); ++ip)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "EXECUTE [" << ip << "] " << ScalarFunction::opCodeName( m_func->opCode(ip)) << " " << m_func->getIndexOperand( ip) << std::endl;
#endif
			switch (m_func->opCode(ip))
			{
				case ScalarFunction::OpLdCnt:
				{
					idxreg = m_func->getIndexOperand( ip, stk.size()+1);
					break;
				}
				case ScalarFunction::OpPush:
				{
					std::size_t validx = m_func->getIndexOperand( ip, m_valuear.size());
					stk.push_back( m_valuear[ validx]);
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "PUSH " << stk.back() << std::endl;
#endif
					break;
				}
				case ScalarFunction::OpArg:
				{
					std::size_t validx = m_func->getIndexOperand( ip, nofargs);
					stk.push_back( args[ validx]);
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "PUSH " << stk.back() << std::endl;
#endif
					break;
				}
				case ScalarFunction::OpNeg:
				{
					if (stk.size() < 1) throw std::runtime_error( _TXT("illegal stack operation"));
					stk.back() = -stk.back();
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "PUSH " << stk.back() << std::endl;
#endif
					break;
				}
				case ScalarFunction::OpAdd:
				{
					if (stk.size() < 2) throw std::runtime_error( _TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					stk.push_back( a1 + a2);
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "PUSH " << stk.back() << std::endl;
#endif
					break;
				}
				case ScalarFunction::OpSub:
				{
					if (stk.size() < 2) throw std::runtime_error( _TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					stk.push_back( a1 - a2);
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "PUSH " << stk.back() << std::endl;
#endif
					break;
				}
				case ScalarFunction::OpDiv:
				{
					if (stk.size() < 2) throw std::runtime_error( _TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					if (strus::Math::abs( a1) < std::numeric_limits<double>::epsilon())
					{
						stk.push_back( 0.0);
					}
					else if (strus::Math::abs( a2) < std::numeric_limits<double>::epsilon())
					{
						throw std::runtime_error( _TXT("division by zero"));
					}
					else
					{
						stk.push_back( a1 / a2);
					}
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "PUSH " << stk.back() << std::endl;
#endif
					break;
				}
				case ScalarFunction::OpMul:
				{
					if (stk.size() < 2) throw std::runtime_error( _TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					stk.push_back( a1 * a2);
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "PUSH " << stk.back() << std::endl;
#endif
					break;
				}
				case ScalarFunction::FuncUnary:
				{
					if (stk.size() < 1) throw std::runtime_error( _TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -1];
					stk.resize( stk.size() -1);
					ScalarFunction::UnaryFunction func = m_func->getUnaryFunctionOperand( ip);
					stk.push_back( func( a1));
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "PUSH " << stk.back() << std::endl;
#endif
					break;
				}
				case ScalarFunction::FuncBinary:
				{
					if (stk.size() < 2) throw std::runtime_error( _TXT("illegal stack operation"));
					double a1 = stk[ stk.size() -2];
					double a2 = stk[ stk.size() -1];
					stk.resize( stk.size() -2);
					ScalarFunction::BinaryFunction func = m_func->getBinaryFunctionOperand( ip);
					stk.push_back( func( a1, a2));
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "PUSH " << stk.back() << std::endl;
#endif
					break;
				}
				case ScalarFunction::FuncNary:
				{
					if (stk.size() < idxreg) throw std::runtime_error( _TXT("illegal stack operation"));
					const double* arg = stk.data() + (stk.size() - idxreg);
					ScalarFunction::NaryFunction func = m_func->getNaryFunctionOperand( ip);
					double res = func( idxreg, arg);
					stk.resize( stk.size() - idxreg);
					stk.push_back( res);
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "PUSH " << stk.back() << std::endl;
#endif
					break;
				}
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "RESULT " << (stk.size()?stk.back():0.0) << std::endl;
#endif
		return stk.size()?stk.back():0.0;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error executing scalar function: %s"), *m_errorhnd, 0.0);
}


StructView ScalarFunctionInstance::view() const
{
	try
	{
		StructView rt;
		for (std::size_t ip=0; m_func->hasInstruction(ip); ++ip)
		{
			StructView op;
			ScalarFunction::OpCode opCode = m_func->opCode(ip);
			op( "ip", (int)ip)( "opcode", ScalarFunction::opCodeName( opCode));
			switch (opCode)
			{
				case ScalarFunction::OpLdCnt:
				{
					op( "arg", (int)m_func->getIndexOperand( ip));
					break;
				}
				case ScalarFunction::OpPush:
				{
					std::size_t validx = m_func->getIndexOperand( ip, m_valuear.size());
					double value = m_valuear[ validx];
					op( "arg", value);
					break;
				}
				case ScalarFunction::OpArg:
				{
					op( "arg", (int)m_func->getIndexOperand( ip));
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
	CATCH_ERROR_MAP_RETURN( _TXT("error building string representation of scalar function: %s"), *m_errorhnd, std::string());
}
