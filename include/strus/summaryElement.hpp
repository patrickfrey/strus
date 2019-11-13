/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Element of summary (result of summarization)
/// \file summaryElement.hpp
#ifndef _STRUS_SUMMARY_ELEMENT_HPP_INCLUDED
#define _STRUS_SUMMARY_ELEMENT_HPP_INCLUDED
#include <string>
#include <utility>

namespace strus
{

/// \brief One result element of summarization
class SummaryElement
{
public:
	/// \brief Constructor
	SummaryElement( const std::string& name_, const std::string& value_, double weight_=1.0, int index_=-1)
		:m_name(name_),m_value(value_),m_weight(weight_),m_index(index_){}
	/// \brief Copy constructor
	SummaryElement( const SummaryElement& o)
		:m_name(o.m_name),m_value(o.m_value),m_weight(o.m_weight),m_index(o.m_index){}
	SummaryElement& operator=( const SummaryElement& o)
		{m_name=o.m_name; m_value=o.m_value; m_weight=o.m_weight; m_index=o.m_index; return *this;}

#if __cplusplus >= 201103L
	SummaryElement( SummaryElement&& o)
		:m_name(std::move(o.m_name)),m_value(std::move(o.m_value)),m_weight(o.m_weight),m_index(o.m_index){}
	SummaryElement& operator=( SummaryElement&& o)
		{m_name=std::move(o.m_name); m_value=std::move(o.m_value); m_weight=o.m_weight; m_index=o.m_index; return *this;}
#endif
	/// \brief Set the summarizer id prefix given to it when defining it for the query evaluation
	void setSummarizerPrefix( const std::string& id_)
	{
		m_name.insert( 0, id_);
	}

	/// \brief Get the name of the element
	const std::string& name() const	{return m_name;}
	/// \brief Get the value of the element
	const std::string& value() const	{return m_value;}
	/// \brief Get the weight of the element if defined
	double weight() const			{return m_weight;}
	/// \brief Get the index of the element if defined
	int index() const			{return m_index;}

private:
	std::string m_name;		///< name of the element
	std::string m_value;		///< content value of the element
	double m_weight;		///< weight of the element if defined
	int m_index;			///< index for multiple results (results with same index belong to the same group)
};

} //namespace
#endif

