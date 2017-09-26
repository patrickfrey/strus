/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/lib/statsproc.hpp"
#include "strus/lib/error.hpp"
#include "strus/statisticsProcessorInterface.hpp"
#include "strus/statisticsViewerInterface.hpp"
#include "strus/statisticsBuilderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/local_ptr.hpp"
#include "random.hpp"
#include <memory>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <limits>
#include <cstring>
#include <ctime>
#include <cmath>
#include <iomanip>
#include "strus/base/stdint.h"
#include <stdarg.h>

#undef STRUS_LOWLEVEL_DEBUG

static strus::Random g_random;
static strus::ErrorBufferInterface* g_errorhnd = 0;

class StlRandomGen
{
public:
	StlRandomGen(){}

	std::size_t operator()( std::size_t i)
	{
		return (std::size_t)g_random.get( 0, i);
	}
};


static const char* randomType()
{
	enum {NofTypes=5};
	static const char* ar[ NofTypes] = {"WORD","STEM","NUM","LOC","ORIG"};
	return ar[ g_random.get( 0, (unsigned int)NofTypes-1)];
}

static std::string randomTerm()
{
	std::string rt;
	static const char* alphabet
		= {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};
	unsigned int val = g_random.get( 0, std::numeric_limits<int>::max());
	unsigned int le = g_random.get( 1, 20, 10, 2, 3, 4, 5, 6, 8, 10, 12, 14, 17);
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

struct TermCollection
{
	TermCollection( unsigned int nofTerms, unsigned int diffrange)
	{
		if (nofTerms < 1)
		{
			std::cerr << "ERROR number of distinct terms in the collection has to be at least 1" << std::endl;
			nofTerms = 1;
		}
		std::set<Term> termset;
		while (termset.size() < nofTerms)
		{
			unsigned int ti = termset.size();
			for (; ti < nofTerms; ++ti)
			{
				int diff = g_random.get( 0, (int)diffrange) - (int)diffrange/2;
				termset.insert( Term( randomType(), randomTerm(), diff));
			}
		}
		std::set<Term>::const_iterator ti = termset.begin(), te = termset.end();
		for (; ti != te; ++ti)
		{
			termar.push_back( *ti);
		}
	}

	struct Term
	{
		std::string type;
		std::string value;
		int diff;

		Term( const Term& o)
			:type(o.type),value(o.value),diff(o.diff){}
		Term( const std::string& type_, const std::string& value_, int diff_)
			:type(type_),value(value_),diff(diff_){}

		std::string tostring() const
		{
			std::ostringstream rt;
			rt << " " << type << " '" << value << "' " << diff;
			return rt.str();
		}

		bool operator <( const Term& o) const
		{
			if (type > o.type) return false;
			if (type < o.type) return true;
			if (value > o.value) return false;
			if (value < o.value) return true;
			return false;
		}
	};

	std::vector<Term> termar;
};


static std::string doubleToString( double val_)
{
	unsigned int val = (unsigned int)::floor( val_ * 1000);
	unsigned int val_sec = val / 10000;
	unsigned int val_ms = val % 10000;
	std::ostringstream val_str;
	val_str << val_sec << "." << std::setfill('0') << std::setw(4) << val_ms;
	return val_str.str();
}

static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " <nofterms> <diffrange>" << std::endl;
	std::cerr << "<nofterms>      = number of distinct terms" << std::endl;
	std::cerr << "<diffrange>     = maximum diff" << std::endl;
}

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

int main( int argc, const char* argv[])
{
	g_errorhnd = strus::createErrorBuffer_standard( stderr, 1);
	if (!g_errorhnd) return -1;

	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		printUsage( argc, argv);
		return 0;
	}
	else if (argc < 3)
	{
		std::cerr << "ERROR too few parameters" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	else if (argc > 3)
	{
		std::cerr << "ERROR too many parameters" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	try
	{
		int nofDocs = (int)g_random.get( 0, 1000000) - 500000;
		unsigned int nofTerms = getUintValue( argv[1]);
		unsigned int diffRange = getUintValue( argv[2]);
		TermCollection collection( nofTerms, diffRange);

		strus::local_ptr<strus::StatisticsProcessorInterface> pmp( strus::createStatisticsProcessor( g_errorhnd));
		strus::local_ptr<strus::StatisticsBuilderInterface> builder( pmp->createBuilder());
		if (!builder.get())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		builder->setNofDocumentsInsertedChange( nofDocs);

		typedef TermCollection::Term Term;
		unsigned int termsByteSize = 0;
		std::clock_t start = std::clock();
		std::vector<Term>::const_iterator ti = collection.termar.begin(), te = collection.termar.end();
		for (; ti != te; ++ti)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "add df change '" << ti->type << "' '" << ti->value << "' " << ti->diff << std::endl;
#endif
			termsByteSize += ti->type.size() + ti->value.size() + 5;
			builder->addDfChange( ti->type.c_str(), ti->value.c_str(), ti->diff);
		}
		const void* msgblk;
		std::size_t msgblksize;
		std::size_t blobsize = 0;
		int nofDocsInserted = 0;
		if (!builder->fetchMessage( msgblk, msgblksize))
		{
			if (g_errorhnd->hasError())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		int blockcnt = 0;
		std::cerr << "fetch message " << ++blockcnt << " " << msgblksize << std::endl;
#endif
		blobsize += msgblksize;
		double duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		std::cerr << "inserted " << collection.termar.size() << " terms in " << doubleToString(duration) << " seconds " << std::endl;

		typedef strus::StatisticsViewerInterface::DocumentFrequencyChange DocumentFrequencyChange;
		DocumentFrequencyChange rec;

		std::set<Term> termset;
		while (msgblksize)
		{
			strus::local_ptr<strus::StatisticsViewerInterface> viewer( pmp->createViewer( msgblk, msgblksize));
			if (!viewer.get())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}
			while (viewer->nextDfChange( rec))
			{
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "result df change " << rec.type() << " " << rec.value() << " " << rec.increment() << std::endl;
#endif
				termset.insert( Term( rec.type(), rec.value(), rec.increment()));
			}
			nofDocsInserted += viewer->nofDocumentsInsertedChange();

			if (!builder->fetchMessage( msgblk, msgblksize))
			{
				if (g_errorhnd->hasError())
				{
					throw std::runtime_error( g_errorhnd->fetchError());
				}
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "fetch message " << ++blockcnt << " " << msgblksize << std::endl;
#endif
			blobsize += msgblksize;
		}
		std::vector<Term> termar;
		std::set<Term>::const_iterator si = termset.begin(), se = termset.end();
		for (; si != se; ++si)
		{
			termar.push_back( *si);
		}
		if (nofDocs != nofDocsInserted)
		{
			throw std::runtime_error( "statistics number of documents does not match");
		}
		if (collection.termar.size() != termar.size())
		{
			std::cerr << "COLLECTION SIZE " << collection.termar.size() << " MESSAGE ITEMS " << termar.size() << std::endl;
			throw std::runtime_error( "statistics message number of messages does not match");
		}
		std::vector<Term>::const_iterator oi = collection.termar.begin(), oe = collection.termar.end();
		ti = termar.begin(), te = termar.end();

		for (int tidx=0; oi != oe && ti != te; ++oi,++ti)
		{
			if (oi->type != ti->type)
			{
				std::cerr << "[" << tidx << "] TYPE '" << ti->type << "' != '" << oi->type << "'" << std::endl;
				throw std::runtime_error( "statistics message item type does not match");
			}
			if (oi->value != ti->value)
			{
				std::cerr << "[" << tidx << "] VALUE '" << ti->value << "' != '" << oi->value << "'" << std::endl;
				throw std::runtime_error( "statistics message item value does not match");
			}
			if (oi->diff != ti->diff)
			{
				std::cerr << "[" << tidx << "] DIFF '" << ti->diff << "' != '" << oi->diff << "'" << std::endl;
				throw std::runtime_error( "statistics message item diff does not match");
			}
		}
		if (g_errorhnd->hasError())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		std::cerr << "processed blob of " << blobsize << " [uncompressed " << termsByteSize << "] bytes" << std::endl;
		std::cerr << "Ok [" << collection.termar.size() << "]" << std::endl;
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
	return 4;
}


