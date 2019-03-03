/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure used as representant of a weighted guess of a query sentence
/// \file sentenceGuess.hpp
#ifndef _STRUS_SENTENCE_GUESS_HPP_INCLUDED
#define _STRUS_SENTENCE_GUESS_HPP_INCLUDED
#include "strus/sentenceTerm.hpp"
#include <string>
#include <cmath>
#include <ctgmath>
#include <utility>
#include <limits>

namespace strus {

/// \brief Weighted guess of a sentence
class SentenceGuess
{
public:
	/// \brief Default constructor
	SentenceGuess()
		:m_classname(),m_terms(),m_weight(0.0){}
	/// \brief Constructor
	SentenceGuess( const std::string& classname_, const SentenceTermList& terms_, double weight_)
		:m_classname(classname_),m_terms(terms_),m_weight(weight_){}
	/// \brief Copy constructor
	SentenceGuess( const SentenceGuess& o)
		:m_classname(o.m_classname),m_terms(o.m_terms),m_weight(o.m_weight){}
	SentenceGuess& operator = ( const SentenceGuess& o)
		{m_classname=o.m_classname; m_terms=o.m_terms; m_weight=o.m_weight; return *this;}
#if __cplusplus >= 201103L
	SentenceGuess( SentenceGuess&& o)
		:m_classname(std::move(o.m_classname)),m_terms(std::move(o.m_terms)),m_weight(o.m_weight){}
	SentenceGuess& operator=( SentenceGuess&& o)
		{m_classname=std::move(o.m_classname); m_terms=std::move(o.m_terms); m_weight=o.m_weight; return *this;}
#endif

	/// \brief Name of the class of this sentence
	/// \return the name given to this guess
	const std::string& classname() const		{return m_classname;}
	/// \brief List of terms of this sentence
	/// \return the list of terms
	const SentenceTermList& terms() const		{return m_terms;}
	/// \brief Weight assigned to this sentence
	/// \return the weight
	double weight() const				{return m_weight;}
	/// \brief Evaluate if the result is defined (non empty)
	/// \return true if yes, false else
	bool valid() const				{return !m_terms.empty();}

	/// \brief Comparator for sorting results according relevance
	bool operator < (const SentenceGuess& o) const
	{
		return (std::abs( m_weight - o.m_weight) <= std::numeric_limits<float>::epsilon())
			? (m_terms.size() == o.m_terms.size()
				? (m_classname == o.m_classname
					? lesserTermList( m_terms, o.m_terms)
					: m_classname < o.m_classname)
				: m_terms.size() < o.m_terms.size())
			: m_weight < o.m_weight;
	}
	/// \brief Comparator for testing equality
	bool operator == (const SentenceGuess& o) const
	{
		return (std::abs( m_weight - o.m_weight) <= std::numeric_limits<float>::epsilon())
			&& m_terms.size() == o.m_terms.size()
			&& m_classname == o.m_classname
			&& equalTermList( m_terms, o.m_terms);
	}

	/// \brief Reset the contents
	void clear()
	{
		m_terms.clear();
		m_classname.clear();
		m_weight = 0.0;
	}

private:
	static bool lesserTermList( const SentenceTermList& t1, const SentenceTermList& t2)
	{
		SentenceTermList::const_iterator i1 = t1.begin(), e1 = t1.end();
		SentenceTermList::const_iterator i2 = t2.begin(), e2 = t2.end();
		for (; i1 != e1 && i2 != e2 && *i1 == *i2; ++i1,++i2){}
		if (i1 == e1)
		{
			if (i2 == e2)
			{
				return false; /* == */
			}
			else
			{
				return true; /* < */
			}
		}
		else
		{
			if (i2 == e2)
			{
				return false; /* > */
			}
			else
			{
				return *i1 < *i2;
			}
		}
	}
	static bool equalTermList( const SentenceTermList& t1, const SentenceTermList& t2)
	{
		SentenceTermList::const_iterator i1 = t1.begin(), e1 = t1.end();
		SentenceTermList::const_iterator i2 = t2.begin(), e2 = t2.end();
		for (; i1 != e1 && i2 != e2 && *i1 == *i2; ++i1,++i2){}
		return (i1 == e1 && i2 == e2);
	}

private:
	std::string m_classname;
	SentenceTermList m_terms;
	double m_weight;
};

}//namespace
#endif
