/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "strus/lib/peermsgproc.hpp"
#include "strus/peerMessageProcessorInterface.hpp"
#include "strus/peerMessageViewerInterface.hpp"
#include "strus/peerMessageBuilderInterface.hpp"
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
#include <stdint.h>
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
		std::set<Term> termSet;
		while (termSet.size() < nofTerms)
		{
			int diff = g_random.get( 0, (int)diffrange) - (int)diffrange/2;
			bool isnew = 0!=g_random.get( 0, 1);
			termSet.insert( Term( randomType(), randomTerm(), diff, isnew));
		}
		std::set<Term>::const_iterator ti = termSet.begin(), te = termSet.end();
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
		bool isnew;

		Term( const Term& o)
			:type(o.type),value(o.value),diff(o.diff),isnew(o.isnew){}
		Term( const std::string& type_, const std::string& value_, int diff_, bool isnew_)
			:type(type_),value(value_),diff(diff_),isnew(isnew_){}

		std::string tostring() const
		{
			std::ostringstream rt;
			rt << " " << type << " '" << value << "' " << diff << " " << (isnew?"new":"old");
			return rt.str();
		}

		bool operator <( const Term& o) const
		{
			if (type > o.type) return false;
			if (type < o.type) return true;
			if (value > o.value) return false;
			if (value < o.value) return true;
			if (diff > o.diff) return false;
			if (diff < o.diff) return true;
			if (isnew > o.isnew) return false;
			if (isnew < o.isnew) return true;
			return false;
		}
	};

	std::vector<Term> termar;
};


static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " <config> <nofdocs> <maxsize> <features> <ops>" << std::endl;
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
		unsigned int nofTerms = getUintValue( argv[1]);
		unsigned int diffrange = getUintValue( argv[2]);
		TermCollection collection( nofTerms, diffrange);

		const strus::PeerMessageProcessorInterface* pmp = strus::getPeerMessageProcessor();
		std::auto_ptr<strus::PeerMessageBuilderInterface> builder( pmp->createBuilder());
		builder->setNofDocumentsInsertedChange( (int)g_random.get( 0, 1000000) - 500000);

		typedef TermCollection::Term Term;
		unsigned int termsByteSize = 0;
		std::vector<Term>::const_iterator ti = collection.termar.begin(), te = collection.termar.end();
		for (; ti != te; ++ti)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			const char* isnewstr = ti->isnew?"new":"old";
			std::cerr << "add df change '" << ti->type << "' '" << ti->value << "' " << ti->diff << " " << isnewstr << std::endl;
#endif
			termsByteSize += ti->type.size() + ti->value.size() + 5;
			builder->addDfChange( ti->type.c_str(), ti->value.c_str(), ti->diff, ti->isnew);
		}
		const char* msgptr = 0;
		std::size_t msgsize = 0;

		builder->fetchMessage( msgptr, msgsize);
		typedef strus::PeerMessageViewerInterface::DocumentFrequencyChange DocumentFrequencyChange;
		DocumentFrequencyChange rec;

		std::vector<Term> termar;
		std::auto_ptr<strus::PeerMessageViewerInterface> viewer( pmp->createViewer( msgptr, msgsize));
		while (viewer->fetchDfChange( rec))
		{
			std::string term( rec.type, rec.typesize);
			std::string value( rec.value, rec.valuesize);
			termar.push_back( Term( term, value, rec.increment, rec.isnew));
		}

		if (collection.termar.size() != termar.size())
		{
			std::cerr << "COLLECTION SIZE " << collection.termar.size() << " MESSAGE ITEM NUMBER " << termar.size() << std::endl;
			throw std::runtime_error( "peer message number of messages does not match");
		}
		std::vector<Term>::const_iterator oi = collection.termar.begin(), oe = collection.termar.end();
		ti = termar.begin(), te = termar.end();

		for (int tidx=0; oi != oe && ti != te; ++oi,++ti)
		{
			if (oi->type != ti->type)
			{
				std::cerr << "[" << tidx << "] TYPE '" << ti->type << "' != '" << oi->type << "'" << std::endl;
				throw std::runtime_error( "peer message item type does not match");
			}
			if (oi->value != ti->value)
			{
				std::cerr << "[" << tidx << "] VALUE '" << ti->value << "' != '" << oi->value << "'" << std::endl;
				throw std::runtime_error( "peer message item value does not match");
			}
			if (oi->diff != ti->diff)
			{
				std::cerr << "[" << tidx << "] DIFF '" << ti->diff << "' != '" << oi->diff << "'" << std::endl;
				throw std::runtime_error( "peer message item diff does not match");
			}
			if (oi->isnew != ti->isnew)
			{
				const char* oi_isnew = oi->isnew?"true":"false";
				const char* ti_isnew = ti->isnew?"true":"false";
				std::cerr << "[" << tidx << "] ISNEW '" << ti_isnew << "' != '" << oi_isnew << "'" << std::endl;
				throw std::runtime_error( "peer message item isnew does not match");
			}
		}
		std::cerr << "processed blob of " << msgsize << " [uncompressed " << termsByteSize << "] bytes" << std::endl;
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


