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
#include "strus/storage/sentenceGuess.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class SentenceLexerContextInterface;

/// \brief Lexer interface for analysis of query sentences
class SentenceLexerInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~SentenceLexerInstanceInterface(){}

	/// \brief Get a ranked list weighting the probability of alternative sentences forming a query
	/// \param[in] fields source fields to analyze
	/// \param[in] maxNofResults maximum number of results to return or -1 if not defined
	/// \param[in] minWeight minimum weight (normalized with the first rank weight) a returned result should have
	/// \return the ranked list of weighted sentences
	virtual std::vector<SentenceGuess> call( const std::vector<std::string>& fields, int maxNofResults, double minWeight) const=0;
};

}//namespace
#endif

