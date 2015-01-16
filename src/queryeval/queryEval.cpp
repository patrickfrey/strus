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
#include "queryEval.hpp"
#include "query.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/constants.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "parser/lexems.hpp"
#include "parser/keyMap.hpp"
#include "parser/mapFunctionParameters.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;
using namespace strus::parser;

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

void QueryEval::parseTermDef( char const*& src)
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
		m_predefinedTerms.push_back( TermDef( termset, termtype, termvalue));
	}
	else
	{
		throw std::runtime_error( "feature set identifier expected as start of a term declaration in the query");
	}
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

void QueryEval::parseWeightingFunctionDef( char const*& src)
{
	if (m_weightingFunction.function)
	{
		throw std::runtime_error( "more than one weighting function defined");
	}
	if (!isAlpha( *src))
	{
		throw std::runtime_error( "weighting function identifier expected");
	}
	m_weightingFunction.functionName = boost::algorithm::to_lower_copy( parse_IDENTIFIER( src));
	m_weightingFunction.function
		= m_processor->getWeightingFunction(
			m_weightingFunction.functionName);
	for (;;)
	{
		if (isOpenSquareBracket( *src))
		{
			if (!m_weightingFunction.selectorSets.empty())
			{
				throw std::runtime_error("duplicate definion of selected features in weighting function definition");
			}
			parse_OPERATOR(src);
			m_weightingFunction.selectorSets = parseIdentifierList( src);
			if (!isCloseSquareBracket( *src))
			{
				throw std::runtime_error( "comma ',' or close oval bracket ']' expected as separator or terminator of the selected feature set declaration for the weighting function");
			}
			parse_OPERATOR(src);
		}
		else if (isOpenAngleBracket(*src))
		{
			if (!m_weightingFunction.parameters.empty())
			{
				throw std::runtime_error("duplicate definion of arguments in weighting function definition");
			}
			parse_OPERATOR(src);
			m_weightingFunction.parameters
				= parseParameters( m_weightingFunction.function->parameterNames(), src);
			if (!isCloseAngleBracket(*src))
			{
				throw std::runtime_error( "expected comma ',' as separator or close angle bracket '>' to close parameter list of weighting function");
			}
			parse_OPERATOR(src);
		}
		else if (isOpenOvalBracket( *src))
		{
			if (!m_weightingFunction.weightingSets.empty())
			{
				throw std::runtime_error("duplicate definion of the weighted features in the weighting function definition");
			}
			parse_OPERATOR(src);
			m_weightingFunction.weightingSets = parseIdentifierList( src);
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
}


void QueryEval::parseSummarizeDef( char const*& src)
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
	m_summarizers.push_back(
		SummarizerDef(
			function, functionName, parameters, resultAttribute,
			contentType, structSet, featureSet));
}


void QueryEval::parseJoinOperationDef( char const*& )
{
}

void QueryEval::loadProgram( const std::string& source)
{
	char const* src = source.c_str();
	enum StatementKeyword {e_EVAL, e_JOIN, e_TERM, e_SUMMARIZE};
	std::string id;

	skipSpaces( src);
	try
	{
		while (*src)
		{
			switch ((StatementKeyword)parse_KEYWORD( src, 4, "EVAL", "JOIN", "TERM", "SUMMARIZE"))
			{
				case e_TERM:
					parseTermDef( src);
					break;
				case e_JOIN:
					throw std::runtime_error("JOIN not implemented yet");
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

QueryEval::QueryEval(
		const QueryProcessorInterface* processor_,
		const std::string& source)
	:m_processor(processor_)
{
	loadProgram( source);
}


void QueryEval::print( std::ostream& out) const
{
	std::vector<TermDef>::const_iterator ti = m_predefinedTerms.begin(), te = m_predefinedTerms.end();
	for (; ti != te; ++ti)
	{
		out << "TERM " << ti->set << ": " << ti->type << " '" << ti->value << "';" << std::endl;
	}
	if (m_weightingFunction.function)
	{
		out << "EVAL ";
		out << " " << m_weightingFunction.functionName;
		if (m_weightingFunction.parameters.size())
		{
			out << "<";
			std::size_t ai = 0, ae = m_weightingFunction.parameters.size();
			for(; ai != ae; ++ai)
			{
				if (ai) out << ", ";
				out << m_weightingFunction.function->parameterNames()[ai]
					<< "=" << m_weightingFunction.parameters[ai];
			}
			out << ">";
		}
		if (m_weightingFunction.selectorSets.size())
		{
			out << "[";
			std::size_t si = 0, se = m_weightingFunction.selectorSets.size();
			for(; si != se; ++si)
			{
				if (si) out << ", ";
				out << m_weightingFunction.selectorSets[si];
			}
			out << "]";
		}
		if (m_weightingFunction.weightingSets.size())
		{
			out << "(";
			std::size_t wi = 0, we = m_weightingFunction.weightingSets.size();
			for(; wi != we; ++wi)
			{
				if (wi) out << ", ";
				out << m_weightingFunction.weightingSets[wi];
			}
			out << ")";
		}
		out << ";" << std::endl;
	}
	std::vector<SummarizerDef>::const_iterator
		si = m_summarizers.begin(), se = m_summarizers.end();
	for (; si != se; ++si)
	{
		out << "SUMMARIZE ";
		out << si->resultAttribute << " = " << si->functionName;
		if (si->parameters.size())
		{
			out << "<";
			std::size_t ai = 0, ae = si->parameters.size();
			for(; ai != ae; ++ai)
			{
				if (ai) out << ", ";
				out << si->function->parameterNames()[ai] << "=" << si->parameters[ai];
			}
			out << ">";
		}
		if (!si->contentType.empty())
		{
			out << "[" << si->contentType << "]";
		}
		if (si->featureSet.size() || si->structSet.size())
		{
			out << "(";
			if (!si->structSet.empty())
			{
				out << si->structSet << ":";
			}
			std::size_t fi = 0, fe = si->featureSet.size();
			for(; fi != fe; ++fi)
			{
				if (fi) out << ", ";
				out << si->featureSet[fi];
			}
			out << ")";
		}
		out << ";" << std::endl;
	}
}


QueryInterface* QueryEval::createQuery() const
{
	return new Query( this, m_processor);
}

