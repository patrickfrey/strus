/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/reference.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/lib/error.hpp"
#include "strus/lib/filelocator.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/statsstorage.hpp"
#include "strus/lib/statsproc.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/statisticsStorageInterface.hpp"
#include "strus/statisticsStorageClientInterface.hpp"
#include "strus/statisticsBuilderInterface.hpp"
#include "strus/statisticsProcessorInterface.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/pseudoRandom.hpp"
#include "strus/base/math.hpp"
#include "strus/base/stdint.h"
#include "strus/base/configParser.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <cstdarg>
#include <cstdio>
#include <memory>

#undef STRUS_LOWLEVEL_DEBUG
#undef STRUS_GENERATE_READABLE_NAMES

static strus::PseudoRandom g_random;
static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::FileLocatorInterface* g_fileLocator = 0;
static strus::TimeStamp g_timestamp = 0;

class StlRandomGen
{
public:
	StlRandomGen(){}

	std::size_t operator()( std::size_t i)
	{
		return (std::size_t)g_random.get( 0, i);
	}
};

static std::string randomType()
{
	enum {NofTypes=5};
	static const char* ar[ NofTypes] = {"WORD","STEM","NUM","LOC","ORIG"};
	return ar[ g_random.get( 0, (unsigned int)NofTypes)];
}

static std::string randomStorage()
{
	enum {NofStorages=5};
	static const char* ar[ NofStorages] = {"S1","S2","S3","S4","S5"};
	return ar[ g_random.get( 0, (unsigned int)NofStorages)];
}

static std::string randomTerm()
{
	std::string rt;
	static const char* alphabet
		= {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};
	unsigned int val = g_random.get( 0, std::numeric_limits<int>::max());
	unsigned int sel = g_random.select( 11, 0, 2, 3, 4, 5, 6, 8, 10, 12, 14, 17);
	unsigned int le = sel == 0 ? g_random.get( 1, 20) : sel;
	unsigned int li = 0;
	for (; li < le; ++li)
	{
		unsigned int pf = (li * val) >> 8;
		unsigned int chidx = ((val^pf) % 52);
		rt.push_back( alphabet[chidx]);
	}
	StlRandomGen rnd;
	std::random_shuffle( rt.begin(), rt.end(), rnd);
	return rt;
}

class RandomStatisticsBlock
{
public:
	RandomStatisticsBlock( const RandomStatisticsBlock& o)
		:m_termstats(o.m_termstats),m_timestamp(o.m_timestamp),m_storageid(o.m_storageid),m_nofDocs(o.m_nofDocs){}
	RandomStatisticsBlock( size_t maxNofTerms)
	{
		if (maxNofTerms < 1)
		{
			throw std::runtime_error( "internal: bad number of terms");
		}
		std::set<std::string> termSet;
		size_t nofTerms = g_random.get( 0, g_random.get( 1, g_random.get( 1, maxNofTerms)));
		while (termSet.size() < nofTerms)
		{
			std::string type = randomType();
			std::string value = randomTerm();
			int dfchange = g_random.get( 0, g_random.get( 1, 100));
			if (termSet.insert( type + " " + value).second == true)
			{
				m_termstats.emplace_back( type, value, dfchange);
			}
		}
		m_timestamp = ++g_timestamp;
		m_storageid = randomStorage();
		m_nofDocs = g_random.get( 1, 1000);
	}

	struct TermStat
	{
		std::string type;
		std::string value;
		int dfchange;

		TermStat( const TermStat& o)
			:type(o.type),value(o.value),dfchange(o.dfchange){}
		TermStat( const std::string& t, const std::string& v, int d)
			:type(t),value(v),dfchange(d){}

		std::string tostring() const
		{
			std::ostringstream rt;
			rt << " " << type << " '" << value << "'";
			return rt.str();
		}
	};

	const std::string& storageid() const noexcept
	{
		return m_storageid;
	}
	const std::vector<TermStat>& termstats() const noexcept
	{
		return m_termstats;
	}
	int nofDocs() const noexcept
	{
		return m_nofDocs;
	}

private:
	std::vector<TermStat> m_termstats;
	strus::TimeStamp m_timestamp;
	std::string m_storageid;
	int m_nofDocs;
};

static unsigned int getUintValue( const char* arg)
{
	unsigned int rt = 0, prev = 0;
	char const* cc = arg;
	for (; *cc; ++cc)
	{
		if (*cc < '0' || *cc > '9') throw std::runtime_error( std::string( "parameter is not a non negative integer number: ") + arg);
		rt = (rt * 10) + (*cc - '0');
		if (rt < prev) throw std::runtime_error( std::string( "parameter out of range: ") + arg);
	}
	return rt;
}

static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " <config> <nofblocks> <maxblocksize>" << std::endl;
	std::cerr << "<config>       = statistics storage description" << std::endl;
	std::cerr << "<nofblocks>    = number of blocks to insert" << std::endl;
	std::cerr << "<maxblocksize> = maximum block size" << std::endl;
}

int main( int argc, const char* argv[])
{
	g_errorhnd = strus::createErrorBuffer_standard( stderr, 1, NULL/*debug trace interface*/);
	if (!g_errorhnd)
	{
		std::cerr << "construction of error buffer failed" << std::endl;
		return -1;
	}
	g_fileLocator = strus::createFileLocator_std( g_errorhnd);
	if (!g_fileLocator)
	{
		std::cerr << "construction of file locator failed" << std::endl;
		return -1;
	}

	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		printUsage( argc, argv);
		return 0;
	}
	else if (argc < 4)
	{
		std::cerr << "ERROR too few arguments" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	else if (argc > 4)
	{
		std::cerr << "ERROR too many arguments" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	try
	{
		std::string config = argv[1];
		unsigned int nofBlocks = getUintValue( argv[2]);
		unsigned int maxBlockSize = getUintValue( argv[3]);

		std::string configCopy = config;
		std::string path;
		if (!strus::extractStringFromConfigString( path, configCopy, "path", g_errorhnd))
		{
			throw std::runtime_error("bad configuration passed to test");
		}
		{
			strus::local_ptr<strus::DatabaseInterface> dbi( strus::createDatabaseType_leveldb( g_fileLocator, g_errorhnd));
			if (!dbi.get())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}
			strus::local_ptr<strus::StatisticsStorageInterface> sti( strus::createStatisticsStorageType_std( g_fileLocator, g_errorhnd));
			if (!sti.get() || g_errorhnd->hasError())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}
			(void)dbi->destroyDatabase( config); //... destroy leftover of previous test
			(void)g_errorhnd->fetchError(); //... ignore error

			sti->createStorage( config, dbi.get());
			const strus::StatisticsProcessorInterface* statisticsMessageProc = strus::createStatisticsProcessor_std( g_fileLocator, g_errorhnd);
			strus::local_ptr<strus::StatisticsStorageClientInterface>
				storage( sti->createClient( config, dbi.get(), statisticsMessageProc));
			if (!storage.get())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}
			std::vector<RandomStatisticsBlock> blockar;
			for (int ii=0; ii<nofBlocks; ++ii)
			{
				blockar.push_back( RandomStatisticsBlock( maxBlockSize));
			}
			strus::local_ptr<strus::StatisticsBuilderInterface> statbuilder( statisticsMessageProc->createBuilder( path));
			for (int ii=0; ii<nofBlocks; ++ii)
			{
				for (auto& ts : blockar[ii].termstats())
				{
					statbuilder->addDfChange( ts.type.c_str(), ts.value.c_str(), ts.dfchange);
				}
				statbuilder->addNofDocumentsInsertedChange( blockar[ii].nofDocs());
				statbuilder->commit();
			}
		}
		if (g_fileLocator) delete g_fileLocator;
		if (g_errorhnd) delete g_errorhnd;
		return 0;
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "ERROR " << e.what() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "EXCEPTION " << e.what() << std::endl;
	}
	delete g_fileLocator;
	delete g_errorhnd;
	return 4;
}


