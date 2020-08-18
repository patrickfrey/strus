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
#include "strus/base/math.hpp"
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

	/// \brief All alternative type known for this term
	const std::string& type() const		{return m_type;}
	/// \brief Term value of this term
	const std::string& value() const	{return m_value;}

	bool operator < (const SentenceTerm& o) const
	{
		return m_value == o.m_value ? m_type < o.m_type : m_value < o.m_value;
	}
	bool operator == (const SentenceTerm& o) const
	{
		return m_value == o.m_value && m_type == o.m_type;
	}
	bool operator != (const SentenceTerm& o) const
	{
		return m_value != o.m_value || m_type != o.m_type;
	}

private:
	std::string m_type;
	std::string m_value;
};

/// \brief List of weighted sentence terms
typedef std::vector<SentenceTerm> SentenceTermList;

/// \brief Structure used as representant of a query term with a weight
class WeightedSentenceTerm
	:public SentenceTerm
{
public:
	/// \brief Constructor
	WeightedSentenceTerm( const std::string& type_, const std::string& value_, double weight_)
		:SentenceTerm(type_,value_),m_weight(weight_){}
	/// \brief Constructor
	WeightedSentenceTerm( const SentenceTerm& o, double weight_)
		:SentenceTerm(o),m_weight(weight_){}
	/// \brief Copy constructor
	WeightedSentenceTerm( const WeightedSentenceTerm& o)
		:SentenceTerm(o),m_weight(o.m_weight){}
	/// \brief Default constructor
	WeightedSentenceTerm()
		:SentenceTerm(),m_weight(0.0){}

	WeightedSentenceTerm& operator = ( const WeightedSentenceTerm& o)
		{SentenceTerm::operator=(o); m_weight = o.m_weight; return *this;}
#if __cplusplus >= 201103L
	WeightedSentenceTerm( WeightedSentenceTerm&& o)
		:SentenceTerm(o),m_weight(o.m_weight){}
	WeightedSentenceTerm& operator=( WeightedSentenceTerm&& o)
		{SentenceTerm::operator=(o); m_weight = o.m_weight; return *this;}
#endif

	/// \brief Weight of this term
	double weight() const		{return m_weight;}
	/// \brief Set weight of this term
	void setWeight( double w)	{m_weight = w;}

	bool operator < (const WeightedSentenceTerm& o) const
	{
		return strus::Math::isequal(m_weight,o.m_weight)
			? value() == o.value()
				? type() < o.type()
				: value() < o.value()
			: m_weight < o.m_weight;
	}
	bool operator == (const WeightedSentenceTerm& o) const
	{
		return strus::Math::isequal(m_weight,o.m_weight) && type() == o.type() && value() == o.value();
	}
	bool operator != (const WeightedSentenceTerm& o) const
	{
		return !strus::Math::isequal(m_weight,o.m_weight) || type() != o.type() || value() != o.value();
	}

private:
	double m_weight;
};

/// \brief List of weighted sentence terms
typedef std::vector<WeightedSentenceTerm> WeightedSentenceTermList;

}//namespace
#endif
