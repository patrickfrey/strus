/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure used as representant of a query term
/// \file sentenceTerm.hpp
#ifndef _STRUS_SENTENCE_TERM_HPP_INCLUDED
#define _STRUS_SENTENCE_TERM_HPP_INCLUDED
#include <string>
#include <vector>

namespace strus {

/// \brief Structure used as representant of a query term
class SentenceTerm
{
public:
	/// \brief Constructor
	SentenceTerm( const std::string& type_, const std::string& value_)
		:m_type(type_),m_value(value_){}
	/// \brief Copy constructor
	SentenceTerm( const SentenceTerm& o)
		:m_type(o.m_type),m_value(o.m_value){}
	/// \brief Default constructor
	SentenceTerm()
		:m_type(),m_value(){}

	SentenceTerm& operator = ( const SentenceTerm& o)
		{m_type=o.m_type; m_value=o.m_value; return *this;}
#if __cplusplus >= 201103L
	SentenceTerm( SentenceTerm&& o)
		:m_type(std::move(o.m_type)),m_value(std::move(o.m_value)){}
	SentenceTerm& operator=( SentenceTerm&& o)
		{m_type=std::move(o.m_type); m_value=std::move(o.m_value); return *this;}
#endif

	/// \brief All alternative type known for this entity
	std::string type() const	{return m_type;}
	/// \brief Term value of this entity
	std::string value() const	{return m_value;}

	bool operator < (const SentenceTerm& o) const
	{
		return m_value == o.m_value ? m_type < o.m_type : m_value < o.m_value;
	}
	bool operator == (const SentenceTerm& o) const
	{
		return m_value == o.m_value && m_type == o.m_type;
	}

private:
	std::string m_type;
	std::string m_value;
};

/// \brief Structure used as representant of a query term guess
typedef std::vector<SentenceTerm> SentenceTermList;

}//namespace
#endif
