/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Object describing a variable associated to a set of postings. This variable can be used by summarizers to extract features from the forward index of matching documents.
/// \file summarizationVariable.hpp
#ifndef _STRUS_SUMMARIZATION_VARIABLE_HPP_INCLUDED
#define _STRUS_SUMMARIZATION_VARIABLE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <vector>
#include <string>

namespace strus
{

/// \brief Structure describing a variable referencing a named match of a subexpression. Variables are attached to features used for summarization.
class SummarizationVariable
{
public:
	/// \brief Constructor
	/// \param[in] name_ name of the variable
	/// \param[in] itr_ pointer to postinglist representing the variable (owned by caller)
	SummarizationVariable( const std::string& name_, PostingIteratorInterface* itr_)
		:m_name(name_),m_itr(itr_){}
	/// \brief Copy constructor
	/// \param[in] o variable to copy
	SummarizationVariable( const SummarizationVariable& o)
		:m_name(o.m_name),m_itr(o.m_itr){}

	/// \brief Name of the variable
	const std::string& name() const			{return m_name;}
	/// \brief Feature occurrence attached to this variable at the current position of the summarizer feature posting iterator this variable is attached to.
	/// \return the current position
	Index position() const				{return m_itr->posno();}

	/// \brief Posting iterator of the variable
	/// \return the posting iterator pointer of this variable
	const PostingIteratorInterface* itr() const	{return m_itr;}

private:
	std::string m_name;
	PostingIteratorInterface* m_itr;
};

}//namespace
#endif

