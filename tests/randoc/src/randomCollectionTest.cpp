/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#include "strus/storageLib.hpp"
#include "strus/queryProcessorLib.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/queryEvalLib.hpp"
#include "strus/iteratorInterface.hpp"
#include "strus/accumulatorInterface.hpp"
#include "strus/storageInterface.hpp"
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <ctime>
#include <algorithm>
#include <limits>
#include <boost/scoped_ptr.hpp>

/// \brief Pseudo random generator 
enum {KnuthIntegerHashFactor=2654435761U};
#define STRUS_LOWLEVEL_DEBUG

class Random
{
public:
	Random()
	{
		time_t nowtime;
		struct tm* now;

		::time( &nowtime);
		now = ::localtime( &nowtime);

		m_value = ((now->tm_year+1) * (now->tm_mon+100) * (now->tm_mday+1)) * KnuthIntegerHashFactor;
	}

	unsigned int get( unsigned int min_, unsigned int max_)
	{
		m_value = (m_value+123) * KnuthIntegerHashFactor;
		unsigned int iv = max_ - min_;
		if (iv)
		{
			return ((m_value ^ (m_value >> 8)) % iv) + min_;
		}
		else
		{
			return min_;
		}
	}

private:
	unsigned int m_value;
};

static Random g_random;

static const char* randomType()
{
	enum {NofTypes=5};
	static const char* ar[ NofTypes] = {"WORD","STEM","NUM","LOC","ORIG"};
	return ar[ g_random.get( 0, (unsigned int)NofTypes-1)];
}

static std::string randomTerm( unsigned int nofFeatures)
{
	std::string rt;
	static const char* alphabet
		= {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};
	unsigned int val = g_random.get( 0, nofFeatures)
			 * (unsigned int)KnuthIntegerHashFactor;
	unsigned int le = g_random.get( 1, 20);
	unsigned int li = 0;
	while (g_random.get( 1, 10) < 6)
	{
		// ... distribution that favors shorter terms
		le = (le / 2) + 1;
	}
	for (; li < le; ++li)
	{
		unsigned int pf = (li * val) >> 8;
		unsigned int chidx = ((val^pf) % 52);
		rt.push_back( alphabet[chidx]);
	}
	return rt;
}

struct TermCollection
{
	TermCollection( unsigned int nofTerms)
	{
		unsigned int ii=0;
		for (; ii<nofTerms; ++ii)
		{
			termar.push_back( Term( randomType(), randomTerm( nofTerms)));
		}
	}

	struct Term
	{
		std::string type;
		std::string value;
		float weight;

		Term( const Term& o)
			:type(o.type),value(o.value)
			,weight(o.weight){}
		Term( const std::string& t, const std::string& v)
			:type(t),value(v)
			,weight(0){}
	};

	std::vector<Term> termar;
};

struct RandomDoc
{
	RandomDoc( const RandomDoc& o)
		:docid(o.docid)
		,occurrencear(o.occurrencear)
		,weightmap(o.weightmap){}

	explicit RandomDoc( unsigned int docid_, const TermCollection& collection, unsigned int size)
	{
		std::ostringstream docid_cnv;
		docid_cnv << "doc" << docid_;
		docid = docid_cnv.str();

		unsigned int posidx = 1;
		while (posidx <= size)
		{
			unsigned int termidx = 1+g_random.get( 0, collection.termar.size());
			occurrencear.push_back( Occurrence( termidx, posidx));
			if (g_random.get( 0, 40) > 20) posidx++;
		}
	}

	struct Occurrence
	{
		unsigned int term;
		unsigned int pos;

		Occurrence( const Occurrence& o)
			:term(o.term),pos(o.pos){}
		Occurrence( unsigned int t, unsigned int p)
			:term(t),pos(p){}
	};

	std::string docid;
	std::vector<Occurrence> occurrencear;
	std::map<unsigned int, float> weightmap;
};

static float tfIdf( unsigned int collSize, unsigned int nofMatchingDocs, unsigned int nofMatchesInDoc, unsigned int docLen, double avgDocLen)
{
	// Use tf from Okapi but IDF not, because we do not want to have negative weights
	const double k1 = 1.5; //.... [1.2,2.0]
	const double b = 0.75; // fix

	double IDF = ::log( collSize / (nofMatchingDocs + 1.0));
	double tf = ((double)nofMatchesInDoc * (k1 + 1.0))
		/ ((double)nofMatchesInDoc 
			+ (k1 * (1.0 - b + ((b * (double)docLen) / avgDocLen)))
		);
	return (float)(tf * IDF);
}

struct RandomCollection
{
	RandomCollection( unsigned int nofTerms, unsigned int nofDocuments, unsigned int maxDocumentSize)
		:termCollection(nofTerms)
	{
		unsigned int di = 0;
		for (; di < nofDocuments; ++di)
		{
			unsigned int docsize = g_random.get( 2, maxDocumentSize);
			while (g_random.get( 1, 10) < 4)
			{
				// ... distribution that favors shorter documents
				docsize = (docsize / 2) + 1;
			}
			docar.push_back( RandomDoc( di+1, termCollection, docsize));
		}
		std::vector<unsigned int> termDocumentFrequencyMap( termCollection.termar.size(), 0);
		std::vector<unsigned int> termCollectionFrequencyMap( termCollection.termar.size(), 0);
		double avgDocLen = 0.0;
		
		for (unsigned int di=0; di < docar.size(); ++di)
		{
			RandomDoc& doc = docar[di];
			std::map<unsigned int, unsigned int> matchcount;

			std::vector<RandomDoc::Occurrence>::const_iterator oi = doc.occurrencear.begin(), oe = doc.occurrencear.end();
			for (; oi != oe; ++oi)
			{
				if (++matchcount[ oi->term] == 1)
				{
					++termDocumentFrequencyMap[ oi->term-1];
				}
				++termCollectionFrequencyMap[ oi->term-1];
			}
			avgDocLen += (double)doc.occurrencear.size() / nofDocuments;
		}
		for (unsigned int di=0; di < docar.size(); ++di)
		{
			RandomDoc& doc = docar[di];
			std::map<unsigned int, unsigned int> matchcount;

			std::vector<RandomDoc::Occurrence>::iterator oi = doc.occurrencear.begin(), oe = doc.occurrencear.end();
			for (; oi != oe; ++oi)
			{
				++matchcount[ oi->term];
			}
			oi = doc.occurrencear.begin();
			for (; oi != oe; ++oi)
			{
				unsigned int collSize = docar.size();
				unsigned int nofMatchingDocs = termDocumentFrequencyMap[ oi->term-1];
				unsigned int nofMatchesInDoc = matchcount[ oi->term];
				unsigned int docLen = doc.occurrencear.size();

				doc.weightmap[ oi->term] = tfIdf( collSize, nofMatchingDocs, nofMatchesInDoc, docLen, avgDocLen);
			}
		}
	}

	TermCollection termCollection;
	std::vector<RandomDoc> docar;
};


class RandomGen
{
public:
	RandomGen(){}

	std::size_t operator()( std::size_t i)
	{
		return (std::size_t)g_random.get( 0, i);
	}
};
	
struct RandomQuery
{
	enum {MaxNofArgs=8};

	RandomQuery( const RandomCollection& collection)
		:range(0)
	{
	AGAIN:
		operation = (Operation)g_random.get( 0, NofOperations);
		std::size_t pickDocIdx = g_random.get( 0, collection.docar.size());
		const RandomDoc& pickDoc = collection.docar[ pickDocIdx];
		operation = Union;

		switch (operation)
		{
			case Intersect:
			{
				unsigned int pi = g_random.get( 0, pickDoc.occurrencear.size());
				arg.push_back( pickDoc.occurrencear[pi].term);

				for (; pi+1 < pickDoc.occurrencear.size() && arg.size() < (unsigned int)MaxNofArgs; ++pi)
				{
					if (pickDoc.occurrencear[pi].pos == pickDoc.occurrencear[pi+1].pos)
					{
						arg.push_back( pickDoc.occurrencear[pi+1].term);
					}
					else
					{
						break;
					}
				}
				if (arg.size() == 1)
				{
					goto AGAIN;
				}
				break;
			}
			case Union:
			{
				unsigned int nn = g_random.get( 1, MaxNofArgs);
				unsigned int ii = 0;

				for (; ii<nn; ++ii)
				{
					unsigned int pickOccIdx = g_random.get( 0, pickDoc.occurrencear.size());
					const RandomDoc::Occurrence& pickOcc = pickDoc.occurrencear[ pickOccIdx];
					arg.push_back( pickOcc.term);
				}
				break;
			}
			case Difference:
			{
				unsigned int pi = g_random.get( 0, pickDoc.occurrencear.size());
				arg.push_back( pickDoc.occurrencear[pi].term);

				for (; pi+1 < pickDoc.occurrencear.size(); ++pi)
				{
					if (pickDoc.occurrencear[pi].pos == pickDoc.occurrencear[pi+1].pos)
					{
						arg.push_back( pickDoc.occurrencear[pi+1].term);
						break;
					}
					else
					{
						break;
					}
				}
				if (arg.size() == 1)
				{
					goto AGAIN;
				}
				break;
			}
			case StructWithin:
			case Within:
			case StructSequence:
			case Sequence:
			{
				unsigned int pickOccIdx = g_random.get( 0, pickDoc.occurrencear.size());
				const RandomDoc::Occurrence& pickOcc = pickDoc.occurrencear[ pickOccIdx];
				arg.push_back( pickOcc.term);

				unsigned int rangeOccIdx = g_random.get( pickOccIdx, pickDoc.occurrencear.size());
				const RandomDoc::Occurrence& rangeOcc = pickDoc.occurrencear[ rangeOccIdx];

				unsigned int prevpos = pickOcc.pos;
				if (operation == StructSequence || operation == Sequence)
				{
					if (rangeOcc.pos == prevpos) goto AGAIN;
				}
				for (unsigned int ti = pickOccIdx; ti < rangeOccIdx; ++ti)
				{
					const RandomDoc::Occurrence& midOcc = pickDoc.occurrencear[ ti];
					if (midOcc.term == rangeOcc.term)
					{
						goto AGAIN;
					}
					if (midOcc.term != pickOcc.term)
					{
						if (operation == StructSequence || operation == Sequence)
						{
							if (midOcc.pos == prevpos) continue;
						}
						arg.push_back( midOcc.term);
					}
					prevpos = midOcc.pos;
				}
				arg.push_back( rangeOcc.term);
				if (operation == StructWithin || operation == Within)
				{
					//... in case of within range condition without order, shuffle the elements
					shuffleArg();
				}
				range = (int)rangeOcc.pos - (int)pickOcc.pos;

				if (operation == StructWithin || operation == StructSequence)
				{
					//... in case of structure insert structure delimiter as first argument
					unsigned int cutOccIdx = pickOccIdx + g_random.get( 0, rangeOccIdx - pickOccIdx);
					const RandomDoc::Occurrence& cutOcc = pickDoc.occurrencear[ cutOccIdx];
					arg.insert( arg.begin(), cutOcc.term);
				}
				break;
			}
		}
	}
	RandomQuery( const RandomQuery& o)
		:operation(o.operation),arg(o.arg),range(o.range){}

	void shuffleArg()
	{
		RandomGen rnd;
		std::random_shuffle( arg.begin(), arg.end(), rnd);
	}

	struct Match
	{
		unsigned int docno;
		unsigned int pos;

		Match( const Match& o)
			:docno(o.docno),pos(o.pos){}
		Match( unsigned int docno_, unsigned int pos_)
			:docno(docno_),pos(pos_){}
	};

	std::vector<RandomDoc::Occurrence>::const_iterator
		findTerm( std::vector<RandomDoc::Occurrence>::const_iterator oi, std::vector<RandomDoc::Occurrence>::const_iterator oe, unsigned int term, unsigned int lastpos) const
	{
		if (oi != oe)
		{
			for (++oi; oi != oe && oi->pos <= lastpos; ++oi)
			{
				if (oi->term == term) return oi;
			}
		}
		return oe;
	}

	bool matchTerm( std::vector<RandomDoc::Occurrence>::const_iterator oi, std::vector<RandomDoc::Occurrence>::const_iterator oe, unsigned int term, unsigned int lastpos) const
	{
		return oe != findTerm( oi, oe, term, lastpos);
	}

	bool skipNextPosition( std::vector<RandomDoc::Occurrence>::const_iterator& oi, const std::vector<RandomDoc::Occurrence>::const_iterator& oe) const
	{
		if (oi == oe) return false;
		unsigned int pos = oi->pos;
		for (++oi; oi != oe && oi->pos == pos; ++oi){}
		return oi != oe;
	}

	std::vector<Match> expectedMatches( const RandomCollection& collection) const
	{
		std::vector<Match> rt;
		std::vector<RandomDoc>::const_iterator di = collection.docar.begin(), de = collection.docar.end();
		for (unsigned int docno=1; di != de; ++di,++docno)
		{
			std::vector<RandomDoc::Occurrence>::const_iterator oi = di->occurrencear.begin(), oe = di->occurrencear.end();
			for (; oi != oe; (void)skipNextPosition(oi,oe))
			{
				switch (operation)
				{
					case Intersect:
					{
						if (arg.empty()) continue;
						std::vector<unsigned int>::const_iterator ai = arg.begin(), ae = arg.end();
						for (; ai!=ae; ++ai)
						{
							if (!matchTerm( oi, oe, *ai, oi->pos))
							{
								break;
							}
						}
						if (ai == ae)
						{
							rt.push_back( Match( docno, oi->pos));
						}
						break;
					}
					case Difference:
						if (arg.size() != 2) continue;
						if (matchTerm( oi, oe, arg[0], oi->pos))
						{
							if (matchTerm( oi, oe, arg[1], oi->pos)) continue;
							rt.push_back( Match( docno, oi->pos));
						}
					case Union:
					{
						if (arg.empty()) continue;
						std::vector<unsigned int>::const_iterator ai = arg.begin(), ae = arg.end();
						for (; ai!=ae; ++ai)
						{
							if (matchTerm( oi, oe, *ai, oi->pos))
							{
								break;
							}
						}
						if (ai != ae)
						{
							rt.push_back( Match( docno, oi->pos));
						}
						break;
					}
					case Sequence:
					case StructSequence:
					{
						if (arg.empty()) continue;
						unsigned int delimiter_term = 0;
						std::size_t argidx = 0;
						unsigned int lastpos = range<0?(oi->pos-range):(oi->pos+range);

						if (operation == StructSequence)
						{
							delimiter_term = arg[argidx++];
							if (arg.size() == 1) continue;
						}
						// Try to match sequence:
						if (!matchTerm( oi, oe, arg[argidx], oi->pos)) continue;

						std::vector<RandomDoc::Occurrence>::const_iterator fi = oi;
						(void)skipNextPosition(fi,oe);

						for (++argidx; argidx < arg.size() && fi != oe && fi->pos <= lastpos; (void)skipNextPosition(fi,oe))
						{
							if (matchTerm( fi, oe, arg[argidx], fi->pos))
							{
								argidx++;
								if (argidx == arg.size()) break;
							}
						}
						// Check if matched and check the structure delimiter term
						if (argidx == arg.size()
						&& (!delimiter_term || matchTerm( oi, oe, delimiter_term, fi->pos)))
						{
							if (range >= 0)
							{
								rt.push_back( Match( docno, oi->pos));
							}
							else
							{
								rt.push_back( Match( docno, fi->pos));
							}
						}
						break;
					}
					case Within:
					case StructWithin:
					{
						if (arg.empty()) continue;
						unsigned int delimiter_term = 0;
						std::size_t argidx = 0;
						unsigned int lastpos = range<0?(oi->pos-range):(oi->pos+range);

						if (operation == StructWithin)
						{
							delimiter_term = arg[argidx++];
							if (arg.size() == 1) continue;
						}
						std::size_t ai = argidx;
						for (; ai<arg.size(); ++ai)
						{
							if (matchTerm( oi, oe, arg[ai], oi->pos)) break;
						}
						if (ai<arg.size())
						{
							unsigned int maxpos = oi->pos;
							for (ai=argidx; ai<arg.size(); ++ai)
							{
								std::vector<RandomDoc::Occurrence>::const_iterator
									fi = findTerm( oi, oe, arg[ai], lastpos);
								if (fi->pos > maxpos)
								{
									maxpos = fi->pos;
								}
							}
							if (argidx == arg.size()
							&& (!delimiter_term || matchTerm( oi, oe, delimiter_term, maxpos)))
							{
								if (range >= 0)
								{
									rt.push_back( Match( docno, oi->pos));
								}
								else
								{
									rt.push_back( Match( docno, maxpos));
								}
							}
						}
						break;
					}
				}
			}
		}
		return rt;
	}

	static std::vector<Match> resultMatches( strus::IteratorInterface* itr)
	{
		std::vector<Match> rt;
		unsigned int docno = (unsigned int)itr->skipDoc( 0);
		unsigned int pos = 0;
		while (docno)
		{
			pos = (unsigned int)itr->skipPos( pos);
			if (pos)
			{
				rt.push_back( Match( docno, pos));
				++pos;
			}
			else
			{
				docno = (unsigned int)itr->skipDoc( docno+1);
				pos = 0;
			}
		}
		return rt;
	}

	bool compareMatches( const std::vector<Match>& matchar, strus::IteratorInterface* itr) const
	{
		std::vector<Match> res = resultMatches( itr);

		std::vector<Match>::const_iterator mi = matchar.begin(), me = matchar.end();
		std::vector<Match>::const_iterator ri = res.begin(), re = res.end();
		for (; mi != me && ri != re; ++mi,++ri)
		{
			if (mi->docno < ri->docno)
			{
				std::cerr << "match missed in doc " << mi->docno << " at " << mi->pos << std::endl;
				return false;
			}
			if (mi->docno > ri->docno)
			{
				std::cerr << "unexpected match in doc " << ri->docno << " at " << ri->pos << std::endl;
				return false;
			}
			if (mi->pos < ri->pos)
			{
				std::cerr << "match missed in doc " << mi->docno << " at " << mi->pos << std::endl;
				return false;
			}
			if (mi->pos > ri->pos)
			{
				std::cerr << "unexpected match in doc " << ri->docno << " at " << ri->pos << std::endl;
				return false;
			}
		}
		if (mi != me)
		{
			std::cerr << "match missed in doc " << mi->docno << " at " << mi->pos << std::endl;
			return false;
		}
		if (ri != re)
		{
			std::cerr << "unexpected match in doc " << ri->docno << " at " << ri->pos << std::endl;
			return false;
		}
		return true;
	}

	std::string tostring( const RandomCollection& collection) const
	{
		std::ostringstream rt;
		rt << operationName();
		if (range)
		{
			rt << " " << range;
		}
		for (unsigned int ai=0; ai<arg.size(); ++ai)
		{
			const TermCollection::Term& term = collection.termCollection.termar[ arg[ai]-1];
			rt << " " << term.type << " '" << term.value << "'";
		}
		return rt.str();
	}

	bool execute( strus::QueryProcessorInterface* queryproc, const RandomCollection& collection) const
	{
		unsigned int nofitr = arg.size();
		const strus::IteratorInterface* itr[ MaxNofArgs];
		for (unsigned int ai=0; ai<nofitr; ++ai)
		{
			const TermCollection::Term& term = collection.termCollection.termar[ arg[ai]-1];
			itr[ ai] = queryproc->createIterator( term.type, term.value);
			if (!itr[ ai])
			{
				std::cerr << "term not found [" << arg[ai] << "]: " << term.type << " '" << term.value << "'" << std::endl;
				std::cerr << "random query failed: " << tostring( collection) << std::endl;
				return false;
			}
		}
		std::string opname( operationName());
		strus::IteratorInterface* res = 
			queryproc->createIterator(
					opname, range, std::size_t(nofitr), &itr[0]);

		if (!compareMatches( expectedMatches( collection), res))
		{
			std::cerr << "random query failed: " << tostring( collection) << std::endl;
			return false;
		}
		for (unsigned int ai=0; ai<nofitr; ++ai)
		{
			delete itr[ai];
		}
		delete res;
		return true;
	}

	enum Operation
	{
		Intersect,
		Union,
		Difference,
		Within,
		StructWithin,
		Sequence,
		StructSequence
	};
	enum {NofOperations=7};
	static const char* operationName( Operation op)
	{
		static const char* ar[] = {"intersect","union","diff","within","within_struct","sequence","sequence_struct"};
		return ar[op];
	}
	const char* operationName() const
	{
		return operationName( operation);
	}

	Operation operation;
	std::vector<unsigned int> arg;
	int range;
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

static std::string timeToString( double val_)
{
	unsigned int val = ::floor( val_ * 1000);
	unsigned int val_sec = val / 1000;
	unsigned int val_ms = val & 1000;
	std::ostringstream val_str;
	val_str << val_sec << "." << std::setfill('0') << std::setw(3) << val_ms;
	return val_str.str();
}

static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " <config> <nofdocs> <maxsize> <features> <ops>" << std::endl;
	std::cerr << "<config>  = storage description" << std::endl;
	std::cerr << "<nofdocs> = number of documents to insert" << std::endl;
	std::cerr << "<maxsize> = maximum size of a document" << std::endl;
	std::cerr << "<features>= number of distinct features" << std::endl;
	std::cerr << "<ops>     = number of random test query operations" << std::endl;
}

int main( int argc, const char* argv[])
{
	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		printUsage( argc, argv);
		return 0;
	}
	else if (argc < 6)
	{
		std::cerr << "too few parameters" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	try
	{
		const char* config = argv[1];
		unsigned int nofDocuments = getUintValue( argv[2]);
		unsigned int maxDocumentSize = getUintValue( argv[3]);
		unsigned int nofFeatures = getUintValue( argv[4]);
		unsigned int nofQueries = getUintValue( argv[5]);

		strus::createStorageDatabase( config);

		RandomCollection collection( nofFeatures, nofDocuments, maxDocumentSize);
		boost::scoped_ptr<strus::StorageInterface> storage( strus::createStorageClient( config));

		strus::Index totNofOccurrencies = 0;
		strus::Index totNofFeatures = 0;
		strus::Index totNofDocuments = 0;
		strus::Index totTermStringSize = 0;
		unsigned int insertIntervallSize = 1000;
		unsigned int insertIntervallCnt = 0;

		std::vector<RandomDoc>::const_iterator di = collection.docar.begin(), de = collection.docar.end();
		for (; di != de; ++di,++totNofDocuments)
		{
			typedef boost::scoped_ptr<strus::StorageInterface::TransactionInterface> Transaction;

			Transaction transaction( storage->createTransaction( di->docid));
			std::vector<RandomDoc::Occurrence>::const_iterator oi = di->occurrencear.begin(), oe = di->occurrencear.end();

			for (; oi != oe; ++oi,++totNofOccurrencies)
			{
				const TermCollection::Term& term = collection.termCollection.termar[ oi->term-1];
				transaction->addTermOccurrence( term.type, term.value, oi->pos);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "term [" << oi->term << "] type '" << term.type << "' value '" << term.value << "' pos " << oi->pos << std::endl;
#endif
				totTermStringSize += term.value.size();
			}
			std::map<unsigned int, float>::const_iterator wi = di->weightmap.begin(), we = di->weightmap.end();
			for (; wi != we; ++wi,++totNofFeatures)
			{
				const TermCollection::Term& term = collection.termCollection.termar[ wi->first-1];
				transaction->setTermWeight( term.type, term.value, wi->second);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "weigth type '" << term.type << "' value '" << term.value << "' weight " << wi->second << std::endl;
#endif
			}
			transaction->commit();

#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "inserted document '" << di->docid << "' size " << di->occurrencear.size() << std::endl;
#endif
			if (++insertIntervallCnt == insertIntervallSize)
			{
				insertIntervallCnt = 0;
				std::cerr << "inserted " << (totNofDocuments+1) << " documents, " << totTermStringSize <<" bytes " << std::endl;
			}
		}
		std::cerr << "inserted collection with " << totNofDocuments << " documents, " << totNofFeatures << " terms, " << totNofOccurrencies << " occurrencies, " << totTermStringSize << " bytes" << std::endl;
		boost::scoped_ptr<strus::QueryProcessorInterface> queryproc(
			strus::createQueryProcessorInterface( storage.get()));

		std::vector<RandomQuery> randomQueryAr;
		{
			for (std::size_t qi=0; qi < nofQueries; ++qi)
			{
				randomQueryAr.push_back( RandomQuery( collection));
			}
		}
		if (nofQueries)
		{
			std::clock_t start;
			double duration;
			unsigned int nofQueriesFailed = 0;
			start = std::clock();

			std::vector<RandomQuery>::const_iterator qi = randomQueryAr.begin(), qe = randomQueryAr.end();
			for (; qi != qe; ++qi)
			{
				if (!qi->execute( queryproc.get(), collection))
				{
					++nofQueriesFailed;
				}
			}
			duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
			if (nofQueriesFailed)
			{
				std::cerr << "evaluated " << nofQueries << " random queries with " << nofQueriesFailed << " queries failed" << std::endl;
			}
			else
			{
				std::cerr << "evaluated " << nofQueries << " random queries correctly in " << timeToString(duration) << " milliseconds" << std::endl;
			}
		}
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
	return -1;
}


