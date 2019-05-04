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

	/// \brief Define a separator of entities in the query
	/// \param[in] uchr unicode character code for the separating character
	virtual void addSeparator( int uchr)=0;

	/// \brief Define a space in the query
	/// \param[in] uchr unicode character code for the spacing character
	virtual void addSpace( int uchr)=0;

	/// \brief Define a character potentially linking tokens to an entity in the query
	/// \param[in] uchr unicode character code for the character
	/// \param[in] substchr ascii character code for the replacement in an entity
	virtual void addLink( int uchr, char substchr)=0;

	/// \brief Define a value between 0.0 and 1.0 for the similarity measure of features for grouping similar features
	/// \param[in] value similarity measure
	/// \note Interpretation depends on implementation, a reasonable default value is set by default.
	virtual void defineGroupSimilarityDistance( double value)=0;
	
	/// \brief Create an interface for the traversal of a sentence specified
	/// \param[in] source string to parse with the lexer created
	/// \return lexer interface for sentence traversal
	virtual SentenceLexerContextInterface* createContext( const std::string& source) const=0;
};

}//namespace
#endif

