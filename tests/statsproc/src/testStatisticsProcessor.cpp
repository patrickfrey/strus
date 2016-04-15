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

/// \brief Pseudo random generator 
enum {KnuthIntegerHashFactor=2654435761U};

uint32_t uint32_hash( uint32_t a)
{
	a += ~(a << 15);
	a ^=  (a >> 10);
	a +=  (a << 3);
	a ^=  (a >> 6);
	a += ~(a << 11);
	a ^=  (a >> 16);
	return a;
}

class Random
{
public:
	Random()
	{
		time_t nowtime;
		struct tm* now;

		::time( &nowtime);
		now = ::localtime( &nowtime);

		m_value = uint32_hash( ((now->tm_year+1)
					* (now->tm_mon+100)
					* (now->tm_mday+1)));
	}

	unsigned int get( unsigned int min_, unsigned int max_)
	{
		if (min_ >= max_)
		{
			throw std::runtime_error("illegal range passed to pseudo random number generator");
		}
		m_value = uint32_hash( m_value + 1);
		unsigned int iv = max_ - min_;
		if (iv)
		{
			return (m_value % iv) + min_;
		}
		else
		{
			return min_;
		}
	}

	unsigned int get( unsigned int min_, unsigned int max_, unsigned int psize, ...)
	{
		va_list ap;
		unsigned int pidx = get( 0, psize+1);
		if (pidx == psize)
		{
			return get( min_, max_);
		}
		else
		{
			unsigned int rt = min_;
			va_start( ap, psize);
			for (unsigned int ii = 0; ii <= pidx; ii++)
			{
				rt = va_arg( ap, unsigned int);
			}
			va_end(ap);
			return rt;
		}
	}

private:
	unsigned int m_value;
};

static Random g_random;
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
			int diff = g_random.get( 0, (int)diffrange) - (int)diffrange/2;
			termset.insert( Term( randomType(), randomTerm(), diff));
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
	unsigned int val = (unsigned int)::floor( val_ * 10000);
	unsigned int val_sec = val / 10000;
	unsigned int val_ms = val & 10000;
	std::ostringstream val_str;
	val_str << val_sec << "." << std::setfill('0') << std::setw(4) << val_ms;
	return val_str.str();
}

static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " <nofterms> <diffrange> <maxblocksize> [<lexorder>]" << std::endl;
	std::cerr << "<nofterms>      = number of distinct terms" << std::endl;
	std::cerr << "<diffrange>     = maximum diff" << std::endl;
	std::cerr << "<maxblocksize>  = maximum size of a block in bytes" << std::endl;
	std::cerr << "<lexorder>      = 1, if insert in lexical order, 0 if random" << std::endl;
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
	else if (argc < 4)
	{
		std::cerr << "ERROR too few parameters" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	else if (argc > 5)
	{
		std::cerr << "ERROR too many parameters" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	try
	{
		unsigned int nofTerms = getUintValue( argv[1]);
		unsigned int diffRange = getUintValue( argv[2]);
		unsigned int maxBlockSize = getUintValue( argv[3]);
		bool insertInOrder = (bool)g_random.get( 0, 2);
		if (argc>4) insertInOrder = getUintValue( argv[4]);
		TermCollection collection( nofTerms, diffRange);

		std::auto_ptr<strus::StatisticsProcessorInterface> pmp( strus::createStatisticsProcessor( g_errorhnd));
		strus::StatisticsProcessorInterface::BuilderOptions options( insertInOrder?strus::StatisticsProcessorInterface::BuilderOptions::InsertInLexicalOrder:strus::StatisticsProcessorInterface::BuilderOptions::None, maxBlockSize);
		std::auto_ptr<strus::StatisticsBuilderInterface> builder( pmp->createBuilder( options));
		if (!builder.get())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		builder->setNofDocumentsInsertedChange( (int)g_random.get( 0, 1000000) - 500000);

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
		const char* msgblk;
		std::size_t msgblksize;
		std::size_t blobsize = 0;
		if (!builder->fetchMessage( msgblk, msgblksize))
		{
			if (g_errorhnd->hasError())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}
		}
		blobsize += msgblksize;
		double duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		const char* insertInOrder_str = (insertInOrder?"in lexical order":"in random order");
		std::cerr << "inserted " << collection.termar.size() << " terms in " << doubleToString(duration) << " seconds " << insertInOrder_str << std::endl;

		typedef strus::StatisticsViewerInterface::DocumentFrequencyChange DocumentFrequencyChange;
		DocumentFrequencyChange rec;

		std::set<Term> termset;
		while (msgblksize)
		{
			std::auto_ptr<strus::StatisticsViewerInterface> viewer( pmp->createViewer( msgblk, msgblksize));
			if (!viewer.get())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}
			while (viewer->nextDfChange( rec))
			{
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "result df change " << rec.type << " " << rec.value << " " << rec.increment << std::endl;
#endif
				termset.insert( Term( rec.type, rec.value, rec.increment));
			}
			if (!builder->fetchMessage( msgblk, msgblksize))
			{
				if (g_errorhnd->hasError())
				{
					throw std::runtime_error( g_errorhnd->fetchError());
				}
			}
			blobsize += msgblksize;
		}
		std::vector<Term> termar;
		std::set<Term>::const_iterator si = termset.begin(), se = termset.end();
		for (; si != se; ++si)
		{
			termar.push_back( *si);
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


