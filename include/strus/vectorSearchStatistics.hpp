/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Statistics of a vector similarity search search
/// \file "vectorSearchStatistics.hpp"
#ifndef _STRUS_VECTOR_SEARCH_STATISTICS_HPP_INCLUDED
#define _STRUS_VECTOR_SEARCH_STATISTICS_HPP_INCLUDED
#include "strus/numericVariant.hpp"
#include "strus/vectorQueryResult.hpp"
#include <string>

namespace strus {

/// \brief Result of a vector search (associated feature number with weight)
class VectorSearchStatistics
{
public:
	class Item
	{
	public:
		/// \brief Default constructor
		Item()
			:m_name(),m_value(0.0){}
		/// \brief Copy constructor
		Item( const Item& o)
			:m_name(o.m_name),m_value(o.m_value){}
		/// \brief Constructor
		Item( const std::string& name_, const strus::NumericVariant& value_)
			:m_name(name_),m_value(value_){}
	
#if __cplusplus >= 201103L
		Item( Item&& o) :m_name(std::move(o.m_name)),m_value(std::move(o.m_value)) {}
		Item& operator=( Item&& o) {m_name=std::move(o.m_name); m_value=std::move(o.m_value); return *this;}
#endif
		const std::string& name() const		{return m_name;}
		double value() const			{return m_value;}
	
	private:
		std::string m_name;
		strus::NumericVariant m_value;
	};

	/// \brief Default constructor
	VectorSearchStatistics()
		:m_items(),m_results(){}
	/// \brief Copy constructor
	VectorSearchStatistics( const VectorSearchStatistics& o)
		:m_items(o.m_items),m_results(o.m_results){}
	/// \brief Constructor
	VectorSearchStatistics( const std::vector<Item>& items_, const std::vector<VectorQueryResult>& results_)
		:m_items(items_),m_results(results_){}
#if __cplusplus >= 201103L
	VectorSearchStatistics( VectorSearchStatistics&& o) :m_items(std::move(o.m_items)),m_results(std::move(o.m_results)) {}
	VectorSearchStatistics& operator=( VectorSearchStatistics&& o) {m_items=std::move(o.m_items); m_results=std::move(o.m_results); return *this;}
#endif

	VectorSearchStatistics& operator()( const std::string& name, const NumericVariant& value)
	{
		m_items.push_back( Item( name, value));
		return *this;
	}
	void setResults( const std::vector<VectorQueryResult>& results_)
	{
		m_results = results_;
	}

	const std::vector<Item>& items() const			{return m_items;}
	const std::vector<VectorQueryResult>& results() const	{return m_results;}

	typedef std::vector<Item>::const_iterator const_iterator;
	const_iterator begin() const	{return m_items.begin();}
	const_iterator end() const	{return m_items.end();}

private:
	std::vector<Item> m_items;
	std::vector<VectorQueryResult> m_results;
};

}//namespace
#endif

