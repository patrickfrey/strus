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
#ifndef _STRUS_QUERY_EVAL_INTERFACE_HPP_INCLUDED
#define _STRUS_QUERY_EVAL_INTERFACE_HPP_INCLUDED
#include "strus/weightedDocument.hpp"
#include "strus/index.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace strus
{
/// \brief Forward declaration
class QueryProcessorInterface;

/// \brief Defines a program for evaluating a query
class QueryEvalInterface
{
public:
	class Query
	{
	public:
		Query(){}
		Query( const Query& o)
			:m_termar(o.m_termar){}

		/// \brief Add a new term to the query
		/// \param[in] set_ set name of the created feature
		/// \param[in] type_ type of the term in the storage
		/// \param[in] value_ value of the term in the storage
		void addTerm(
			const std::string& set_,
			const std::string& type_,
			const std::string& value_)
		{
			m_termar.push_back( Term( set_, type_, value_));
		}

		/// \brief Create a new term by replacing the top elements on the stack with a new element that is the result of a join operation
		/// \param[in] set_ set name of the created feature
		/// \param[in] opname_ join operation to perform
		/// \param[in] range_ range argument of the join operation
		/// \param[in] nofArgs_ number of arguments (top elements on the stack) 
		void joinTerms(
			const std::string& set_,
			const std::string& opname_,
			int range_,
			std::size_t nofArgs_)
		{
			m_joinar.push_back( JoinOp( m_termar.size(), set_, opname_, range_, nofArgs_));
		}

	public:
		struct Term
		{
			Term( const Term& o)
				:set(o.set),type(o.type),value(o.value){}
			Term( const std::string& s, const std::string& t, const std::string& v)
				:set(s),type(t),value(v){}

			std::string set;
			std::string type;
			std::string value;
		};
		struct JoinOp
		{
			JoinOp( const JoinOp& o)
				:termcnt(o.termcnt)
				,set(o.set)
				,opname(o.opname)
				,range(o.range)
				,nofArgs(o.nofArgs){}
			JoinOp( std::size_t c, const std::string& s, const std::string& o, int r, std::size_t n)
				:termcnt(c),set(s),opname(o),range(r),nofArgs(n){}

			std::size_t termcnt;
			std::string set;
			std::string opname;
			int range;
			std::size_t nofArgs;
		};

		const std::vector<Term>& termar() const		{return m_termar;}
		const std::vector<JoinOp>& joinar() const	{return m_joinar;}

	private:
		std::vector<Term> m_termar;
		std::vector<JoinOp> m_joinar;
	};

public:
	/// \brief Destructor
	virtual ~QueryEvalInterface(){}

	/// \brief Calculate a list of the best ranked documents
	/// \param[in] processor processor that creates the items needed to process the query
	/// \param[in] querystr query string (syntax depending on implementation)
	/// \param[in] maxNofRanks maximum number of ranks to return
	virtual std::vector<WeightedDocument>
		getRankedDocumentList(
			const QueryProcessorInterface& processor,
			const Query& query,
			std::size_t maxNofRanks) const=0;

	/// \brief Print the internal representation of the program to 'out'
	/// \param[out] out stream to print the program to
	/// \remark this method is mainly used to testing and has no other purpose
	virtual void print( std::ostream& out) const=0;
};

}//namespace
#endif

