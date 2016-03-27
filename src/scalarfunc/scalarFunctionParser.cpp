/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Default implementation of the strus scalar function parser
/// \file scalarFunctionParser.cpp
#include "scalarFunctionParser.hpp"
#include "scalarFunction.hpp"
#include "private/utils.hpp"
#include "strus/private/snprintf.h"
#include <limits>

using namespace strus;
#define STRUS_LOWLEVEL_DEBUG

static bool isAlpha( char ch)
{
	return ((ch|32) >= 'a' && (ch|32) <= 'z');
}

static bool isDigit( char ch)
{
	return ((ch) >= '0' && (ch) <= '9');
}

static bool isAlnum( char ch)
{
	return isAlpha(ch)|isDigit(ch)|(ch == '_');
}

static bool isSpace( char ch)
{
	return ((unsigned char)(ch) <= 32);
}

static unsigned int getOperatorPrecedence( char op)
{
	if (op == '*' || op == '/') return 2;
	if (op == '+' || op == '-') return 1;
	return 0;
}

static void skipSpaces( std::string::const_iterator& si, const std::string::const_iterator& se)
{
	for (;si < se && isSpace(*si); ++si){}
}

static std::string parseIdentifier( std::string::const_iterator& si, const std::string::const_iterator& se)
{
	std::string rt;
	for (;si < se && isAlnum(*si); ++si) rt.push_back( ::tolower(*si));
	skipSpaces(si,se);
	return rt;
}

static double parseNumber( std::string::const_iterator& si, const std::string::const_iterator& se)
{
	double rt = 0.0;
	bool sig = false;
	if (si < se && (*si == '-' || *si == '+'))
	{
		sig = (*si == '-');
		++si;
	}
	if (!isDigit(*si))
	{
		throw strus::runtime_error(_TXT("number expected"));
	}
	for (;si < se && isDigit(*si); ++si)
	{
		rt *= 10;
		rt += *si - '0';
	}
	if (si < se && *si == '.')
	{
		double frac = 1.0;
		for (++si; si < se && isDigit(*si); ++si)
		{
			frac /= 10;
			rt += (*si - '0') * frac;
		}
	}
	skipSpaces(si,se);
	return sig?-rt:rt;
}

void ScalarFunctionParser::parseOperand( ScalarFunction* func, std::string::const_iterator& si, const std::string::const_iterator& se) const
{
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "enter parseOperand [" << std::string(si,se) << "]" << std::endl;
#endif
	skipSpaces( si, se);
	if (si == se)
	{
		throw strus::runtime_error( _TXT( "unexpected end of expression, operand expected"));
	}
	std::string::const_iterator start = si;
	if (isAlpha(*si))
	{
		// ... check if it is a function call and step back to 'start' if not
		std::string funcname = parseIdentifier( si, se);
		skipSpaces( si, se);
		if (*si == '(')
		{
			++si;
			parseFunctionCall( func, funcname, si, se);
			return;
		}
		else
		{
			si = start;
		}
	}
	if (*si == '_')
	{
		// ... argument index
		std::string::const_iterator start = si;
		++si;
		if (si == se || !isDigit(*si))
		{
			si = start;
			std::string var = parseIdentifier( si, se);
			func->addOpPushVariable( var);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "parse variable " << var << std::endl;
#endif
		}
		else
		{
			unsigned int argid = (unsigned char)(*si - '0');
			for (++si; si != se && isDigit(*si); ++si)
			{
				argid = argid * 10 + (unsigned char)(*si - '0');
				if (argid > std::numeric_limits<unsigned short>::max())
				{
					throw strus::runtime_error( _TXT("argument identifier out of range"));
				}
			}
			if (!argid)
			{
				throw strus::runtime_error( _TXT("argument indices are starting with _1 and not with _0"));
			}
			if (si != se && isAlpha(*si))
			{
				si = start;
				std::string var = parseIdentifier( si, se);
				func->addOpPushVariable( var);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "parse variable " << var << std::endl;
#endif
			}
			else
			{
				func->addOpPushArgument( argid-1);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "parse argument id " << (argid-1) << std::endl;
#endif
			}
		}
	}
	else if (isAlpha(*si))
	{
		// ... variable identifier
		std::string var = parseIdentifier( si, se);
		func->addOpPushVariable( var);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "parse variable " << var << std::endl;
#endif
	}
	else if (*si == '+' || *si == '-')
	{
		std::string::const_iterator start = si;
		++si;
		skipSpaces( si, se);
		if (si == se) throw strus::runtime_error( _TXT("unexpected end of expression"));
		if (isDigit(*si))
		{
			si = start;
			double val = parseNumber( si, se);
			func->addOpPushConstant( val);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "parse number " << val << std::endl;
#endif
		}
		else
		{
			parseOperand( func, si, se);
			if (*start == '-')
			{
				func->addOp( ScalarFunction::OpNeg);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "parse negation" << std::endl;
#endif
			}
		}
	}
	else if (isDigit(*si))
	{
		// ... numeric value
		double val = parseNumber( si, se);
		func->addOpPushConstant( val);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "parse number " << val << std::endl;
#endif
	}
	else if (*si == '(')
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "start subexpression" << std::endl;
#endif
		// ... sub expression
		++si;
		parseExpression( func, 0, si, se);
		skipSpaces( si, se);
		if (si == se || *si != ')')
		{
			throw strus::runtime_error( _TXT("missing end brakced ')' at end of expression"));
		}
		++si;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "end subexpression" << std::endl;
#endif
	}
	else
	{
		throw strus::runtime_error( _TXT( "unexpected char '%c' in expression"), *si);
	}
}

void ScalarFunctionParser::resolveFunctionCall( ScalarFunction* func, const std::string& functionName, std::size_t nofArguments) const
{
	const char* errmsg = 0;
	UnaryFunctionMap::const_iterator ufi = m_unaryFunctionMap.find( functionName);
	if (ufi != m_unaryFunctionMap.end())
	{
		if (nofArguments > 1) errmsg = _TXT("too many arguments in unary function call");
		else if (nofArguments < 1) throw strus::runtime_error( _TXT("too few arguments in unary function call"));
		else
		{
			func->addOpUnaryFunctionCall( ufi->second);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "parse unary function call " << functionName << " " << nofArguments << std::endl;
#endif
			return;
		}
	}
	BinaryFunctionMap::const_iterator bfi = m_binaryFunctionMap.find( functionName);
	if (bfi != m_binaryFunctionMap.end())
	{
		if (nofArguments > 2) errmsg = _TXT("too many arguments in binary function call");
		else if (nofArguments < 2) throw strus::runtime_error( _TXT("too few arguments in binary function call"));
		else
		{
			func->addOpBinaryFunctionCall( bfi->second);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "parse binary function call " << functionName << " " << nofArguments << std::endl;
#endif
			return;
		}
	}
	NaryFunctionMap::const_iterator nfi = m_naryFunctionMap.find( functionName);
	if (nfi != m_naryFunctionMap.end())
	{
		if (nofArguments > nfi->second.max_nofargs) throw strus::runtime_error( _TXT("too many arguments in N-ary function call"));
		else if (nofArguments < nfi->second.min_nofargs) throw strus::runtime_error( _TXT("too few arguments in N-ary function call"));
		func->addOpNaryFunctionCall( nofArguments, nfi->second.func);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "parse N-ary function call " << functionName << " " << nofArguments << std::endl;
#endif
		return;
	}
	if (errmsg)
	{
		throw strus::runtime_error( errmsg);
	}
	else
	{
		throw strus::runtime_error( _TXT("call of unknown function in scalar function expression"));
	}
}

void ScalarFunctionParser::parseFunctionCall( ScalarFunction* func, const std::string& functionName, std::string::const_iterator& si, const std::string::const_iterator& se) const
{
	std::size_t nofArguments = 0;
	for(;;)
	{
		skipSpaces( si, se);
		if (si == se )
		{
			throw strus::runtime_error( _TXT( "unexpected end of expression"));
		}
		if (*si == ')')
		{
			++si;
			break;
		}
		parseExpression( func, 0, si, se);
		skipSpaces( si, se);
		++nofArguments;
		if (si == se)
		{
			throw strus::runtime_error( _TXT( "unexpected end of expression"));
		}
		if (*si == ',')
		{
			++si;
		}
		else if (*si == ')')
		{
			++si;
			break;
		}
		else
		{
			throw strus::runtime_error( _TXT( "end bracket ')' or argument separator ',' expected after argument in function call"));
		}
	}
	resolveFunctionCall( func, functionName, nofArguments);
}

static ScalarFunction::OpCode getOperator( const char chr)
{
	switch (chr)
	{
		case '+':
			return ScalarFunction::OpAdd;
		case '-':
			return ScalarFunction::OpSub;
		case '*':
			return ScalarFunction::OpMul;
		case '/':
			return ScalarFunction::OpDiv;
		default:
			throw strus::runtime_error( _TXT( "operator expected in expression after operand"));
	}
}

void ScalarFunctionParser::parseExpression( ScalarFunction* func, unsigned int oprPrecedenceParent, std::string::const_iterator& si, const std::string::const_iterator& se) const
{
	skipSpaces( si, se);
	parseOperand( func, si, se);
	skipSpaces( si, se);
	if (si == se)
	{
		return;
	}
	unsigned int oprPrecedence = getOperatorPrecedence( *si);
	while (oprPrecedence > oprPrecedenceParent)
	{
		ScalarFunction::OpCode opCode = getOperator( *si++);
		parseExpression( func, oprPrecedence, si, se);
		func->addOp( opCode);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "parse operator " << ScalarFunction::opCodeName( opCode) << std::endl;
#endif
		skipSpaces( si, se);
		if (si == se)
		{
			return;
		}
		oprPrecedence = getOperatorPrecedence( *si);
	}
}

void ScalarFunctionParser::defineUnaryFunction( const std::string& name, UnaryFunction func)
{
	try
	{
		m_unaryFunctionMap[ strus::utils::tolower(name)] = func;
	}
	CATCH_ERROR_MAP( _TXT("error defining unary function (scalar function parser): %s"), *m_errorhnd);
}

void ScalarFunctionParser::defineBinaryFunction( const std::string& name, BinaryFunction func)
{
	try
	{
		m_binaryFunctionMap[ strus::utils::tolower(name)] = func;
	}
	CATCH_ERROR_MAP( _TXT("error defining binary function (scalar function parser): %s"), *m_errorhnd);
}

void ScalarFunctionParser::defineNaryFunction( const std::string& name, NaryFunction func, std::size_t min_nofargs, std::size_t max_nofargs)
{
	try
	{
		m_naryFunctionMap[ strus::utils::tolower(name)] = NaryFunctionDef( min_nofargs, max_nofargs, func);
	}
	CATCH_ERROR_MAP( _TXT("error defining N-ary function (scalar function parser): %s"), *m_errorhnd);
}

ScalarFunctionInterface* ScalarFunctionParser::createFunction( const std::string& src) const
{
	std::string::const_iterator si = src.begin(), se = src.end();
	try
	{
		std::auto_ptr<ScalarFunction> func( new ScalarFunction( m_errorhnd));
		ScalarFunction* rt = func.get();
		try
		{
			parseExpression( rt, 0/*precedence*/, si, se);
		}
		catch (const std::runtime_error& err)
		{
			char msgbuf[ 512];
			strus_snprintf( msgbuf, sizeof(msgbuf), _TXT(" at byte position %u in expression [%s]"),
					(unsigned int)(si - src.begin()), src.c_str());
			throw std::runtime_error( std::string(err.what()) + msgbuf);
		}
		skipSpaces(si,se);
		if (si != se)
		{
			std::string expr( si, se);
			throw strus::runtime_error( _TXT( "unexpected characters at end of expression: '%s'"), expr.c_str());
		}
		func.release();
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error parsing and creating scalar function: %s"), *m_errorhnd, 0);
}


