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
#include "strus/sentenceLexerContextInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/math.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <limits>
#include <iostream>

using namespace strus;

#define MODULENAME "sentence analyzer"
#define STRUS_DBGTRACE_COMPONENT_NAME "sentence"
#undef STRUS_LOWLEVEL_DEBUG

SentenceAnalyzerInstance::SentenceAnalyzerInstance( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_),m_debugtrace(0),m_typeprioritymap()
{
	DebugTraceInterface* dbgi = m_errorhnd->debugTrace();
	if (dbgi) m_debugtrace = dbgi->createTraceContext( STRUS_DBGTRACE_COMPONENT_NAME);
}

void SentenceAnalyzerInstance::defineType(
		const std::string& typenam,
		int priority)
{
	try
	{
		m_typeprioritymap[ typenam] = priority;
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error defining sentence analyzer type in '%s': %s"), MODULENAME, *m_errorhnd);
}

static std::string termListString( const SentenceTermList& terms, const char* sep)
{
	std::string rt;
	SentenceTermList::const_iterator ti = terms.begin(), te = terms.end();
	for (; ti != te; ++ti)
	{
		if (!rt.empty()) rt.append( sep);
		rt.append( strus::string_format( "%s '%s'", ti->type().c_str(), ti->value().c_str()));
	}
	return rt;
}

std::vector<SentenceGuess> SentenceAnalyzerInstance::analyzeSentence( const SentenceLexerInstanceInterface* lexer, const std::string& source, int maxNofResults, double minWeight) const
{
	try
	{
		std::vector<SentenceGuess> candidates;

		strus::local_ptr<SentenceLexerContextInterface> lexerctx( lexer->createContext( source));
		if (!lexerctx.get()) throw std::runtime_error(_TXT("failed to create lexer context"));

		bool more = lexerctx->fetchFirstSplit();
		for (; more; more = lexerctx->fetchNextSplit())
		{
			std::vector<SentenceGuess> samples;
			samples.push_back( SentenceGuess( SentenceTermList(), 1.0));

			int ki = 0, ke = lexerctx->nofTokens();
			for (; ki != ke; ++ki)
			{
				std::string feat = lexerctx->featureValue( ki);
				std::vector<std::string> types = lexerctx->featureTypes( ki);
		
				int priority = -1;
				std::vector<std::string>::const_iterator ti = types.begin(), te = types.end();
				for (; ti != te; ++ti)
				{
					std::map<std::string,int>::const_iterator pi = m_typeprioritymap.find( *ti);
					if (pi != m_typeprioritymap.end() && (priority < 0 || priority > pi->second))
					{
						priority = pi->second;
					}
				}
				if (priority == -1)
				{
					std::vector<SentenceGuess> newSamples;
					std::vector<SentenceGuess>::iterator
						xi = samples.begin(), xe = samples.end();
					for (; xi != xe; ++xi)
					{
						SentenceTermList tl = xi->terms();
						tl.push_back( SentenceTerm( "", feat));
						newSamples.push_back( SentenceGuess( tl, xi->weight()));
					}
					samples.swap( newSamples);
				}
				else
				{
					std::vector<SentenceGuess> newSamples;
					ti = types.begin();
					for (; ti != te; ++ti)
					{
						std::map<std::string,int>::const_iterator pi = m_typeprioritymap.find( *ti);
						if (pi != m_typeprioritymap.end() && priority == pi->second)
						{
							std::vector<SentenceGuess>::const_iterator
								xi = samples.begin(), xe = samples.end();
							for (; xi != xe; ++xi)
							{
								SentenceTermList tl = xi->terms();
								tl.push_back( SentenceTerm( *ti, feat));
								newSamples.push_back( SentenceGuess( tl, xi->weight()));
							}
						}
					}
					samples.swap( newSamples);
				}
			}
			candidates.insert( candidates.end(), samples.begin(), samples.end());
		}
		std::vector<SentenceGuess>  rt = lexerctx->rankSentences( candidates, maxNofResults);
		std::vector<SentenceGuess>::const_iterator ri = rt.begin(), re = rt.end();

		int ridx = 0;
		for (; ri != re && ri->weight() + std::numeric_limits<float>::epsilon() >= minWeight; ++ri,++ridx){}
		rt.resize( ridx);
		if (m_errorhnd->hasError())
		{
			throw std::runtime_error( m_errorhnd->fetchError());
		}
		if (m_debugtrace)
		{
			m_debugtrace->open( "result");
			ri = rt.begin();
			for (; ri != re; ++ri)
			{
				std::string termliststr = termListString( ri->terms(), ", ");
				m_debugtrace->event( "guess", "%.5f '%s'", ri->weight(), termliststr.c_str());
			}
			m_debugtrace->close();
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in analyze sentence of '%s': %s"), MODULENAME, *m_errorhnd, std::vector<SentenceGuess>());
}



