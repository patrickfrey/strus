/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for defining what to detect in a query sentence and how to weight it for guessing the most probable variants of a query
#ifndef _STRUS_SENTENCE_ANALYZER_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_SENTENCE_ANALYZER_INSTANCE_INTERFACE_HPP_INCLUDED
#include "strus/storage/sentenceTerm.hpp"
#include "strus/storage/sentenceGuess.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class SentenceLexerInstanceInterface;

/// \brief Interface for defining what to detect in a query sentence and how to weight it for guessing the most probable variants of a query
class SentenceAnalyzerInstanceInterface
{
public:
	virtual ~SentenceAnalyzerInstanceInterface(){}	

	/// \brief Assign a type to a priority group to reduce candidate expansion
	/// \param[in] name name of the type
	/// \param[in] priority priority assigned to it (highest is 0, descending with increment)
	virtual void defineWordType(
			const std::string& name,
			int priority)=0;

	/// \brief Analyze a sentence and return the best ranked candidates with weight and class name
	/// \param[in] lexer lexer instance
	/// \param[in] source source to parse
	/// \param[in] maxNofResults maximum number of results to return or -1 if undefined
	/// \param[in] minWeight defines cut of results with a lower weight
	/// \return list of named term lists with weight
	virtual std::vector<SentenceGuess> analyzeSentence(
			const SentenceLexerInstanceInterface* lexer,
			const std::string& source,
			int maxNofResults,
			double minWeight) const=0;
};

}//namespace
#endif

