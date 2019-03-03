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
#include "strus/sentenceGuess.hpp"
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
	/// \param[in] type type string of the term to match or an empty string for any type accepted
	/// \param[in] name regular expression of the term to match or an empty string for any value accepted
	/// \param[in] weight weight of the pick (greater is better), value between 0.0 and 1.0
	virtual void pushTerm( const std::string& type, const std::string& name, float weight)=0;

	/// \brief Push a null symbol on the stack for describing a state switch to the follow state without consuming a symbol
	/// \param[in] weight weight of the pick (greater is better), value between 0.0 and 1.0
	virtual void pushNone( float weight)=0;

	/// \brief Define the top elements (specified as number of elements) on the stack as alternatives of a choice
	/// \param[in] argc number of elements to group together as selection of alternatives
	virtual void pushAlt( int argc)=0;

	/// \brief Define the top elements (specified as number of elements) on the stack as sequence of immediately succeeding elements to appear in this order
	/// \param[in] argc number of elements to group together as sequence of immediately succeeding elements
	virtual void pushSequenceImm( int argc)=0;

	/// \brief Define the top element on the stack to be a repeating structure
	/// \param[in] mintimes minimum number of repetitions
	/// \param[in] maxtimes maximum number of repetitions or -1 for unlimited
	/// \note number of repetitions is between 0 and times, e.g. equivalent to {1,3} or '*' for 0,-1
	virtual void pushRepeat( int mintimes, int maxtimes)=0;

	/// \brief Define the last element on the stack as sentence structure
	/// \param[in] classname name given to the structure
	/// \param[in] weight weight of the structure (greater is better), value between 0.0 and 1.0
	virtual void defineSentence( const std::string& classname, float weight)=0;

	/// \brief Complete definition of all the patterns defined
	/// \return true on success
	virtual bool compile()=0;

	/// \brief Analyze a sentence and return the best ranked candidates with weight and class name
	/// \param[in] lexer lexer instance
	/// \param[in] source source to parse
	/// \param[in] maxNofResults maximum number of results to return
	virtual std::vector<SentenceGuess> analyzeSentence( const SentenceLexerInstanceInterface* lexer, const std::string& source, int maxNofResults) const=0;	
};

}//namespace
#endif

