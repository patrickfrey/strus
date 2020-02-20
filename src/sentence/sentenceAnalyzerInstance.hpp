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
#include "strus/base/regex.hpp"
#include "strus/reference.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <map>

namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class DebugTraceContextInterface;
/// \brief Forward declaration
class SentenceLexerContextInterface;


/// \brief Implementation of the sentence analyzer detecting sentence candidates in queries
class SentenceAnalyzerInstance
	:public SentenceAnalyzerInstanceInterface
{
public:
	explicit SentenceAnalyzerInstance( ErrorBufferInterface* errorhnd_);

	virtual void defineWordType(
			const std::string& typenam,
			int priority);

	virtual std::vector<SentenceGuess> analyzeSentence(
			const SentenceLexerInstanceInterface* lexer,
			const std::string& source,
			int maxNofResults,
			double minWeight) const;

private:
	ErrorBufferInterface* m_errorhnd;
	DebugTraceContextInterface* m_debugtrace;
	std::map<std::string,int> m_typeprioritymap;
};

}//namespace
#endif


