/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace strus
{

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
		,m_accumulateOperations(o.m_accumulateOperations)
	{}

	/// \brief Term definition in the query
	class Term
	{
	public:
		Term()
			:m_resultset(0){}
		Term( const Term& o)
			:m_resultset(o.m_resultset),m_type(o.m_type),m_value(o.m_value){}
		Term( unsigned int resultset_, const std::string& type_, const std::string& value_)
			:m_resultset(resultset_),m_type(type_),m_value(value_){}

		unsigned int resultset() const				{return m_resultset;}
		const std::string& type() const				{return m_type;}
		const std::string& value() const			{return m_value;}

	private:
		unsigned int m_resultset;				///< set index of the term occurencies
		std::string m_type;					///< term type string
		std::string m_value;					///< term value string
	};

	/// \brief Attributes of a set needed in the initial parsing phase
	struct SetAttributes
	{
		SetAttributes()
			:id(0),nofElements(0),referenced(false){}
		SetAttributes( const SetAttributes& o)
			:id(o.id),nofElements(o.nofElements),referenced(o.referenced){}
		SetAttributes( std::size_t id_, std::size_t nofElements_)
			:id(id_),nofElements(nofElements_),referenced(false){}
		
		std::size_t id;			///< internal identifier of the set
		std::size_t nofElements;	///< number of elements in the set
		bool referenced;		///< true, if set has been referenced
	};

	struct SetElement
	{
		enum Type {TermType,IteratorType};

		Type type;
		std::size_t idx;

		SetElement()
			:type(TermType),idx(0){}
		SetElement( const SetElement& o)
			:type(o.type),idx(o.idx){}
		SetElement( Type type_, std::size_t idx_)
			:type(type_),idx(idx_){}
	};

	typedef std::vector<SetElement> SetElementList;

	/// \brief Defines a join operation of term occurrence sets
	class JoinOperation
	{
	public:
		struct Selector
		{
			Selector( unsigned int setIndex_, unsigned int elemIndex_)
				:setIndex(setIndex_),elemIndex(elemIndex_){}
			Selector( const Selector& o)
				:setIndex(o.setIndex),elemIndex(o.elemIndex){}
			Selector()
				:setIndex(0),elemIndex(0){}
	
			unsigned int setIndex;
			unsigned int elemIndex;
		};
		class SelectorSet
		{
		public:
			SelectorSet( std::size_t rowsize_)
				:m_rowsize(rowsize_){}

			void pushRow( const Selector* row)		{for (std::size_t ii=0; ii<m_rowsize; ++ii) m_ar.push_back( row[ii]);}
			void append( const SelectorSet& o)		{if (m_rowsize != o.m_rowsize) throw std::runtime_error("joining incompatible sets (with different number of rows)"); else m_ar.insert( m_ar.end(), o.m_ar.begin(), o.m_ar.end());}

			const std::vector<Selector>& ar() const		{return m_ar;}
			std::size_t rowsize() const			{return m_rowsize;}
			std::size_t nofrows() const			{return m_rowsize ? (m_ar.size() / m_rowsize):0;}

		private:
			std::vector<Selector> m_ar;			///< iterator reference sequences
			std::size_t m_rowsize;				///< number of selector elements in a sequence
		};
		typedef boost::shared_ptr<SelectorSet> SelectorSetR;

		JoinOperation(
				unsigned int resultset_,
				const std::string& name_,
				int range_,
				const SelectorSetR& selectorset_)
			:m_resultset(resultset_)
			,m_name(name_)
			,m_range(range_)
			,m_selectorset(selectorset_){}

		JoinOperation( const JoinOperation& o)
			:m_resultset(o.m_resultset)
			,m_name(o.m_name)
			,m_range(o.m_range)
			,m_selectorset(o.m_selectorset){}

	public:
		unsigned int resultset() const			{return m_resultset;}
		std::string name() const			{return m_name;}
		int range() const				{return m_range;}
		const SelectorSetR& selectorset() const		{return m_selectorset;}

	private:
		unsigned int m_resultset;	///< join operation result set index
		std::string m_name;		///< name of operation
		int m_range;			///< range for operations defined in a range (position difference)
		SelectorSetR m_selectorset;	///< iterator reference sequences
	};

	/// \brief Defines an accumulate operation
	class AccumulateOperation
	{
	public:
		struct Argument
		{
			std::string itrAccuOp;		///< specifies operation on iterator to create an accumulator. if empty then argument references an accumulator directly
			unsigned int setIndex;		///< index of argument (accumulator or iterator set)
			double weight;			///< weight of argument

			Argument()
				:itrAccuOp(),setIndex(0),weight(0.0){}
			Argument( const Argument& o)
				:itrAccuOp(o.itrAccuOp),setIndex(o.setIndex),weight(o.weight){}
			Argument( const std::string& itrAccuOp_, unsigned int setIndex_, double weight_=1.0)
				:itrAccuOp(itrAccuOp_),setIndex(setIndex_),weight(weight_){}
			explicit Argument( unsigned int setIndex_, double weight_=1.0)
				:itrAccuOp(),setIndex(setIndex_),weight(weight_){}

			bool isAccumulator() const	{return itrAccuOp.empty();}
		};

		AccumulateOperation(
				unsigned int resultaccu_,
				const std::string& name_,
				const std::vector<double>& scale_,
				const std::vector<Argument>& args_)
			:m_resultaccu(resultaccu_)
			,m_name(name_)
			,m_scale(scale_)
			,m_args(args_){}

		AccumulateOperation( const AccumulateOperation& o)
			:m_resultaccu(o.m_resultaccu)
			,m_name(o.m_name)
			,m_scale(o.m_scale)
			,m_args(o.m_args)
		{}

		unsigned int resultaccu() const				{return m_resultaccu;}
		std::string name() const				{return m_name;}
		const std::vector<double> scale() const			{return m_scale;}
		const std::vector<Argument>& args() const		{return m_args;}

	private:
		unsigned int m_resultaccu;				///< accumulate operation result 
		std::string m_name;					///< name of operation
		std::vector<double> m_scale;				///< scaling factors depending on the function
		std::vector<Argument> m_args;				///< list of set references with weight
	};
	
	const std::vector<Term>& terms() const					{return m_terms;}
	const std::vector<SetElementList>& setdefs() const			{return m_setdefs;}
	const std::vector<JoinOperation>& joinOperations() const		{return m_joinOperations;}
	const std::vector<AccumulateOperation>& accumulateOperations() const	{return m_accumulateOperations;}

	void pushQuery( const std::string& qry);

private:
	unsigned int defineSetElement( const std::string& setname, SetElement::Type type, std::size_t idx);

	void defineTerm( const std::string& setname, const std::string& type, const std::string& value);
	void defineJoinOperation( const std::string& setname, const std::string& funcname, int range, const JoinOperation::SelectorSetR& input);
	void defineAccumulateOperation( const std::string& setname, const std::string& funcname, const std::vector<double>& scale, const std::vector<AccumulateOperation::Argument>& args);

private:
	strus::KeyMap<SetAttributes> m_setmap;
	std::vector<SetElementList> m_setdefs;
	strus::KeyMap<unsigned int> m_accumap;

	std::vector<Term> m_terms;
	std::vector<JoinOperation> m_joinOperations;
	std::vector<AccumulateOperation> m_accumulateOperations;
};

}//namespace
#endif

