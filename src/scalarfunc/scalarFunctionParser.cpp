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
#include "strus/private/snprintf.hpp"
#include <limits>

using namespace strus;

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

static unsigned int operatorPrecedence( char op)
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

void ScalarFunctionParser::parseOperand( ScalarFunction* func, unsigned int oprPrecedence, std::string::const_iterator& si, const std::string::const_iterator& se) const
{
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
			if (si != se && isAlpha(*si))
			{
				si = start;
				std::string var = parseIdentifier( si, se);
				func->addOpPushVariable( var);
			}
			else
			{
				func->addOpPushArgument( argid);
			}
		}
	}
	else if (isAlpha(*si))
	{
		// ... variable identifier
		std::string var = parseIdentifier( si, se);
		func->addOpPushVariable( var);
	}
	else if (*si == '+' || *si == '-' || isDigit(*si))
	{
		// ... numeric value
		double val = parseNumber( si, se);
		func->addOpPushConstant( val);
	}
	else if (*si == '(')
	{
		// ... sub expression
		++si;
		parseExpression( func, 0, si, se);
		skipSpaces( si, se);
		if (si == se || *si != ')')
		{
			throw strus::runtime_error( _TXT("missing end brakced ')' at end of expression"));
		}
	}
	else
	{
		throw strus::runtime_error( _TXT("unexpected token in expression"));
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
		parseOperand( func, si, se);
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
	NaryFunctionMap::const_iterator nfi = m_naryFunctionMap.find( functionName);
	if (nfi != m_naryFunctionMap.end())
	{
		func->addOpNaryFunctionCall( nofArguments, nfi->second);
		return;
	}
	BinaryFunctionMap::const_iterator bfi = m_binaryFunctionMap.find( functionName);
	if (bfi != m_binaryFunctionMap.end())
	{
		if (nofArguments > 2) throw strus::runtime_error( _TXT("too many arguments in binary function call"));
		if (nofArguments < 2) throw strus::runtime_error( _TXT("too few arguments in binary function call"));
		func->addOpBinaryFunctionCall( bfi->second);
		return;
	}
	UnaryFunctionMap::const_iterator ufi = m_unaryFunctionMap.find( functionName);
	if (ufi != m_unaryFunctionMap.end())
	{
		if (nofArguments > 1) throw strus::runtime_error( _TXT("too many arguments in unary function call"));
		if (nofArguments < 1) throw strus::runtime_error( _TXT("too few arguments in unary function call"));
		func->addOpUnaryFunctionCall( ufi->second);
		return;
	}
	throw strus::runtime_error( _TXT("call of unknown function in scalar function expression"));
}

static ScalarFunction::OpCode getOperator( std::string::const_iterator& si, const std::string::const_iterator& se)
{
	switch (*si)
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

void ScalarFunctionParser::parseExpression( ScalarFunction* func, unsigned int oprPrecedence, std::string::const_iterator& si, const std::string::const_iterator& se) const
{
	skipSpaces( si, se);
	parseOperand( func, si, se);
	skipSpaces( si, se);
	if (si == se)
	{
		return;
	}
	ScalarFunction::OpCode opCode = getOperator( si, se);
	unsigned int oprPrecedence2 = operatorPrecedence( *si);
	while (oprPrecedence2 > oprPrecedence)
	{
		parseExpression( func, oprPrecedence2, si, se);
		func->addOp( opCode);

		skipSpaces( si, se);
		if (si == se)
		{
			return;
		}
		opCode = getOperator( si, se);
		oprPrecedence2 = operatorPrecedence( *si);
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

void ScalarFunctionParser::defineNaryFunction( const std::string& name, NaryFunction func)
{
	try
	{
		m_naryFunctionMap[ strus::utils::tolower(name)] = func;
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
			throw strus::runtime_error( _TXT( "unexpected characters at end of expression"));
		}
		func.release();
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error parsing and creating scalar function: %s"), *m_errorhnd, 0);
}


