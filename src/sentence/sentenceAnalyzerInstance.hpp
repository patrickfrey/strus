/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the sentence analyzer detecting sentence candidates in queries
#ifndef _STRUS_SENTENCE_ANALYZER_INSTANCE_IMPL_HPP_INCLUDED
#define _STRUS_SENTENCE_ANALYZER_INSTANCE_IMPL_HPP_INCLUDED
#include "strus/sentenceAnalyzerInstanceInterface.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class DebugTraceContextInterface;

/// \brief Implementation of the sentence analyzer detecting sentence candidates in queries
class SentenceAnalyzerInstance
	:public SentenceAnalyzerInstanceInterface
{
public:
	SentenceAnalyzerInstance( ErrorBufferInterface* errorhnd_);

	virtual ~SentenceAnalyzerInstance();

	virtual void pushTerm( const std::string& type, const std::string& name, int priority);

	virtual void pushAlt( int argc, bool similarity, int priority);

	virtual void pushSequenceImm( int argc, int priority);

	virtual void pushRepeat( int times) const;

	virtual void defineSentence( const std::string& classname, int priority);

	virtual std::vector<SentenceGuess> analyzeSentence( const SentenceLexerInstanceInterface* lexer, const std::string& sentence) const;

private:
	ErrorBufferInterface* m_errorhnd;
	DebugTraceContextInterface* m_debugtrace;
};

}//namespace
#endif

