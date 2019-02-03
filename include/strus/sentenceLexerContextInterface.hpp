/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Context for parsing a sentence
/// \file sentenceLexerContextInterface.hpp
#ifndef _STRUS_SENTENCE_LEXER_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_SENTENCE_LEXER_CONTEXT_INTERFACE_HPP_INCLUDED
#include "strus/sentenceTerm.hpp"
#include <string>
#include <vector>

namespace strus {

/// \brief Context for parsing the lexical tokens of a sentence
class SentenceLexerContextInterface
{
public:
	/// \brief Destructor
	virtual ~SentenceLexerContextInterface(){}

	/// \brief Parse a list of alternatives of the next term in the string parsed
	/// \return list of alternative picks
	virtual std::vector<SentenceTerm> altLexems() const=0;

	/// \brief Eat the token with a defined length and continue parsing with the next token
	virtual bool skipToFollow( int length)=0;

	/// \brief Skip back to previous position in string (before the last call of 'skipToNext()'
	/// \note for backtracking parse state
	virtual void skipBack()=0;
};

}//namespace
#endif

