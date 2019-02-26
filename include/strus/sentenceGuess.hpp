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
#include <vector>
#include <cmath>
#include <ctgmath>
#include <limits>

namespace strus {

/// \brief Weighted guess of a sentence
class SentenceGuess
{
public:
	/// \brief Constructor
	SentenceGuess( const std::string classname_, const std::vector<SentenceTerm>& terms_, double weight_)
		:m_classname(classname_),m_terms(terms_),m_weight(weight_){}
	/// \brief Copy constructor
	SentenceGuess( const SentenceGuess& o)
		:m_classname(o.m_classname),m_terms(o.m_terms),m_weight(o.m_weight){}

	/// \brief Name of the class of this sentence
	const std::string& classname() const		{return m_classname;}
	/// \brief List of terms of this sentence
	const SentenceTermList& terms() const		{return m_terms;}
	/// \brief Weight assigned to this sentence
	double weight() const				{return m_weight;}

	bool operator < (const SentenceGuess& o) const
	{
		return (std::abs( m_weight - o.m_weight) <= std::numeric_limits<double>::epsilon())
			? (m_terms.size() == o.m_terms.size()
				? (m_classname == o.m_classname
					? lesserTermList( m_terms, o.m_terms)
					: m_classname < o.m_classname)
				: m_terms.size() < o.m_terms.size())
			: m_weight < o.m_weight;
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

private:
	std::string m_classname;
	SentenceTermList m_terms;
	double m_weight;
};

}//namespace
#endif
