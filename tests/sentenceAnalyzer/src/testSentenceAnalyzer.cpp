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
#include <set>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <new>
#include <cmath>

static bool g_verbose = false;
static strus::PseudoRandom g_random;
static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::DebugTraceInterface* g_dbgtrace = 0;
static int g_nofSamples = 1000;
static int g_nofInstances = 100;
static int g_nofFeatures = 100;
static int g_nofTerms = 20;
static int g_nofTypes = 20;

static int nextPrimeNumber( int xx)
{
	if ((xx % 2) == 0)
	{
		if (xx == 2) return 2;
		++xx;
		// ... xx is even, make it odd
	}
	for (;;)
	{
		int ii=3, ie=(int)std::sqrt(xx)+1;
		for (; ii < ie; ii += 2)
		{
			if (xx % ii == 0) break;
		}
		if (ii >= ie) return xx;
		xx += 2;
	}
}

class TestSentenceLexerData
{
public:
	TestSentenceLexerData()
		:m_types(),m_priorities(),m_instances(){}

	void init()
	{
		int ti = 0, te = g_random.get( 0, g_nofTypes);
		int pi = 1;
		for (; ti != te; ++ti)
		{
			pi = nextPrimeNumber( pi+1);
			m_types.push_back( pi);
			m_priorities.push_back( g_random.get( 0, g_random.get( 0, g_nofTypes)));
		}
		int ii = 0, ie = g_nofInstances;
		for (; ii != ie; ++ii)
		{
			std::vector<int> instance;
			int fi = 0, fe = g_random.get( 0, g_nofFeatures);
			for (; fi != fe; ++fi)
			{
				instance.push_back( g_random.get( 0, g_nofTerms));
			}
			m_instances.push_back( instance);
		}
	}

	const std::vector<int>& types() const			{return m_types;}
	const std::vector<int>& priorities() const		{return m_priorities;}
	const std::vector<std::vector<int> >& instances() const	{return m_instances;}

	std::string typeString( int typeidx) const
	{
		return strus::string_format( "t%d", typeidx);
	}
	std::string featureString( int featidx) const
	{
		return strus::string_format( "f%d", featidx);
	}
	int featureIndexFromString( const std::string& featstr) const
	{
		if (featstr[0] != 'f') return -1;
		return strus::numstring_conv::toint( featstr.c_str()+1, featstr.size()-1, std::numeric_limits<int>::max());
	}
	int typeIndexFromString( const std::string& typestr) const
	{
		if (typestr[0] != 't') return -1;
		return strus::numstring_conv::toint( typestr.c_str()+1, typestr.size()-1, std::numeric_limits<int>::max());
	}

	std::vector<std::pair<std::string,int> > typePriorityPairs() const
	{
		std::vector<std::pair<std::string,int> > rt;
		int ti = 0, te = m_types.size();
		for (; ti != te; ++ti)
		{
			rt.push_back( std::pair<std::string,int>( typeString( m_types[ ti]), m_priorities[ ti]));
		}
		return rt;
	}

	std::vector<int> featureTypes( int featidx) const
	{
		std::vector<int> rt;
		int ti = 0, te = m_types.size();
		for (; ti != te; ++ti)
		{
			if (featidx % m_types[ ti] == 0)
			{
				rt.push_back( m_types[ ti]);
			}
		}
		return rt;
	}

	std::vector<std::string> featureTypeStrings( int featidx) const
	{
		std::vector<int> tplist = featureTypes( featidx);
		std::vector<int>::const_iterator ti = tplist.begin(), te = tplist.end();
		std::vector<std::string> rt;
		for (; ti != te; ++ti)
		{
			rt.push_back( typeString( *ti));
		}
		return rt;
	}

	std::vector<std::string> getTopPriorityTypes( int featidx) const
	{
		std::vector<std::string> rt;
		std::vector<std::pair<std::string,int> > tpl = typePriorityPairs();
		std::vector<std::string> ftypes = featureTypeStrings( featidx);
		std::vector<std::string>::const_iterator fi = ftypes.begin(), fe = ftypes.end();
		int priority = -1;
		for (; fi != fe; ++fi)
		{
			std::vector<std::pair<std::string,int> >::const_iterator
				ti = tpl.begin(), te = tpl.end();
			for (; ti != te; ++ti)
			{
				if (ti->first == *fi && (priority < 0 || ti->second < priority))
				{
					priority = ti->second;
				}
			}
		}
		fi = ftypes.begin();
		for (; fi != fe; ++fi)
		{
			std::vector<std::pair<std::string,int> >::const_iterator
				ti = tpl.begin(), te = tpl.end();
			for (; ti != te; ++ti)
			{
				if (ti->first == *fi && ti->second == priority)
				{
					rt.push_back( *fi);
				}
			}
		}
		return rt;
	}

	std::size_t expectedResultSize() const
	{
		std::size_t rt = 0;
		std::vector<std::vector<int> >::const_iterator ii = m_instances.begin(), ie = m_instances.end();
		for (; ii != ie; ++ii)
		{
			std::size_t nn = 1;
			std::vector<int>::const_iterator ei = ii->begin(), ee = ii->end();
			for (; ei != ee; ++ei)
			{
				std::size_t factor = getTopPriorityTypes( *ei).size();
				nn *= (factor ? factor : 1);
			}
			rt += nn;
		}
		return rt;
	}

	void printSentenceTermList( std::ostream& out, const strus::SentenceTermList& tlist) const
	{
		strus::SentenceTermList::const_iterator
			ti = tlist.begin(), te = tlist.end();
		for (int tidx=0; ti != te; ++ti,++tidx)
		{
			out << strus::string_format( tidx?" %s '%s'":"%s '%s'", ti->type().c_str(), ti->value().c_str());
		}
		
	}

	void printSentenceGuess( std::ostream& out, const strus::SentenceGuess& guess) const
	{
		out << strus::string_format( "%.3f ", guess.weight());
		printSentenceTermList( out, guess.terms());
	}

	void printResult( std::ostream& out, const std::vector<strus::SentenceGuess>& glist) const
	{
		std::vector<strus::SentenceGuess>::const_iterator gi = glist.begin(), ge = glist.end();
		for (int gidx=0; gi != ge; ++gi,++gidx)
		{
			out << strus::string_format( "[%d] ", gidx);
			printSentenceGuess( out, *gi);
			out << std::endl;
		}
	}

	void verifyResult( const std::vector<strus::SentenceGuess>& glist) const
	{
		// Test size:
		std::size_t esize = expectedResultSize();
		if (glist.size() != esize)
		{
			throw std::runtime_error( strus::string_format( "expected size %d of result guesses %d does not match", (int)esize, (int)glist.size()));
		}
		// Test random samples:
		int si = 0, se = g_nofSamples;
		for (; si != se; ++si)
		{
			strus::SentenceTermList sample;
			int iidx = g_random.get( 0, m_instances.size());
			std::vector<int>::const_iterator
				fi = m_instances[ iidx].begin(), fe = m_instances[ iidx].end();
			for (; fi != fe; ++fi)
			{
				std::string featstr = featureString( *fi);
				std::vector<std::string> typeCandidates = getTopPriorityTypes( *fi);
				std::string typestr = typeCandidates.empty()
							? std::string()
							: typeCandidates[ g_random.get( 0, typeCandidates.size())];
				sample.push_back( strus::SentenceTerm( typestr, featstr));
			}
			if (g_verbose)
			{
				std::cerr << strus::string_format( "verifying picked expected [%d] ", si);
				printSentenceTermList( std::cerr, sample);
				std::cerr << std::endl;
			}
			std::vector<strus::SentenceGuess>::const_iterator
				gi = glist.begin(), ge = glist.end();
			for (; gi != ge && gi->terms() != sample; ++gi){}
			if (gi == ge)
			{
				std::ostringstream buf;
				printSentenceTermList( buf, sample);
				std::string samplestr = buf.str();
				throw std::runtime_error(
					strus::string_format(
						"expected sample term list {%s} not found in results",
						samplestr.c_str()));
			}
		}
		for (si = 0; si != se; ++si)
		{
			const strus::SentenceTermList& sample = glist[ g_random.get( 0, glist.size())].terms();
			if (g_verbose)
			{
				std::cerr << strus::string_format( "verifying random result picked [%d] ", si);
				printSentenceTermList( std::cerr, sample);
				std::cerr << std::endl;
			}
			std::vector<int> sampleFeatures;
			strus::SentenceTermList::const_iterator ti = sample.begin(), te = sample.end();
			for (; ti != te; ++ti)
			{
				sampleFeatures.push_back( featureIndexFromString( ti->value()));
				if (ti->type().empty())
				{
					std::vector<int>::const_iterator xi = m_types.begin(), xe = m_types.end();
					for (; xi != xe && sampleFeatures.back() % *xi != 0; ++xi){}
					if (xi != xe)
					{
						throw std::runtime_error("result type should not be defined");
					}
				}
				else
				{
					int sampleTypeidx = typeIndexFromString( ti->type());
					if (sampleTypeidx == -1 || sampleFeatures.back() % sampleTypeidx != 0)
					{
						throw std::runtime_error("result type does not match");
					}
				}
			}
			std::vector<std::vector<int> >::const_iterator
				ii = m_instances.begin(), ie = m_instances.end();
			for (; ii != ie && *ii != sampleFeatures; ++ii){}
			if (ii == ie)
			{
				std::ostringstream buf;
				printSentenceTermList( buf, sample);
				std::string samplestr = buf.str();
				throw std::runtime_error(
					strus::string_format(
						"result {%s} not found in expected term list", samplestr.c_str()));
			}
		}
	}

private:
	std::vector<int> m_types;
	std::vector<int> m_priorities;
	std::vector<std::vector<int> > m_instances;
};

static TestSentenceLexerData g_lexerData;


class TestSentenceLexerContext
	:public strus::SentenceLexerContextInterface
{
public:
	explicit TestSentenceLexerContext()
		:m_instanceidx(0)

	{}

	TestSentenceLexerContext( const TestSentenceLexerContext& o)
		:m_instanceidx(o.m_instanceidx)
	{}

	virtual ~TestSentenceLexerContext(){}

	virtual bool fetchFirstSplit()
	{
		m_instanceidx = 0;
		return m_instanceidx < g_lexerData.instances().size();
	}
	virtual bool fetchNextSplit()
	{
		++m_instanceidx;
		return m_instanceidx < g_lexerData.instances().size();
	}
	virtual int nofTokens() const
	{
		return g_lexerData.instances()[ m_instanceidx].size();
	}
	virtual std::string featureValue( int idx) const
	{
		return g_lexerData.featureString( g_lexerData.instances()[ m_instanceidx][ idx]);
	}
	virtual std::vector<std::string> featureTypes( int idx) const
	{
		return g_lexerData.featureTypeStrings( g_lexerData.instances()[ m_instanceidx][ idx]);
	}
	virtual std::vector<strus::SentenceGuess> rankSentences( const std::vector<strus::SentenceGuess>& sentences, int maxNofResults) const
	{
		return sentences;
	}

private:
	std::size_t m_instanceidx;
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
	virtual void defineGroupSimilarityDistance( double){}

	virtual strus::SentenceLexerContextInterface* createContext( const std::string& ) const
	{
		return new TestSentenceLexerContext();
	}
};


void testSentenceAnalyer()
{
	strus::local_ptr<strus::SentenceAnalyzerInstanceInterface>
		analyzer( strus::createSentenceAnalyzerInstance_std( g_errorhnd));
	if (!analyzer.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	std::vector<std::pair<std::string,int> > typePriorityPairs = g_lexerData.typePriorityPairs();
	std::vector<std::pair<std::string,int> >::const_iterator
		ti = typePriorityPairs.begin(), te = typePriorityPairs.end();
	for (; ti != te; ++ti)
	{
		analyzer->defineWordType( ti->first, ti->second);
	}
	// Run the test:
	TestSentenceLexerInstance lexer;
	std::vector<strus::SentenceGuess> resultar = analyzer->analyzeSentence( &lexer, std::string(), std::numeric_limits<int>::max(), 0.0);
	if (g_verbose) g_lexerData.printResult( std::cerr, resultar);
	g_lexerData.verifyResult( resultar);
}

void printUsage( std::ostream& out)
{
	out << "Usage: testSentenceAnalyzer [<options>] <inst> <feats> <terms> <types>" << std::endl;
	out << "options:" << std::endl;
	out << "-h                     : print this usage" << std::endl;
	out << "-V                     : verbose output to stderr" << std::endl;
	out << "-G <DEBUG>             : enable debug trace for <DEBUG>" << std::endl;
	out << "<inst>                 : number of instances" << std::endl;
	out << "<feats>                : number of features" << std::endl;
	out << "<terms>                : number of terms" << std::endl;
	out << "<types>                : number of types" << std::endl;
}

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
		if (argidx + 4 > argc)
		{
			printUsage( std::cerr);
			throw std::runtime_error( "too few arguments");
		}
		if (argidx + 4 < argc)
		{
			printUsage( std::cerr);
			throw std::runtime_error( "too many arguments");
		}
		if (printUsageAndExit)
		{
			printUsage( std::cout);
			return rt;
		}
		g_nofInstances = 100;
		g_nofFeatures = 100;
		g_nofTerms = 20;
		g_nofTypes = 20;
		int ai = 0;
		g_nofInstances = strus::numstring_conv::toint( argv[ argidx+ai], std::strlen(argv[ argidx+ai]), std::numeric_limits<int>::max());
		++ai;
		g_nofFeatures = strus::numstring_conv::toint( argv[ argidx+ai], std::strlen(argv[ argidx+ai]), std::numeric_limits<int>::max());
		++ai;
		g_nofTerms = strus::numstring_conv::toint( argv[ argidx+ai], std::strlen(argv[ argidx+ai]), std::numeric_limits<int>::max());
		++ai;
		g_nofTypes = strus::numstring_conv::toint( argv[ argidx+ai], std::strlen(argv[ argidx+ai]), std::numeric_limits<int>::max());
		++ai;
		g_nofSamples = g_nofInstances * 10;
		g_lexerData.init();

		testSentenceAnalyer();

		// Debug output dump:
		if (!strus::dumpDebugTrace( g_dbgtrace, NULL/*filename (stderr)*/))
		{
			throw std::runtime_error( "failed to dump the debug trace");
		}
		if (g_errorhnd->hasError())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
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
