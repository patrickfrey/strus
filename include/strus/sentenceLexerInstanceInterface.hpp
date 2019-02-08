/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Client interface for a storage with the items used for guessing multipart features in a query sentence
/// \file sentenceLexerInstanceInterface.hpp
#ifndef _STRUS_SENTENCE_LEXER_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_SENTENCE_LEXER_INSTANCE_INTERFACE_HPP_INCLUDED
#include "strus/sentenceTerm.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class SentenceLexerContextInterface;

/// \brief Lexer interface for tokenizing query sentences
class SentenceLexerInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~SentenceLexerInstanceInterface(){}

	/// \brief Define separator of entities in the query
	/// \param[in] uchr unicode character code for the punctuation character
	/// \param[in] sbchr ascii character code for the replacement
	/// \param[in] priority defines the replacement rule of lower priority seperators by higher priority separators
	virtual void addSeparator( int uchr)=0;

	/// \brief Define a character potentially linking tokens to an entity in the query
	/// \param[in] uchr unicode character code for the character
	/// \param[in] sbchr ascii character code for the replacement in an entity
	/// \param[in] priority defines the replacement rule of lower priority seperators by higher priority linking characters
	virtual void addLink( int uchr, char sbchr, int priority)=0;

	/// \brief Create an interface for the traversal of a sentence specified
	/// \param[in] source string to parse with the lexer created
	/// \return lexer interface for sentence traversal
	virtual SentenceLexerContextInterface* createContext( const std::string& source) const=0;

	/// \brief Get the similarity of a term to a another
	/// \param[in] term term to check
	/// \param[in] other term to check against
	/// \note Similarity may be used in corner cases to select a variant of a query
	virtual double getSimilarity( const SentenceTerm& term, const SentenceTerm& other) const=0;
};

}//namespace
#endif

