/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure used as representant of a query term guess
/// \file sentenceTerm.hpp
#ifndef _STRUS_SENTENCE_TERM_HPP_INCLUDED
#define _STRUS_SENTENCE_TERM_HPP_INCLUDED
#include <string>
#include <vector>

namespace strus {

/// \brief Structure used as representant of a query term guess
class SentenceTerm
{
public:
	SentenceTerm( const std::string& type_, const std::string& value_, int origsize_)
		:m_type(type_),m_value(value_),m_origsize(origsize_){}
	SentenceTerm( const SentenceTerm& o)
		:m_type(o.m_type),m_value(o.m_value),m_origsize(o.m_origsize){}
	SentenceTerm()
		:m_type(),m_value(),m_origsize(0){}

	/// \brief All alternative type known for this entity
	std::string type() const	{return m_type;}
	/// \brief Term value of this entity
	std::string value() const	{return m_value;}
	/// \brief Size of this entity in the source in bytes
	int origsize() const		{return m_origsize;}

private:
	std::string m_type;
	std::string m_value;
	int m_origsize;
};


/// \brief Possible guess of a sentence
class SentenceGuess
{
public:
	SentenceGuess( const std::string classname_, const std::vector<SentenceTerm>& terms_)
		:m_classname(classname_),m_terms(terms_){}
	SentenceGuess( const SentenceGuess& o)
		:m_classname(o.m_classname),m_terms(o.m_terms){}

	const std::string& classname() const			{return m_classname;}
	const std::vector<SentenceTerm>& terms() const		{return m_terms;}

private:
	std::string m_classname;
	std::vector<SentenceTerm> m_terms;
};

}//namespace
#endif
