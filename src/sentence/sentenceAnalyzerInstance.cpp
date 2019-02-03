/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the sentence analyzer detecting sentence candidates in queries
#include "sentenceAnalyzerInstance.hpp"
#include "strus/sentenceLexerInstanceInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <string>

using namespace strus;

#define MODULENAME "sentence analyzer"
#define STRUS_DBGTRACE_COMPONENT_NAME "sentence"

SentenceAnalyzerInstance::SentenceAnalyzerInstance( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_),m_debugtrace(0)
{
	DebugTraceInterface* dbgi = m_errorhnd->debugTrace();
	if (dbgi) m_debugtrace = dbgi->createTraceContext( STRUS_DBGTRACE_COMPONENT_NAME);
}

SentenceAnalyzerInstance::~SentenceAnalyzerInstance()
{
	if (m_debugtrace) delete m_debugtrace;
}

void SentenceAnalyzerInstance::pushTerm( const std::string& type, const std::string& name, int priority)
{
	try
	{
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push term: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushAlt( int argc, bool similarity, int priority)
{
	try
	{
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push alt: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushSequenceImm( int argc, int priority)
{
	try
	{
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push sequence immediate: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushRepeat( int times) const
{
	try
	{
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push repeat: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::defineSentence( const std::string& classname, int priority)
{
	try
	{
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in define sentence of '%s': %s"), MODULENAME, *m_errorhnd);
}

std::vector<SentenceGuess> SentenceAnalyzerInstance::analyzeSentence( const SentenceLexerInstanceInterface* lexer, const std::string& sentence) const
{
	try
	{
		std::vector<SentenceGuess> rt;
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in analyze sentence of '%s': %s"), MODULENAME, *m_errorhnd, std::vector<SentenceGuess>());
}



