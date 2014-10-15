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
#include "queryParser.hpp"
#include "parser/lexems.hpp"
#include <sstream>
#include <iostream>
#include <stdexcept>

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

unsigned int QueryParser::defineSetElement( const std::string& setname, SetElement::Type type, std::size_t idx)
{
	std::size_t setdefidx;
	strus::KeyMap<SetDimDescription>::iterator ti = m_setmap.find( setname);
	if (ti == m_setmap.end())
	{
		setdefidx = m_setdefs.size();
		m_setdefs.push_back( SetElementList());
		m_setmap[ setname] = SetDimDescription( setdefidx+1, 1);
	}
	else
	{
		if (ti->second.referenced)
		{
			throw std::runtime_error( std::string( "try to add element to already referenced set '") + setname + "'");
		}
		setdefidx = ti->second.id-1;
		++ ti->second.nofElements;
	}
	m_setdefs[ setdefidx].push_back( SetElement( type, idx+1));
	return setdefidx+1;
}

void QueryParser::defineTerm( const std::string& setname, const std::string& type, const std::string& value)
{
	unsigned int resultsetIndex = defineSetElement( setname, SetElement::TermType, m_terms.size());
	Term term( resultsetIndex, type, value);
	m_terms.push_back( term);
}

void QueryParser::defineJoinOperation( const std::string& setname, const std::string& funcname, int range, const SelectorSetR& input)
{
	unsigned int resultsetIndex = defineSetElement( setname, SetElement::IteratorType, m_joinOperations.size());
	JoinOperation op( resultsetIndex, funcname, range, input);
	m_joinOperations.push_back( op);
}

void QueryParser::parseTerm( char const*& src)
{
	std::string group;
	std::string type;
	std::string value;

	if (!isAlpha(*src))
	{
		throw std::runtime_error( "identifier expected for set name after TERM");
	}
	group = IDENTIFIER( src);
	if (!isColon(*src))
	{
		throw std::runtime_error( "colon ':' expected after name of term set in TERM declaration");
	}
	OPERATOR(src);
	if (!isAlpha(*src))
	{
		throw std::runtime_error( "identifier expected for term type in TERM declaration");
	}
	type = IDENTIFIER( src);
	if (isAlnum( *src))
	{
		value = IDENTIFIER( src);
	}
	else if (isStringQuote( *src))
	{
		value = STRING( src);
	}
	else
	{
		throw std::runtime_error( "unexpected token, string or alphanumeric sequence expected in TERM declaration");
	}
	defineTerm( group, type, value);
}

void QueryParser::parseJoin( char const*& src)
{
	std::string group;
	std::string function;
	int range = 0;

	if (!isAlpha(*src))
	{
		throw std::runtime_error( "identifier expected for set name after JOIN");
	}
	group = IDENTIFIER( src);
	if (!isColon(*src))
	{
		throw std::runtime_error( "colon ':' expected after name of join destination set in a JOIN declaration");
	}
	OPERATOR(src);
	if (!isAlpha(*src))
	{
		throw std::runtime_error( "identifier expected for the set join operator name in a JOIN declaration");
	}
	function = IDENTIFIER( src);
	if (isDigit(*src))
	{
		range = INTEGER( src);
	}
	if (isOpenSquareBracket( *src))
	{
		OPERATOR( src);
		SelectorSetR input = SelectorSet::parseExpression( src, m_setmap);
		defineJoinOperation( group, function, range, input);
	}
	else if (isAlpha(*src))
	{
		SelectorSetR input = SelectorSet::parseAtomic( src, m_setmap);
		defineJoinOperation( group, function, range, input);
	}
	else
	{
		throw std::runtime_error( "identifier (name of set) or tuple constructor in '[' ']' brackets expected after the join operator name and an optional range in a JOIN declaration");
	}
}

void QueryParser::parseEval( char const*& src)
{
	if (m_accumulateOperation.get())
	{
		throw std::runtime_error( "duplicate definition of an query evaluation function with EVAL");
	}
	if (isAlpha( *src))
	{
		std::string accuname( IDENTIFIER( src));

		if (isOpenOvalBracket( *src))
		{
			OPERATOR(src);
			std::vector<WeightingFunction> args
				= WeightingFunction::parseExpression( src, m_setmap);

			m_accumulateOperation.reset( new AccumulateOperation( accuname, args));
		}
		else
		{
			throw std::runtime_error( "oval brackets expected after accumulate function name and the scaling parameters");
		}
	}
	else
	{
		throw std::runtime_error( "name of accumulator function expected after EVAL");
	}
}

void QueryParser::pushQuery( const std::string& qry)
{
	char const* src = qry.c_str();
	try
	{
		skipSpaces( src);
		while (*src)
		{
			if (isAlpha(*src))
			{
				std::string opname = IDENTIFIER( src);
				if (isEqual( opname, "TERM"))
				{
					parseTerm( src);
				}
				else if (isEqual( opname, "JOIN"))
				{
					parseJoin( src);
				}
				else if (isEqual( opname, "EVAL"))
				{
					parseEval( src);
				}
				else
				{
					throw std::runtime_error( std::string( "TERM,JOIN or EVAL expected instead of '") + opname + "'");
				}
				if (*src)
				{
					if (isSemiColon( *src))
					{
						OPERATOR( src);
					}
					else
					{
						throw std::runtime_error( "semicolon ';' expected as query expression separator");
					}
				}
			}
			else
			{
				throw std::runtime_error( "identifier (TERM,JOIN or EVAL) expected at start of an query operation declaration");
			}
		}
	}
	catch (const std::runtime_error& e)
	{
		throw std::runtime_error(
			std::string( "error in query ")
			+ errorPosition( qry.c_str(), src)
			+ ":" + e.what());
	}
}


