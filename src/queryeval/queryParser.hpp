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
#ifndef _STRUS_QUERY_PARSER_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_HPP_INCLUDED
#include "keyMap.hpp"
#include "parser/term.hpp"
#include "parser/selector.hpp"
#include "parser/setElement.hpp"
#include "parser/setDimDescription.hpp"
#include "parser/joinOperation.hpp"
#include "parser/accumulateOperation.hpp"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace strus {

/// \brief Parser of a query defined as string
class QueryParser
{
public:
	QueryParser(){}
	QueryParser( const QueryParser& o)
		:m_setmap(o.m_setmap)
		,m_setdefs(o.m_setdefs)
		,m_terms(o.m_terms)
		,m_joinOperations(o.m_joinOperations)
	{
		if (o.m_accumulateOperation.get())
		{
			m_accumulateOperation.reset( 
				new parser::AccumulateOperation(
					*o.m_accumulateOperation));
		}
	}

	const std::vector<parser::Term>& terms() const				{return m_terms;}
	const std::vector<parser::SetElementList>& setdefs() const		{return m_setdefs;}
	const std::vector<parser::JoinOperation>& joinOperations() const	{return m_joinOperations;}
	const parser::AccumulateOperation* accumulateOperation() const		{return m_accumulateOperation.get();}

	void pushQuery( const std::string& qry);

private:
	unsigned int defineSetElement(
			const std::string& setname,
			parser::SetElement::Type type,
			std::size_t idx);

	void defineTerm(
			const std::string& setname,
			const std::string& type,
			const std::string& value);

	void defineJoinOperation(
			const std::string& setname,
			const std::string& funcname,
			int range,
			const parser::SelectorSetR& input);

	void parseTerm( char const*& src);
	void parseJoin( char const*& src);
	void parseEval( char const*& src);

private:
	strus::KeyMap<parser::SetDimDescription> m_setmap;
	std::vector<parser::SetElementList> m_setdefs;

	std::vector<parser::Term> m_terms;
	std::vector<parser::JoinOperation> m_joinOperations;
	boost::shared_ptr<parser::AccumulateOperation> m_accumulateOperation;
};

}//namespace
#endif

