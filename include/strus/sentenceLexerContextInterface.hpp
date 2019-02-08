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

	/// \brief Get the first alternative split of tokens
	/// \return true if any split available
	virtual bool fetchFirstSplit()=0;

	/// \brief Get the first/next alternative split of tokens
	/// \return true if next alternative split available
	virtual bool fetchNextSplit()=0;

	/// \brief Get the number of tokens of the current split
	/// \return the number of tokens
	virtual int nofTokens() const=0;

	/// \brief Get the feature value in the context of the current split by index
	/// \return the feature value string
	virtual std::string featureValue( int idx)=0;

	/// \brief Get the possible feature types in the context of the current split by index
	/// \return the feature type list
	virtual std::vector<std::string> featureTypes( int idx)=0;
};

}//namespace
#endif

