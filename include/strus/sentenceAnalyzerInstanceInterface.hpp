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
#include "strus/sentenceTerm.hpp"
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

	/// \brief Push a term on the stack for building expressions
	/// \param[in] type type of the term to match or an empty string for any type accepted
	/// \param[in] name string of the term to match or an empty string for any value accepted
	/// \param[in] priority priority of the pick (greater is better) for choosing a best match of matching alternatives
	virtual void pushTerm( const std::string& type, const std::string& name, int priority)=0;

	/// \brief Define the top elements (specified as number of elements) on the stack as alternatives of a choice
	/// \param[in] argc number of elements to group together as selection of alternatives
	/// \param[in] similarity true if similarity to a neighbour token in the query is deciding between picks with identical priorization
	/// \param[in] priority priority of the resulting structure (greater is better) for choosing a best match of matching alternatives
	virtual void pushAlt( int argc, bool similarity, int priority)=0;

	/// \brief Define the top elements (specified as number of elements) on the stack as sequence of immediately succeeding elements to appear in this order
	/// \param[in] argc number of elements to group together as sequence of immediately succeeding elements
	/// \param[in] priority priority of the resulting structure (greater is better) for choosing a best match of matching alternatives
	virtual void pushSequenceImm( int argc, int priority)=0;

	/// \brief Define the top element on the stack to be a repeating structure
	/// \param[in] times maximum number of repetitions or -1 for unlimited
	virtual void pushRepeat( int times) const=0;

	/// \brief Define the last element on the stack as sentence structure
	/// \param[in] classname name given to the structure
	/// \param[in] priority assigned to the structure (greater is better) for eliminating lower priority structures in the result
	virtual void defineSentence( const std::string& classname, int priority)=0;

	/// \brief Return all possible interpretations of a sencence that could not be eliminated by priorization
	virtual std::vector<SentenceGuess> analyzeSentence( const SentenceLexerInstanceInterface* lexer, const std::string& sentence) const=0;	
};

}//namespace
#endif

