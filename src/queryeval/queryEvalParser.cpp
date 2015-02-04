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
#ifndef _STRUS_QUERY_PROGRAM_PARSER_HPP_INCLUDED
#define _STRUS_QUERY_PROGRAM_PARSER_HPP_INCLUDED
#include "queryEvalParser.hpp"
#include "weightingFunctionDef.hpp"
#include "summarizerDef.hpp"
#include "termDef.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "lexems.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/algorithm/string.hpp>

using namespace strus;
using namespace strus::parser;

#include "mapFunctionParameters.hpp"

static std::string errorPosition( const char* base, const char* itr)
{
	unsigned int line = 1;
	unsigned int col = 1;
	std::ostringstream msg;

	for (unsigned int ii=0,nn=itr-base; ii < nn; ++ii)
	{
		if (base[ii] == '\n')
		{
			col = 1;
			++line;
		}
		else
		{
			++col;
		}
	}
	msg << "at line " << line << " column " << col;
	return msg.str();
}

static std::vector<ArithmeticVariant> parseParameters( const char** paramNames, char const*& src)
{
	KeyMap<ArithmeticVariant> paramDefs;
	while (*src)
	{
		if (!isAlpha( *src))
		{
			throw std::runtime_error( "list of comma separated identifier value assignments expected as function parameters");
		}
		std::string paramName = boost::algorithm::to_lower_copy( parse_IDENTIFIER( src));
		if (!isAssign( *src))
		{
			throw std::runtime_error( "equal '=' expected after function parameter name");
		}
		parse_OPERATOR(src);
		if (!isDigit( *src) && !isMinus(*src))
		{
			throw std::runtime_error( "numeric value expected as function argument value");
		}
		bool isNegative = false;
		char const* cc = src;
		if (isMinus(*src))
		{
			isNegative = true;
			++cc;
		}
		if (!isDigit(*cc))
		{
			throw std::runtime_error("numeric scalar value expected as function argument value");
		}
		for (; *cc && isDigit( *cc); ++cc){}
		if (*cc == '.')
		{
			paramDefs[ paramName] = parse_FLOAT( src);
		}
		else if (isNegative)
		{
			paramDefs[ paramName] = parse_INTEGER( src);
		}
		else
		{
			paramDefs[ paramName] = parse_UNSIGNED( src);
		}
		if (!isComma(*src))
		{
			break;
		}
		parse_OPERATOR(src);
	}
	return mapFunctionParameters( paramNames, paramDefs);
}

static std::vector<std::string> parseIdentifierList( char const*& src)
{
	std::vector<std::string> rt;
	for (;;)
	{
		if (!isAlnum( *src))
		{
			throw std::runtime_error( "feature set identifier expected");
		}
		rt.push_back( boost::algorithm::to_lower_copy( parse_IDENTIFIER( src)));
		if (isComma( *src))
		{
			parse_OPERATOR(src);
		}
		else
		{
			break;
		}
	}
	return rt;
}


TermDef QueryEvalParser::parseTermDef( char const*& src) const
{
	if (isAlpha(*src))
	{
		std::string termset = boost::algorithm::to_lower_copy( parse_IDENTIFIER( src));
		std::string termvalue;
		std::string termtype;

		if (isStringQuote( *src))
		{
			termvalue = parse_STRING( src);
		}
		else if (isAlpha( *src))
		{
			termvalue = parse_IDENTIFIER( src);
		}
		else
		{
			throw std::runtime_error( "term value (string,identifier,number) after the feature group identifier");
		}
		if (!isColon( *src))
		{
			throw std::runtime_error( "colon (':') expected after term value");
		}
		parse_OPERATOR(src);
		if (!isAlpha( *src))
		{
			throw std::runtime_error( "term type identifier expected after colon and term value");
		}
		termtype = boost::algorithm::to_lower_copy( parse_IDENTIFIER( src));
		return TermDef( termset, termtype, termvalue);
	}
	else
	{
		throw std::runtime_error( "feature set identifier expected as start of a term declaration in the query");
	}
}

WeightingFunctionDef QueryEvalParser::parseWeightingFunctionDef( char const*& src) const
{
	WeightingFunctionDef rt;
	if (!isAlpha( *src))
	{
		throw std::runtime_error( "weighting function identifier expected");
	}
	rt.functionName = boost::algorithm::to_lower_copy( parse_IDENTIFIER( src));
	rt.function = m_processor->getWeightingFunction( rt.functionName);
	for (;;)
	{
		if (isOpenSquareBracket( *src))
		{
			if (!rt.selectorSets.empty())
			{
				throw std::runtime_error("duplicate definion of selected features in weighting function definition");
			}
			parse_OPERATOR(src);
			rt.selectorSets = parseIdentifierList( src);
			if (!isCloseSquareBracket( *src))
			{
				throw std::runtime_error( "comma ',' or close oval bracket ']' expected as separator or terminator of the selected feature set declaration for the weighting function");
			}
			parse_OPERATOR(src);
		}
		else if (isOpenAngleBracket(*src))
		{
			if (!rt.parameters.empty())
			{
				throw std::runtime_error("duplicate definion of arguments in weighting function definition");
			}
			parse_OPERATOR(src);
			rt.parameters
				= parseParameters( rt.function->parameterNames(), src);
			if (!isCloseAngleBracket(*src))
			{
				throw std::runtime_error( "expected comma ',' as separator or close angle bracket '>' to close parameter list of weighting function");
			}
			parse_OPERATOR(src);
		}
		else if (isOpenOvalBracket( *src))
		{
			if (!rt.weightingSets.empty())
			{
				throw std::runtime_error("duplicate definion of the weighted features in the weighting function definition");
			}
			parse_OPERATOR(src);
			rt.weightingSets = parseIdentifierList( src);
			if (!isCloseOvalBracket( *src))
			{
				throw std::runtime_error( "comma ',' or close oval bracket ')' expected as separator or terminator of the feature set declaration for the weighting function");
			}
			parse_OPERATOR(src);
		}
		else
		{
			break;
		}
	}
	if (rt.selectorSets.empty())
	{
		rt.selectorSets = rt.weightingSets;
	}
	return rt;
}


SummarizerDef QueryEvalParser::parseSummarizerDef( char const*& src) const
{
	const SummarizerFunctionInterface* function;
	std::string functionName;
	std::vector<ArithmeticVariant> parameters;
	std::string resultAttribute;
	std::string contentType;
	std::string structSet;
	std::vector<std::string> featureSet;

	if (!isAlpha( *src))
	{
		throw std::runtime_error( "name of result attribute expected after SUMMARIZE");
	}
	resultAttribute = boost::algorithm::to_lower_copy( parse_IDENTIFIER( src));
	if (!isAssign(*src))
	{
		throw std::runtime_error( "assignment operator '=' expected after the name of result attribute in summarizer definition");
	}
	parse_OPERATOR( src);
	if (!isAlpha( *src))
	{
		throw std::runtime_error( "name of summarizer expected after assignment in summarizer definition");
	}
	functionName = boost::algorithm::to_lower_copy( parse_IDENTIFIER( src));
	function = m_processor->getSummarizerFunction( functionName);

	for (;;)
	{
		if (isOpenSquareBracket( *src))
		{
			if (!contentType.empty())
			{
				throw std::runtime_error("duplicate definion of feature selected in summarizer definition");
			}
			parse_OPERATOR( src);
			contentType = boost::algorithm::to_lower_copy( parse_IDENTIFIER( src));
			if (!isCloseSquareBracket( *src))
			{
				throw std::runtime_error( "expected only one identifier inside in square brackets '[' ']' in summarizer definition");
			}
			parse_OPERATOR( src);
		}
		else if (isOpenAngleBracket( *src))
		{
			if (!parameters.empty())
			{
				throw std::runtime_error("duplicate definion of function arguments in summarizer definition");
			}
			parse_OPERATOR( src);
			parameters = parseParameters( function->parameterNames(), src);
			if (!isCloseAngleBracket( *src))
			{
				throw std::runtime_error( "expected comma ',' as separator or close angle bracket '>' to close parameter list of summarize function");
			}
			parse_OPERATOR( src);
		}
		else if (isOpenOvalBracket( *src))
		{
			if (!featureSet.empty())
			{
				throw std::runtime_error("duplicate definion of matching features in summarizer definition");
			}
			parse_OPERATOR( src);
			char const* cc = src;
			structSet = boost::algorithm::to_lower_copy( parse_IDENTIFIER( src));
			if (isColon( *src))
			{
				parse_OPERATOR( src);
			}
			else
			{
				src = cc;
				structSet.clear();
			}
			featureSet = parseIdentifierList( src);
			if (!isCloseOvalBracket( *src))
			{
				throw std::runtime_error("comma ',' as separator or close oval bracket ')' expected as separator in list of match features in a summarizer definition");
			}
			parse_OPERATOR( src);
		}
		else
		{
			break;
		}
	}
	return SummarizerDef( function, functionName, parameters, resultAttribute,
				contentType, structSet, featureSet);
}

void QueryEvalParser::loadProgram( QueryEval& qeprg, const std::string& source) const
{
	char const* src = source.c_str();
	enum StatementKeyword {e_EVAL, e_TERM, e_SUMMARIZE};
	std::string id;

	skipSpaces( src);
	try
	{
		while (*src)
		{
			switch ((StatementKeyword)parse_KEYWORD( src, 3, "EVAL", "TERM", "SUMMARIZE"))
			{
				case e_TERM:
					parseTermDef( src);
					break;
				case e_EVAL:
					parseWeightingFunctionDef( src);
					break;
				case e_SUMMARIZE:
					parseSummarizeDef( src);
					break;
			}
			if (*src)
			{
				if (!isSemiColon(*src))
				{
					throw std::runtime_error("semicolon expected as delimiter of query eval program instructions");
				}
				parse_OPERATOR( src);
			}
		}
	}
	catch (const std::runtime_error& e)
	{
		throw std::runtime_error(
			std::string( "error in query evaluation program ")
			+ errorPosition( source.c_str(), src)
			+ ":" + e.what());
	}
}

