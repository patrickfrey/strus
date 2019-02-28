/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the sentence analyzer detecting sentence candidates in queries
#include "strus/lib/error.hpp"
#include "strus/lib/sentence.hpp"
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
#define MAX_NOF_RESULTS 1024
#define MAX_NOF_PATTERNS 128
#define MAX_NOF_SOURCES 128

static bool g_verbose = false;
static strus::PseudoRandom g_random;
static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::DebugTraceInterface* g_dbgtrace = 0;

class TestSentenceLexerContext
	:public strus::SentenceLexerContextInterface
{
public:
	explicit TestSentenceLexerContext( const std::string& source_)
		:m_sourcear(),m_sourceitr(),m_elements()
	{
		m_sourcear.push_back( source_);
		m_sourceitr = m_sourcear.begin();
	}
	explicit TestSentenceLexerContext( const std::vector<std::string>& sources_)
		:m_sourcear(sources_),m_sourceitr(),m_elements()
	{
		m_sourceitr = m_sourcear.begin();
	}
	explicit TestSentenceLexerContext( const char** sources_)
		:m_sourcear(),m_sourceitr(),m_elements()
	{
		char const** si = sources_;
		for (; *si; ++si)
		{
			m_sourcear.push_back( *si);
		}
		m_sourceitr = m_sourcear.begin();
	}
	TestSentenceLexerContext( const TestSentenceLexerContext& o)
		:m_sourcear(o.m_sourcear),m_sourceitr(),m_elements(o.m_elements)
	{
		m_sourceitr = m_sourcear.begin();
	}

	virtual ~TestSentenceLexerContext(){}

	virtual bool fetchFirstSplit()
	{
		m_sourceitr = m_sourcear.begin();
		loadTerms();
		return m_sourceitr != m_sourcear.end();
	}

	virtual bool fetchNextSplit()
	{
		if (m_sourceitr == m_sourcear.end()) return false;
		++m_sourceitr;
		loadTerms();
		return (m_sourceitr == m_sourcear.end());
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

public:
	enum CharacterClass {ClassLowerCase,ClassUpperCase,ClassConsonant,ClassDigit,ClassAlpha,ClassAlnum};
	static const char* characterClassName( CharacterClass cl)
	{
		const char* ar[] = {"LO","UP","CONS","DIGIT","ALPHA","ALNUM"};
		return ar[ cl];
	}

	static std::vector<CharacterClass> getCharacterClasses( char ch)
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

	static std::vector<std::string> characterClassNames( char ch)
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

private:
	Element getElement( char ch)
	{
		return Element( std::string(&ch,ch ? 1:0), characterClassNames( ch));
	}

	void loadTerms()
	{
		m_elements.clear();
		if (m_sourceitr != m_sourcear.end())
		{
			std::string::const_iterator si = m_sourceitr->begin(), se = m_sourceitr->end();
			for (; si != se; ++si)
			{
				m_elements.push_back( getElement( *si));
			}
		}
	}

private:
	std::vector<std::string> m_sourcear;
	std::vector<std::string>::const_iterator m_sourceitr;
	std::vector<Element> m_elements;
};

static std::vector<std::string> splitSource( const std::string& source)
{
	if (source.empty()) return std::vector<std::string>();
	std::vector<std::string> rt;
	char const* si = source.c_str();
	char splitchr = *si++;
	while (*si)
	{
		const char* start = si;
		for (++si; *si && *si != splitchr; ++si){}
		rt.push_back( std::string( start, si-start));
		if (*si == splitchr)
		{
			++si;
			if (!*si) rt.push_back( std::string());
		}
	}
	return rt;
}

static std::string joinSource( char const** src)
{
	std::string rt;
	for (;*src; ++src)
	{
		rt.push_back( ',');
		rt.append( *src);
	}
	return rt;
}

class TestSentenceLexerInstance
	:public strus::SentenceLexerInstanceInterface
{
public:
	TestSentenceLexerInstance(){}

	virtual ~TestSentenceLexerInstance(){}
	virtual void addSeparator( int){}
	virtual void addSpace( int){}
	virtual void addLink( int, char){}

	virtual strus::SentenceLexerContextInterface* createContext( const std::string& source) const
	{
		return new TestSentenceLexerContext( splitSource( source));
	}
};


static void instantiateAnalyzerRegex( strus::SentenceAnalyzerInstanceInterface* sa, const char* regex)
{
	char const* ri = regex;
	struct StackElem
	{
		int argcnt;

		StackElem() :argcnt(0){}
		StackElem( const StackElem& o) :argcnt(o.argcnt){}
	};
	int argcnt = 0;
	int altcnt = 0;

	while (*ri)
	{
		if (*ri == '[')
		{
			++ri;
			std::vector<std::string> elements;
			int ridx = 0;
			for (; *ri && *ri != ']'; ++ri,++ridx)
			{
				if (*ri == '-') throw std::runtime_error("dash not allowed for test regex");
				if (*ri == '\\') throw std::runtime_error("backslash not allowed for test regex");
				if (*ri == '[') throw std::runtime_error("unexpected open bracket '[' in test regex");
				std::vector<std::string> types = TestSentenceLexerContext::characterClassNames( *ri);
				std::string selected_type = types.empty() ? std::string() : types[ g_random.get( 0, types.size())];
				std::string feat( ri, 1);
				sa->pushTerm( selected_type, feat, 1.0);
			}
			if (*ri != ']') throw std::runtime_error("unexpected end of square bracket '[' ']' selection");
			++ri;
			sa->pushAlt( ridx);
			++argcnt;
		}
		else if (*ri == ']')
		{
			throw std::runtime_error("unexpected close bracket ']' in test regex");
		}
		else if (*ri == '(')
		{
			++ri;
			const char* start = ri;
			int bcnt = 1;
			for (; *ri && bcnt; ++ri)
			{
				if (*ri == '(') ++bcnt;
				else if (*ri == ')') --bcnt;
			}
			if (bcnt) throw std::runtime_error("unexpected end of regex in oval bracket '(' ')' expression");
			std::string subexpression( start, ri-start-1);
			instantiateAnalyzerRegex( sa, subexpression.c_str());
			++argcnt;
		}
		else if (*ri == '|')
		{
			if (argcnt == 0)
			{
				sa->pushNone( 1.0);
			}
			else if (argcnt > 1 || g_random.get( 0, 5) == 1)
			{
				sa->pushSequenceImm( argcnt);
			}
			++altcnt;
			argcnt = 1;
		}
		else
		{
			std::vector<std::string> types = TestSentenceLexerContext::characterClassNames( *ri);
			std::string selected_type = types.empty() ? std::string() : types[ g_random.get( 0, types.size())];
			std::string feat( ri, 1);
			sa->pushTerm( selected_type, feat, 1.0);
			++argcnt;
		}
	}
	if (argcnt > 1 ||  g_random.get( 0, 5) == 1)
	{
		sa->pushSequenceImm( argcnt);
	}
	if (altcnt > 0 || g_random.get( 0, 5) == 1)
	{
		sa->pushAlt( altcnt+1);
	}
}

struct PatternDescr
{
	const char* regex;
	const char* name;
};


static strus::SentenceAnalyzerInstanceInterface* createSentenceAnalyzer( const PatternDescr* patternar)
{
	strus::Reference<strus::SentenceAnalyzerInstanceInterface> rt( strus::createSentenceAnalyzerInstance_std( g_errorhnd));
	for (PatternDescr const* ri = patternar; ri->regex && ri->name; ++ri)
	{
		instantiateAnalyzerRegex( rt.get(), ri->regex);
		rt->defineSentence( ri->name, 1.0);
	}
	return rt.get();
}

class TestData
{
public:
	TestData( const PatternDescr* patternar, const char** sourcear)
		:m_sourcear(),m_joinsource(joinSource(sourcear)),m_namear(),m_patternar(),m_regexar(),m_analyzer(createSentenceAnalyzer(patternar))
	{
		if (!m_analyzer.get())
		{
			throw std::runtime_error("failed to create sentence analyzer");
		}
		if (!m_analyzer->compile())
		{
			throw std::runtime_error("failed to compile sentence analyzer");
		}
		char const** si = sourcear;
		for (; *si; ++si)
		{
			m_sourcear.push_back( *si);
		}
		PatternDescr const* pi = patternar;
		for (; pi->name && pi->regex; ++pi)
		{
			m_namear.push_back( pi->name);
			m_patternar.push_back( pi->regex);
			m_regexar.push_back( new strus::RegexSearch( pi->regex, 0, g_errorhnd));
			if (!m_regexar.back().get()) throw std::runtime_error( "failed to compile regex for pattern");
		}
	}
	TestData( const TestData& o)
		:m_sourcear(o.m_sourcear),m_patternar(o.m_patternar),m_regexar(o.m_regexar),m_analyzer(o.m_analyzer){}

	void run() const
	{
		TestSentenceLexerInstance lexer;
		std::vector<strus::SentenceGuess> result = m_analyzer->analyzeSentence( &lexer, m_joinsource, MAX_NOF_RESULTS);
		std::vector<strus::SentenceGuess> expected;
	}

private:
	std::vector<std::string> m_sourcear;
	std::string m_joinsource;
	std::vector<std::string> m_namear;
	std::vector<std::string> m_patternar;
	std::vector<strus::Reference<strus::RegexSearch> > m_regexar;
	strus::Reference<strus::SentenceAnalyzerInstanceInterface> m_analyzer;
};


struct TestDescr
{
	PatternDescr pattern[ MAX_NOF_PATTERNS];
	const char* sources[ MAX_NOF_SOURCES];
};

static TestDescr test1 =
{
	{{"[Abc]","Abc"},{0,0}},
	{"A",0}
};

int main( int argc, const char** argv)
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
			std::cerr << "-G <DEBUG>             :enable debug trace for <DEBUG>" << std::endl;
			return rt;
		}
		// Insert the feature definitions:
		if (g_verbose) std::cerr << "create test data ..." << std::endl;
		TestData testData1( test1.pattern, test1.sources);

		// Run the tests:
		if (g_verbose) std::cerr << "run tests ..." << std::endl;
		testData1.run();

		// Debug output dump:
		if (!strus::dumpDebugTrace( g_dbgtrace, NULL/*filename (stderr)*/))
		{
			throw std::runtime_error( "failed to dump the debug trace");
		}
		std::cerr << "OK" << std::endl;
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
