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
#include "parser/summarizeOperation.hpp"
#include "parser/lexems.hpp"

using namespace strus;
using namespace strus::parser;

#error DEPRECATED

SummarizeOperation::SummarizeOperation(
			const std::string& resultAttribute_,
			const std::string& summarizerName_,
			const std::string& type_,
			const std::vector<float>& parameter_,
			const std::vector<int>& featureSet_,
			int structSet_)
		:m_resultAttribute(resultAttribute_)
		,m_summarizerName(summarizerName_)
		,m_type(type_)
		,m_parameter(parameter_)
		,m_featureSet(featureSet_)
		,m_structSet(structSet_){}

SummarizeOperation::SummarizeOperation( const SummarizeOperation& o)
		:m_resultAttribute(o.m_resultAttribute)
		,m_summarizerName(o.m_summarizerName)
		,m_type(o.m_type)
		,m_parameter(o.m_parameter)
		,m_featureSet(o.m_featureSet)
		,m_structSet(o.m_structSet){}


void SummarizeOperation::parse( char const*& src, StringIndexMap& setnamemap)
{
	if (!isAlpha( *src))
	{
		throw std::runtime_error( "name of result attribute expected after SUMMARIZE");
	}
	m_resultAttribute = parse_IDENTIFIER( src);
	if (!isAssign(*src))
	{
		throw std::runtime_error( "assignment operator '=' expected after SUMMARIZE and name of result attribute");
	}
	parse_OPERATOR( src);
	if (!isAlpha( *src))
	{
		throw std::runtime_error( "name of summarizer expected after assignment in SUMMARIZE definition");
	}
	m_summarizerName = parse_IDENTIFIER( src);
	if (isOpenSquareBracket( *src))
	{
		parse_OPERATOR( src);
		if (isAlpha( *src))
		{
			m_type = parse_IDENTIFIER( src);
		}
		if (!isCloseSquareBracket( *src))
		{
			throw std::runtime_error( "expected only one identifier inside in square brackets '[' ']' in SUMMARIZE definition");
		}
		parse_OPERATOR( src);
	}
	if (isOpenOvalBracket( *src))
	{
		parse_OPERATOR( src);
		for(;;)
		{
			if (!isDigit( *src))
			{
				throw std::runtime_error( "numeric arguments expected in oval brackets '(' ')' in SUMMARIZE definition");
			}
			m_parameter.push_back( parse_FLOAT( src));
			if (isComma(*src))
			{
				parse_OPERATOR( src);
			}
			else
			{
				break;
			}
		}
		if (!isCloseOvalBracket( *src))
		{
			throw std::runtime_error("comma ',' or close oval bracket ')' expected as separator in list of scalar arguments in a SUMMARIZE definition");
		}
		parse_OPERATOR( src);
	}
	bool isDefinedFROM = false;
	bool isDefinedIN = false;
	while (isAlpha( *src))
	{
		std::string keyword = parse_IDENTIFIER( src);
		if (isEqual( keyword, "FROM"))
		{
			if (isDefinedFROM) throw std::runtime_error("duplicate definition of FROM in SUMMARIZER declaration");
			isDefinedFROM = true;
			for(;;)
			{
				if (!isAlpha( *src))
				{
					throw std::runtime_error( "expected comma separated list of identifiers (feature set list) after FROM in a SUMMARIZE definition");
				}
				m_featureSet.push_back(
					setnamemap.get( parse_IDENTIFIER( src)));
				if (isComma(*src))
				{
					parse_OPERATOR( src);
				}
				else
				{
					break;
				}
			}
		}
		else if (isEqual( keyword, "IN"))
		{
			if (isDefinedIN) throw std::runtime_error("duplicate definition of IN in SUMMARIZER declaration");
			isDefinedIN = true;

			if (!isAlpha( *src))
			{
				throw std::runtime_error( "expected identifier (structure feature set) after IN in a SUMMARIZE definition");
			}
			m_structSet = setnamemap.get( parse_IDENTIFIER( src));
		}
		else
		{
			throw std::runtime_error("expected IN and a feature set or FROM and a feature set list or end of SUMMARIZE declaration");
		}
	}
}

void SummarizeOperation::print( std::ostream& out, const StringIndexMap& setnamemap) const
{
	out << m_resultAttribute << "=" << m_summarizerName;
	if (!m_type.empty()) out << " [" << m_type << "]";
	if (!m_parameter.empty())
	{
		out << "( ";
		std::vector<float>::const_iterator pi = m_parameter.begin(), pe = m_parameter.end();
		for (int pidx=0; pi != pe; ++pi,++pidx)
		{
			if (pidx) out << ", ";
			out << *pi;
		}
		out << ")";
	}
	if (!m_featureSet.empty())
	{
		out << " FROM ";
		std::vector<int>::const_iterator fi = m_featureSet.begin(), fe = m_featureSet.end();
		for (int fidx=0; fi != fe; ++fi,++fidx)
		{
			if (fidx) out << ", ";
			out << setnamemap.name(*fi);
		}
	}
}

