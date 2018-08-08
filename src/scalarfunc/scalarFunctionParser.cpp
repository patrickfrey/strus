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
#include "strus/base/snprintf.h"
#include "strus/base/local_ptr.hpp"
#include "strus/base/bitOperations.hpp"
#include "scalarFunctionLinearComb.hpp"
#include <limits>
#include <cmath>

using namespace strus;
#undef STRUS_LOWLEVEL_DEBUG

static double if_greater( unsigned int nofargs, const double* args)
{
	return (args[0] > args[1])?args[2]:args[3];
}

static double if_greaterequal( unsigned int nofargs, const double* args)
{
	return (args[0] >= args[1])?args[2]:args[3];
}

static double if_smaller( unsigned int nofargs, const double* args)
{
	return (args[0] < args[1])?args[2]:args[3];
}

static double if_smallerequal( unsigned int nofargs, const double* args)
{
	return (args[0] <= args[1])?args[2]:args[3];
}

static double if_equal( unsigned int nofargs, const double* args)
{
	double diff = args[0] - args[1];
	return (diff * diff <= std::numeric_limits<double>::epsilon())?args[2]:args[3];
}

static double log_base( double x, double base)
{
	return log(x) / log(base);
}

static double square( double x)
{
	return x*x;
}

static unsigned int doubleToUint( double x)
{
	return (unsigned int)std::floor( std::fabs( x) + std::numeric_limits<float>::epsilon());
}

static double bit_xor( double x, double y)
{
	return doubleToUint(x)^doubleToUint(y);
}
static double bit_and( double x, double y)
{
	return doubleToUint(x)&doubleToUint(y);
}
static double bit_or( double x, double y)
{
	return doubleToUint(x)|doubleToUint(y);
}
static double bit_cnt( double x)
{
	return BitOperations::bitCount(doubleToUint(x));
}

ScalarFunctionParser::ScalarFunctionParser( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_)
{
	defineUnaryFunction( "fabs", &std::fabs);
	defineUnaryFunction( "floor", &std::floor);
	defineUnaryFunction( "ceil", &std::ceil);
	defineUnaryFunction( "log", &std::log10);
	defineBinaryFunction( "log", &log_base);
	defineUnaryFunction( "ln", &std::log);
	defineUnaryFunction( "exp", &std::exp);
	defineUnaryFunction( "sqrt", &std::sqrt);
	defineUnaryFunction( "sqr", &square);
	defineBinaryFunction( "pow", &std::pow);
	defineBinaryFunction( "mod", &std::fmod);
	defineBinaryFunction( "xor", &bit_xor);
	defineBinaryFunction( "and", &bit_and);
	defineBinaryFunction( "or", &bit_or);
	defineUnaryFunction( "sin", &std::sin);
	defineUnaryFunction( "cos", &std::cos);
	defineUnaryFunction( "tan", &std::tan);
	defineUnaryFunction( "sinh", &std::sinh);
	defineUnaryFunction( "cosh", &std::cosh);
	defineUnaryFunction( "tanh", &std::tanh);
	defineUnaryFunction( "asin", &std::asin);
	defineUnaryFunction( "acos", &std::acos);
	defineUnaryFunction( "atan", &std::atan);
	defineUnaryFunction( "bcnt", &bit_cnt);
	defineNaryFunction( "if_gt", &if_greater, 4, 4);
	defineNaryFunction( "if_ge", &if_greaterequal, 4, 4);
	defineNaryFunction( "if_lt", &if_smaller, 4, 4);
	defineNaryFunction( "if_le", &if_smallerequal, 4, 4);
	defineNaryFunction( "if_eq", &if_equal, 4, 4);
}

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
	for (;si < se && isAlnum(*si); ++si) rt.push_back( *si);
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
		throw std::runtime_error( _TXT("number expected"));
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

void ScalarFunctionParser::parseOperand( ScalarFunction* func, ParserContext* ctx, std::string::const_iterator& si, const std::string::const_iterator& se) const
{
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "enter parseOperand [" << std::string(si,se) << "]" << std::endl;
#endif
	skipSpaces( si, se);
	if (si == se)
	{
		throw strus::runtime_error( "%s",  _TXT( "unexpected end of expression, operand expected"));
	}
	{
		std::string::const_iterator start = si;
		if (isAlpha(*si))
		{
			// ... check if it is a function call and step back to 'start' if not
			std::string funcname = parseIdentifier( si, se);
			skipSpaces( si, se);
			if (*si == '(')
			{
				++si;
				parseFunctionCall( func, ctx, funcname, si, se);
				return;
			}
			else
			{
				si = start;
			}
		}
	}
	if (*si == '_')
	{
		if (ctx->nofNamedArguments > 0)
		{
			// ... when we have named arguments specified, we do not allow argument references by index:
			std::string var = parseIdentifier( si, se);
			if (ctx->argumentNameMap.find( var) == ctx->argumentNameMap.end())
			{
				throw strus::runtime_error( "%s",  _TXT("argument references by index not allowed if named arguments specified"));
			}
			resolveIdentifier( func, ctx, var);
			return;
		}
		// parse argument index:
		std::string::const_iterator start = si;
		++si;
		if (si == se || !isDigit(*si))
		{
			// ... no argument index, so it is a varible:
			si = start;
			std::string var = parseIdentifier( si, se);
			resolveIdentifier( func, ctx, var);
		}
		else
		{
			// get the argument index:
			unsigned int argid = (unsigned char)(*si - '0');
			for (++si; si != se && isDigit(*si); ++si)
			{
				argid = argid * 10 + (unsigned char)(*si - '0');
				if (argid > std::numeric_limits<unsigned short>::max())
				{
					throw strus::runtime_error( "%s",  _TXT("argument identifier out of range"));
				}
			}
			if (si != se && isAlpha(*si))
			{
				// ... no argument index, so it is a varible:
				si = start;
				std::string var = parseIdentifier( si, se);
				resolveIdentifier( func, ctx, var);
			}
			else
			{
				func->addOpPushArgument( argid);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "parse argument id " << argid << std::endl;
#endif
			}
		}
	}
	else if (isAlpha(*si))
	{
		// ... variable identifier
		std::string var = parseIdentifier( si, se);
		resolveIdentifier( func, ctx, var);
	}
	else if (*si == '+' || *si == '-')
	{
		std::string::const_iterator start = si;
		++si;
		skipSpaces( si, se);
		if (si == se) throw strus::runtime_error( "%s",  _TXT("unexpected end of expression"));
		if (isDigit(*si))
		{
			si = start;
			double val = parseNumber( si, se);
			func->addOpPushConstant( val);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "parse number " << val << std::endl;
#endif
		}
		else
		{
			parseOperand( func, ctx, si, se);
			if (*start == '-')
			{
				func->addOp( ScalarFunction::OpNeg);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "parse negation" << std::endl;
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
		std::cerr << "parse number " << val << std::endl;
#endif
	}
	else if (*si == '(')
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "start subexpression" << std::endl;
#endif
		// ... sub expression
		++si;
		parseExpression( func, ctx, 0, si, se);
		skipSpaces( si, se);
		if (si == se || *si != ')')
		{
			throw strus::runtime_error( "%s",  _TXT("missing end brakced ')' at end of expression"));
		}
		++si;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "end subexpression" << std::endl;
#endif
	}
	else
	{
		throw strus::runtime_error( _TXT( "unexpected char '%c' in expression"), *si);
	}
}

void ScalarFunctionParser::resolveIdentifier( ScalarFunction* func, ParserContext* ctx, const std::string& var) const
{
	ArgumentNameMap::const_iterator ai = ctx->argumentNameMap.find( var);
	if (ai == ctx->argumentNameMap.end())
	{
		func->addOpPushVariable( var);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "parse variable " << var << std::endl;
#endif
	}
	else
	{
		func->addOpPushArgument( ai->second);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "parse argument id " << ai->second << std::endl;
#endif
	}
}

void ScalarFunctionParser::resolveFunctionCall( ScalarFunction* func, const std::string& functionName, unsigned int nofArguments) const
{
	const char* errmsg = 0;
	UnaryFunctionMap::const_iterator ufi = m_unaryFunctionMap.find( functionName);
	if (ufi != m_unaryFunctionMap.end())
	{
		if (nofArguments > 1) errmsg = _TXT("too many arguments in unary function call");
		else if (nofArguments < 1) throw strus::runtime_error( "%s",  _TXT("too few arguments in unary function call"));
		else
		{
			func->addOpUnaryFunctionCall( ufi->second);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "parse unary function call " << functionName << " " << nofArguments << std::endl;
#endif
			return;
		}
	}
	BinaryFunctionMap::const_iterator bfi = m_binaryFunctionMap.find( functionName);
	if (bfi != m_binaryFunctionMap.end())
	{
		if (nofArguments > 2) errmsg = _TXT("too many arguments in binary function call");
		else if (nofArguments < 2) throw strus::runtime_error( "%s",  _TXT("too few arguments in binary function call"));
		else
		{
			func->addOpBinaryFunctionCall( bfi->second);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "parse binary function call " << functionName << " " << nofArguments << std::endl;
#endif
			return;
		}
	}
	NaryFunctionMap::const_iterator nfi = m_naryFunctionMap.find( functionName);
	if (nfi != m_naryFunctionMap.end())
	{
		if (nofArguments > nfi->second.max_nofargs) throw strus::runtime_error( "%s",  _TXT("too many arguments in N-ary function call"));
		else if (nofArguments < nfi->second.min_nofargs) throw strus::runtime_error( "%s",  _TXT("too few arguments in N-ary function call"));
		func->addOpNaryFunctionCall( nofArguments, nfi->second.func);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "parse N-ary function call " << functionName << " " << nofArguments << std::endl;
#endif
		return;
	}
	if (errmsg)
	{
		throw strus::runtime_error( "%s", errmsg);
	}
	else
	{
		throw strus::runtime_error( _TXT("call of unknown function '%s' with %u arguments in scalar function expression"), functionName.c_str(), nofArguments);
	}
}

void ScalarFunctionParser::parseFunctionCall( ScalarFunction* func, ParserContext* ctx, const std::string& functionName, std::string::const_iterator& si, const std::string::const_iterator& se) const
{
	unsigned int nofArguments = 0;
	for(;;)
	{
		skipSpaces( si, se);
		if (si == se )
		{
			throw strus::runtime_error( "%s",  _TXT( "unexpected end of expression"));
		}
		if (*si == ')')
		{
			++si;
			break;
		}
		parseExpression( func, ctx, 0, si, se);
		skipSpaces( si, se);
		++nofArguments;
		if (si == se)
		{
			throw strus::runtime_error( "%s",  _TXT( "unexpected end of expression"));
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
			throw strus::runtime_error( "%s",  _TXT( "end bracket ')' or argument separator ',' expected after argument in function call"));
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
			throw strus::runtime_error( "%s",  _TXT( "operator expected in expression after operand"));
	}
}

void ScalarFunctionParser::parseExpression( ScalarFunction* func, ParserContext* ctx, unsigned int oprPrecedenceParent, std::string::const_iterator& si, const std::string::const_iterator& se) const
{
	skipSpaces( si, se);
	parseOperand( func, ctx, si, se);
	skipSpaces( si, se);
	if (si == se)
	{
		return;
	}
	unsigned int oprPrecedence = getOperatorPrecedence( *si);
	while (oprPrecedence > oprPrecedenceParent)
	{
		ScalarFunction::OpCode opCode = getOperator( *si++);
		parseExpression( func, ctx, oprPrecedence, si, se);
		func->addOp( opCode);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "parse operator " << ScalarFunction::opCodeName( opCode) << std::endl;
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
		m_unaryFunctionMap[ name] = func;
	}
	CATCH_ERROR_MAP( _TXT("error defining unary function (scalar function parser): %s"), *m_errorhnd);
}

void ScalarFunctionParser::defineBinaryFunction( const std::string& name, BinaryFunction func)
{
	try
	{
		m_binaryFunctionMap[ name] = func;
	}
	CATCH_ERROR_MAP( _TXT("error defining binary function (scalar function parser): %s"), *m_errorhnd);
}

void ScalarFunctionParser::defineNaryFunction( const std::string& name, NaryFunction func, unsigned int min_nofargs, unsigned int max_nofargs)
{
	try
	{
		m_naryFunctionMap[ name] = NaryFunctionDef( min_nofargs, max_nofargs, func);
	}
	CATCH_ERROR_MAP( _TXT("error defining N-ary function (scalar function parser): %s"), *m_errorhnd);
}

ScalarFunctionInterface*
	ScalarFunctionParser::createFunction(
		const std::string& src,
		const std::vector<std::string>& argumentNames) const
{
	ParserContext ctx;
	std::vector<std::string>::const_iterator ai = argumentNames.begin(), ae = argumentNames.end();
	for (unsigned int aidx=0; ai != ae; ++ai,++aidx)
	{
		ctx.argumentNameMap[ *ai] = aidx;
	}
	ctx.nofNamedArguments = argumentNames.size();

	std::string::const_iterator si = src.begin(), se = src.end();
	try
	{
		strus::local_ptr<ScalarFunction> func( new ScalarFunction( m_errorhnd));
		ScalarFunction* rt = func.get();
		try
		{
			parseExpression( rt, &ctx, 0/*precedence*/, si, se);
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

#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "loaded function:" << std::endl;
		std::cerr << rt->tostring() << std::endl;
#endif
#ifdef DISABLED_BECAUSE_OF_ISSUE_94
		// Special treatment of linear combination:
		std::vector<double> linearcomb_factors;
		if (func->isLinearComb( linearcomb_factors))
		{
			return createScalarFunction_linearcomb( linearcomb_factors, m_errorhnd);
		}
		else
		{
			// Return built function to caller:
			func.release();
			return rt;
		}
#else
		// Return built function to caller:
		func.release();
		return rt;
#endif
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error parsing and creating scalar function: %s"), *m_errorhnd, 0);
}

const char* ScalarFunctionParser::getDescription() const
{
	return _TXT("parser for arithmetic expressions on double precision floating point values with operators *,-,*,/ "
			"and unary and binary functions as defined in the C math library. "
			"External arguments are adressed with decimal numbers with a preceding '_', e.g. _0 for the first argument");
}

