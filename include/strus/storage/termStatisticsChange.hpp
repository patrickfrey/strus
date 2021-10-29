/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Term statistics change (for a distributed search index)
/// \file "termStatisticsChange.hpp"
#ifndef _STRUS_TERM_STATISTICS_CHANGE_HPP_INCLUDED
#define _STRUS_TERM_STATISTICS_CHANGE_HPP_INCLUDED
#include <string>

namespace strus {

/// \class TermStatisticsChange
/// \brief Structure describing the change of statistics of one term in the collection
class TermStatisticsChange
{
public:
	TermStatisticsChange()
		:m_type(),m_value(),m_increment(0){}
	TermStatisticsChange( const TermStatisticsChange& o)
		:m_type(o.m_type),m_value(o.m_value),m_increment(o.m_increment){}
	TermStatisticsChange( const std::string& type_, const std::string& value_, int increment_)
		:m_type(type_),m_value(value_),m_increment(increment_){}

	const std::string& type() const		{return m_type;}
	const std::string& value() const	{return m_value;}
	int increment() const			{return m_increment;}

private:
	std::string m_type;	///< type of the term
	std::string m_value;	///< value of the term
	int m_increment;	///< document frequency increment/decrement
};

}//namespace
#endif

