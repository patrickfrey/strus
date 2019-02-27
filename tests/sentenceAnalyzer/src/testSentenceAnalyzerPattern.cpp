/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the sentence analyzer detecting sentence candidates in queries
#include "strus/lib/error.hpp"
#include "strus/vectorStorageInterface.hpp"
#include "strus/vectorStorageClientInterface.hpp"
#include "strus/vectorStorageTransactionInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "strus/sentenceLexerContextInterface.hpp"
#include "strus/sentenceLexerInstanceInterface.hpp"
#include "strus/base/configParser.hpp"
#include "strus/base/stdint.h"
#include "strus/base/fileio.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/pseudoRandom.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/regex.hpp"
#include "strus/reference.hpp"
#include "strus/sentenceAnalyzerInstanceInterface.hpp"
#include "strus/sentenceLexerInstanceInterface.hpp"
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <new>

#define STRUS_DBGTRACE_COMPONENT_NAME "test"

static bool g_verbose = false;
static strus::PseudoRandom g_random;
static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::DebugTraceInterface* g_dbgtrace = 0;

class TestSentenceLexerContext
	:public strus::SentenceLexerContextInterface
{
public:
	explicit TestSentenceLexerContext( const std::string& source_)
		:m_source(source_),m_sourceitr()
		,m_elements()
	{
		m_sourceitr = m_source.begin();
	}
	TestSentenceLexerContext( const TestSentenceLexerContext& o)
		:m_source(o.m_source),m_sourceitr()
		,m_elements(o.m_elements)
	{
		m_sourceitr = m_source.begin();
	}

	virtual ~TestSentenceLexerContext(){}

	virtual bool fetchFirstSplit()
	{
		m_sourceitr = m_source.begin();
		loadTerms();
	}

	virtual bool fetchNextSplit()
	{
		if (m_sourceitr != m_source.end())
		{
			m_sourceitr++;
			loadTerms();
		}
		return false;
	}

	virtual int nofTokens() const
	{
		return m_elements.size();
	}
	virtual std::string featureValue( int idx) const
	{
		return m_elements[idx].first;
	}
	virtual std::vector<std::string> featureTypes( int idx) const
	{
		return m_elements[idx].second;
	}

	virtual std::vector<strus::SentenceGuess> rankSentences( const std::vector<strus::SentenceGuess>& sentences, int maxNofResults) const
	{
		return sentences;
	}

private:
	enum CharacterClass {ClassLowerCase,ClassUpperCase,ClassConsonant,ClassDigit,ClassAlpha,ClassAlnum};
	const char* characterClassName( CharacterClass cl)
	{
		const char* ar[] = {"LO","UP","CONS","DIGIT","ALPHA","ALNUM"};
	}

	std::vector<CharacterClass> getCharacterClasses( char ch)
	{
		std::vector<CharacterClass> rt;
		if (ch >= 'a' && ch <= 'z')
		{
			rt.push_back( ClassLowerCase);
			rt.push_back( ClassAlpha);
			rt.push_back( ClassAlnum);
		}
		if (ch >= 'A' && ch <= 'Z')
		{
			rt.push_back( ClassUpperCase);
			rt.push_back( ClassAlpha);
			rt.push_back( ClassAlnum);
		}
		if (ch == '_')
		{
			rt.push_back( ClassAlpha);
			rt.push_back( ClassAlnum);
		}
		if (ch >= '0' && ch <= '9')
		{
			rt.push_back( ClassDigit);
			rt.push_back( ClassAlnum);
		}
		if (0!=std::strchr( "aeiouAEIOU", ch))
		{
			rt.push_back( ClassConsonant);
		}
		return rt;
	}
	typedef std::pair<std::string,std::vector<std::string> > Element;

	std::vector<std::string> characterClassNames( char ch)
	{
		std::vector<std::string> rt;
		std::vector<CharacterClass> cl = getCharacterClasses( ch);
		std::vector<CharacterClass>::const_iterator ci = cl.begin(), ce = cl.end();
		for (; ci != ce; ++ci)
		{
			rt.push_back( characterClassName( *ci));
		}
		return rt;
	}

	Element getElement( char ch)
	{
		return Element( std::string(&ch,1), characterClassNames( ch));
	}

	void loadTerms()
	{
		char ch;
		if (m_sourceitr != m_source.end())
		{
			ch = *m_sourceitr;
		}
		else
		{
			ch = '\0';
		}
		m_elements.clear();
		m_elements.push_back = getElement( ch);
	}

private:
	std::string m_source;
	std::string::const_iterator m_sourceitr;
	std::vector<Element> m_elements;
};


class TestSentenceLexerInstance
	:public strus::SentenceLexerInstanceInterface
{
public:
	TestSentenceLexerInstance(){}

	virtual ~TestSentenceLexerInstance(){}
	virtual void addSeparator( int){}
	virtual void addSpace( int){}
	virtual void addLink( int, char){}

	virtual SentenceLexerContextInterface* createContext( const std::string& source) const
	{
		return new TestSentenceLexerContext( source);
	}
};

class TestData
{
public:
	TestData()
		:m_sourcear(),m_patternar(),m_regexar(){}
	TestData( const TestData& o)
		:m_sourcear(o.m_sourcear),m_patternar(o.m_patternar),m_regexar(o.m_regexar){}

	void defineSource( const std::string& source)
	{
		m_sourcear.push_back( source);
	}
	void definePattern( const std::string& pattern)
	{
		m_patternar.push_back( pattern);
		m_regexar.push_back( new RegexSearch( pattern, 0, g_errorhnd));
		if (!m_regexar.back().get()) throw std::runtime_error(_TXT("failed to compile regex for pattern"));
	}

private:
	typedef Reference<RegexSearch> RegexSearchRef;
	std::vector<std::string> m_sourcear;
	std::vector<std::string> m_patternar;
	std::vector<RegexSearchRef> m_regexar;
	
};

class TestResults
{
public:
	TestResults(){}
};


strus::SentenceLexerInstanceInterface* createSentenceLexer()
{
	return 0;
}

void instantiateLexer( strus::SentenceLexerInstanceInterface* lexer)
{
}

void runQueries( TestResults& testResults, const strus::SentenceLexerInstanceInterface* lexer, const TestData& testData, int testidx)
{
	
}

void verifyTestResults( const TestResults& testResults, const TestData& testData, int testidx)
{
	
}


int main( int, const char**)
{
	try
	{
		int rt = 0;
		g_dbgtrace = strus::createDebugTrace_standard( 2);
		if (!g_dbgtrace) throw std::runtime_error("failed to create debug trace interface");
		g_errorhnd = strus::createErrorBuffer_standard( 0, 1, g_dbgtrace);
		if (!g_errorhnd) throw std::runtime_error("failed to create error buffer structure");

		bool printUsageAndExit = false;

		// Parse parameters:
		int argidx = 1;
		int testidx = -1;
		bool finished_options = false;
		while (!finished_options && argc > argidx && argv[argidx][0] == '-')
		{
			if (0==std::strcmp( argv[argidx], "-h"))
			{
				printUsageAndExit = true;
			}
			else if (0==std::strcmp( argv[argidx], "-V"))
			{
				g_verbose = true;
			}
			else if (0==std::strcmp( argv[argidx], "-T"))
			{
				if (argidx+1 == argc)
				{
					std::cerr << "option -T needs argument (test index)" << std::endl;
					printUsageAndExit = true;
				}
				try
				{
					testidx = strus::numstring_conv::touint( argv[++argidx], std::numeric_limits<int>::max());
				}
				catch (const std::runtime_error& err)
				{
					std::cerr << "error parsing option -T: " << err.what() << std::endl;
				}
			}
			else if (0==std::strcmp( argv[argidx], "-G"))
			{
				if (argidx+1 == argc)
				{
					std::cerr << "option -G needs argument (configuration string)" << std::endl;
					printUsageAndExit = true;
				}
				if (!g_dbgtrace->enable( argv[++argidx]))
				{
					std::cerr << "too many debug trace items enabled" << std::endl;
					printUsageAndExit = true;
				}
			}
			else if (0==std::strcmp( argv[argidx], "--"))
			{
				finished_options = true;
			}
			else
			{
				std::cerr << "unknown option " << argv[argidx] << std::endl;
				printUsageAndExit = true;
			}
			++argidx;
		}
		if (argc > argidx)
		{
			std::cerr << "too many arguments, no arguments expected)" << std::endl;
			rt = 1;
			printUsageAndExit = true;
		}

		if (printUsageAndExit)
		{
			std::cerr << "Usage: " << argv[0] << " [<options>]" << std::endl;
			std::cerr << "options:" << std::endl;
			std::cerr << "-h                     : print this usage" << std::endl;
			std::cerr << "-V                     : verbose output to stderr" << std::endl;
			std::cerr << "-T <QIDX>              :evaluate only test case with index <QIDX> starting with 0" << std::endl;
			std::cerr << "-G <DEBUG>             :enable debug trace for <DEBUG>" << std::endl;
			return rt;
		}
		// Insert the feature definitions:
		if (g_verbose) std::cerr << "create test data ..." << std::endl;
		TestData testData;

		// Creating the lexer:
		strus::local_ptr<strus::SentenceLexerInstanceInterface> lexer( createSentenceLexer());
		if (!lexer.get()) throw std::runtime_error( "failed to create test sentence lexer");
		instantiateLexer( lexer.get());

		// Run the tests:
		if (g_verbose) std::cerr << "run queries..." << std::endl;
		TestResults testResults;
		runQueries( testResults, lexer.get(), testData, testidx);
		if (g_verbose) std::cerr << "verify results ..." << std::endl;
		verifyTestResults( testResults, testData, testidx);

		// Debug output dump:
		if (!strus::dumpDebugTrace( g_dbgtrace, NULL/*filename (stderr)*/))
		{
			throw std::runtime_error( "failed to dump the debug trace");
		}
		std::cerr << "done" << std::endl;
		return 0;
	}
	catch (const std::runtime_error& err)
	{
		if (g_dbgtrace) (void)strus::dumpDebugTrace( g_dbgtrace, NULL/*filename (stderr)*/);

		std::string msg;
		if (g_errorhnd && g_errorhnd->hasError())
		{
			msg.append( " (");
			msg.append( g_errorhnd->fetchError());
			msg.append( ")");
		}
		std::cerr << "error: " << err.what() << msg << std::endl;
		return 1;
	}
	catch (const std::bad_alloc& )
	{
		std::cerr << "out of memory" << std::endl;
		return 2;
	}
	catch (const std::logic_error& err)
	{
		std::string msg;
		if (g_errorhnd && g_errorhnd->hasError())
		{
			msg.append( " (");
			msg.append( g_errorhnd->fetchError());
			msg.append( ")");
		}
		std::cerr << "error: " << err.what() << msg << std::endl;
		return 3;
	}
}
