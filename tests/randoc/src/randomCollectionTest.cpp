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
#include "strus/storageLib.hpp"
#include "strus/queryProcessorLib.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/queryEvalLib.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageInserterInterface.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <ctime>
#include <algorithm>
#include <limits>
#include <cstdarg>
#include <stdint.h>
#include <boost/scoped_ptr.hpp>

/// \brief Pseudo random generator 
enum {KnuthIntegerHashFactor=2654435761U};
#undef STRUS_LOWLEVEL_DEBUG

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
					* (now->tm_mday+1))
			);
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
	TermCollection( unsigned int nofTerms)
	{
		if (nofTerms < 1)
		{
			std::cerr << "ERROR number of distinct terms in the collection has to be at least 1" << std::endl;
			nofTerms = 1;
		}
		std::set<std::string> termSet;
		while (termSet.size() < nofTerms)
		{
			termSet.insert( randomTerm());
		}
		std::set<std::string>::const_iterator ti = termSet.begin(), te = termSet.end();
		for (; ti != te; ++ti)
		{
			termar.push_back( Term( randomType(), *ti));
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

		std::string tostring() const
		{
			std::ostringstream rt;
			rt << " " << type << " '" << value << "'";
			return rt.str();
		}
		
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

static float tfIdf( unsigned int collSize, unsigned int nofMatchingDocs, unsigned int nofMatchesInDoc, unsigned int docLen, float avgDocLen)
{
	// Use tf from Okapi but IDF not, because we do not want to have negative weights
	const float k1 = 1.5; //.... [1.2,2.0]
	const float b = 0.75; // fix

	float IDF = ::log( collSize / (nofMatchingDocs + 1.0));
	float tf = ((float)nofMatchesInDoc * (k1 + 1.0))
		/ ((float)nofMatchesInDoc 
			+ (k1 * (1.0 - b + ((b * (float)docLen) / avgDocLen)))
		);
	return (float)(tf * IDF);
}

struct RandomCollection
{
	RandomCollection( unsigned int nofTerms, unsigned int nofDocuments, unsigned int maxDocumentSize)
		:termCollection(nofTerms)
	{
		if (maxDocumentSize < 3)
		{
			std::cerr << "ERROR max document size has to be at least 3" << std::endl;
			maxDocumentSize = 3;
		}
		unsigned int di = 0;
		for (; di < nofDocuments; ++di)
		{
			unsigned int tiny_docsize  = g_random.get( 2, 3 + (maxDocumentSize/16)) + (maxDocumentSize/16);
			unsigned int small_docsize = g_random.get( 2, 3 + (maxDocumentSize/8)) + (maxDocumentSize/8);
			unsigned int med_docsize   = g_random.get( 2, 3 + (maxDocumentSize/4)) + (maxDocumentSize/4);
			unsigned int big_docsize   = g_random.get( 2, 3 + (maxDocumentSize/2)) + (maxDocumentSize/2);

			unsigned int docsize = g_random.get( 2, maxDocumentSize, 4, tiny_docsize, small_docsize, med_docsize, big_docsize);
			docar.push_back( RandomDoc( di+1, termCollection, docsize));
		}
		std::vector<unsigned int> termDocumentFrequencyMap( termCollection.termar.size(), 0);
		std::vector<unsigned int> termCollectionFrequencyMap( termCollection.termar.size(), 0);
		float avgDocLen = 0.0;
		
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
			avgDocLen += (float)doc.occurrencear.size() / nofDocuments;
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

	std::string docSummary( unsigned int docno_, unsigned int pos_, unsigned int range) const
	{
		std::ostringstream rt;
		const RandomDoc& doc = docar[docno_-1];
		std::vector<RandomDoc::Occurrence>::const_iterator oi = doc.occurrencear.begin(), oe = doc.occurrencear.end();
		for (; oi != oe; ++oi)
		{
			if (oi->pos+1 >= pos_ && oi->pos <= pos_ + range)
			{
				rt << " " << oi->pos << ": " << termCollection.termar[ oi->term-1].tostring() << " (" << oi->term << ")";
				rt << std::endl;
			}
		}
		return rt.str();
	}

	TermCollection termCollection;
	std::vector<RandomDoc> docar;
};


struct RandomQuery
{
	enum {MaxNofArgs=8};

	RandomQuery( const RandomCollection& collection)
	{
	AGAIN:
		arg.clear();
		range = 0;
		operation = (Operation)g_random.get( 0, NofOperations);
		std::size_t pickDocIdx = g_random.get( 0, collection.docar.size());
		const RandomDoc& pickDoc = collection.docar[ pickDocIdx];

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

				// insert first element selected
				arg.push_back( pickOcc.term);

				unsigned int maxRange = pickDoc.occurrencear.back().pos - pickOcc.pos;
				range = g_random.get( 0, maxRange+1, 8, 1, 2, 3, 5, 7, 9, 11, 13);

				unsigned int maxNofPicks = MaxNofArgs-2;
				unsigned int nofPicks = g_random.get( 1, maxNofPicks+1);

				// select ordered position occurrencies in a range:
				std::multiset<unsigned int> picks;
				for (unsigned int ii=0; ii<nofPicks; ++ii)
				{
					picks.insert( g_random.get( 0, range+1));
				}
				unsigned int lastOccIdx = pickOccIdx;

				// insert the elements maching to the selected position occurrencies into arg
				std::multiset<unsigned int>::const_iterator pi = picks.begin(), pe = picks.end();
				unsigned int prevpick = 0;
				unsigned int nextOccIdx = pickOccIdx+1;

				for (; pi != pe; ++pi)
				{
					if (operation == StructSequence || operation == Sequence)
					{
						// ... no elements with same position in case of a strictly ordered sequence
						if (*pi == prevpick) continue;
						prevpick = *pi;
					}
					unsigned int nextPos = pickOcc.pos + *pi;
					for (;nextOccIdx < pickDoc.occurrencear.size(); ++nextOccIdx)
					{
						if (pickDoc.occurrencear[ nextOccIdx].pos == nextPos)
						{
							break;
						}
					}
					if (nextOccIdx == pickDoc.occurrencear.size())
					{
						break;
					}
					const RandomDoc::Occurrence& nextOcc = pickDoc.occurrencear[ nextOccIdx];
					lastOccIdx = nextOccIdx;
					arg.push_back( nextOcc.term);
				}
				if (operation == StructWithin || operation == Within)
				{
					//... in case of within range condition without order, shuffle the elements
					shuffleArg();
				}
				if (operation == StructWithin || operation == StructSequence)
				{
					//... in case of structure insert structure delimiter as first argument
					unsigned int cutOccIdx = g_random.get( 0, lastOccIdx - pickOccIdx + 1) + pickOccIdx;
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
		StlRandomGen rnd;
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
			for (; oi != oe && oi->pos <= lastpos; ++oi)
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
						break;
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

						unsigned int lastmatchpos = oi->pos;
						for (++argidx; argidx < arg.size() && fi != oe && fi->pos <= lastpos; (void)skipNextPosition(fi,oe))
						{
							if (matchTerm( fi, oe, arg[argidx], fi->pos))
							{
								lastmatchpos = fi->pos;
								argidx++;
								if (argidx == arg.size()) break;
							}
						}
						// Check if matched and check the structure delimiter term
						if (argidx == arg.size()
						&& !(delimiter_term && matchTerm( oi, oe, delimiter_term, lastmatchpos)))
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
							std::vector<unsigned int> poset;	// .. linear search because sets are small

							unsigned int maxpos = oi->pos;
							for (ai=argidx; ai<arg.size(); ++ai)
							{
								std::vector<RandomDoc::Occurrence>::const_iterator
									fi = findTerm( oi, oe, arg[ai], lastpos);
								for (;;)
								{
									if (fi == oe) break;
									if (std::find( poset.begin(), poset.end(), fi->pos)==poset.end())
									{
										// ... only items at distinct positions are allowed
										break;
									}
									fi = findTerm( fi+1, oe, arg[ai], lastpos);
								}
								if (fi == oe) break;
								if (fi->pos > maxpos)
								{
									maxpos = fi->pos;
								}
								poset.push_back( fi->pos);
							}
							if (ai == arg.size()
							&& !(delimiter_term && matchTerm( oi, oe, delimiter_term, maxpos)))
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

	static std::vector<Match> resultMatches( strus::PostingIteratorInterface* itr)
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
			rt << " " << term.tostring() << " (" << arg[ai] << ")";
		}
		return rt.str();
	}

	bool execute( std::vector<Match>& result, strus::QueryProcessorInterface* queryproc, const RandomCollection& collection) const
	{
		unsigned int nofitr = arg.size();
		const strus::PostingIteratorInterface* itr[ MaxNofArgs];
		for (unsigned int ai=0; ai<nofitr; ++ai)
		{
			const TermCollection::Term& term = collection.termCollection.termar[ arg[ai]-1];
			itr[ ai] = queryproc->createTermPostingIterator( term.type, term.value);
			if (!itr[ ai])
			{
				std::cerr << "ERROR term not found [" << arg[ai] << "]: " << term.type << " '" << term.value << "'" << std::endl;
				std::cerr << "ERROR random query operation failed: " << tostring( collection) << std::endl;
				return false;
			}
		}
		std::string opname( operationName());
		strus::PostingIteratorInterface* res = 
			queryproc->createJoinPostingIterator(
					opname, range, std::size_t(nofitr), &itr[0]);

		result = resultMatches( res);
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

static bool compareMatches( const std::vector<RandomQuery::Match>& res, const std::vector<RandomQuery::Match>& matchar, const RandomCollection& collection)
{
	std::vector<RandomQuery::Match>::const_iterator mi = matchar.begin(), me = matchar.end();
	std::vector<RandomQuery::Match>::const_iterator ri = res.begin(), re = res.end();
	for (; mi != me && ri != re; ++mi,++ri)
	{
		if (mi->docno < ri->docno)
		{
			std::cerr << "ERROR match missed in doc " << mi->docno << " at " << mi->pos << std::endl;
			std::cerr << "summary: " << std::endl;
			std::cerr << collection.docSummary( mi->docno, mi->pos, 10);
			return false;
		}
		if (mi->docno > ri->docno)
		{
			std::cerr << "ERROR unexpected match in doc " << ri->docno << " at " << ri->pos << std::endl;
			std::cerr << "summary: " << std::endl;
			std::cerr << collection.docSummary( ri->docno, ri->pos, 10);
			return false;
		}
		if (mi->pos < ri->pos)
		{
			std::cerr << "ERROR match missed in doc " << mi->docno << " at " << mi->pos << std::endl;
			std::cerr << "summary: " << std::endl;
			std::cerr << collection.docSummary( mi->docno, mi->pos, 10);
			return false;
		}
		if (mi->pos > ri->pos)
		{
			std::cerr << "ERROR unexpected match in doc " << ri->docno << " at " << ri->pos << std::endl;
			std::cerr << "summary: " << std::endl;
			std::cerr << collection.docSummary( ri->docno, ri->pos, 10);
			return false;
		}
	}
	if (mi != me)
	{
		std::cerr << "ERROR match missed in doc " << mi->docno << " at " << mi->pos << std::endl;
		std::cerr << "summary: " << std::endl;
		std::cerr << collection.docSummary( mi->docno, mi->pos, 10);
		return false;
	}
	if (ri != re)
	{
		std::cerr << "ERROR unexpected match in doc " << ri->docno << " at " << ri->pos << std::endl;
		return false;
	}
	return true;
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

static std::string doubleToString( double val_)
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
		std::cerr << "ERROR too few parameters" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	else if (argc > 6)
	{
		std::cerr << "ERROR too many parameters" << std::endl;
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
		strus::Index totNofDocuments = 0;
		strus::Index totTermStringSize = 0;
		unsigned int insertIntervallSize = 1000;
		unsigned int insertIntervallCnt = 0;

		std::vector<RandomDoc>::const_iterator di = collection.docar.begin(), de = collection.docar.end();
		for (; di != de; ++di,++totNofDocuments)
		{
			typedef boost::scoped_ptr<strus::StorageInserterInterface> StorageInserter;

			StorageInserter inserter( storage->createInserter( di->docid));
			std::vector<RandomDoc::Occurrence>::const_iterator oi = di->occurrencear.begin(), oe = di->occurrencear.end();

			for (; oi != oe; ++oi,++totNofOccurrencies)
			{
				const TermCollection::Term& term = collection.termCollection.termar[ oi->term-1];
				inserter->addTermOccurrence( term.type, term.value, oi->pos, 0.0);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "term [" << oi->term << "] type '" << term.type << "' value '" << term.value << "' pos " << oi->pos << std::endl;
#endif
				totTermStringSize += term.value.size();
			}
			inserter->done();

#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "inserted document '" << di->docid << "' size " << di->occurrencear.size() << std::endl;
#endif
			if (++insertIntervallCnt == insertIntervallSize)
			{
				insertIntervallCnt = 0;
				std::cerr << "inserted " << (totNofDocuments+1) << " documents, " << totTermStringSize <<" bytes " << std::endl;
			}
		}
		storage->flush();

		std::cerr << "inserted collection with " << totNofDocuments << " documents, " << totNofOccurrencies << " occurrencies, " << totTermStringSize << " bytes" << std::endl;
		boost::scoped_ptr<strus::QueryProcessorInterface> queryproc(
			strus::createQueryProcessorInterface( storage.get()));

		std::vector<RandomQuery> randomQueryAr;
		if (collection.docar.size())
		{
			for (std::size_t qi=0; qi < nofQueries; ++qi)
			{
				randomQueryAr.push_back( RandomQuery( collection));
			}
		}
		else if (nofQueries)
		{
			std::cerr << "ERROR cannot do random queries on empty collection" << std::endl;
			nofQueries = 0;
		}
		if (nofQueries)
		{
			std::clock_t start;
			double duration;
			unsigned int nofQueriesFailed = 0;
			start = std::clock();
			std::vector<std::vector<RandomQuery::Match> > result_matches;
			std::vector<RandomQuery>::const_iterator qi = randomQueryAr.begin(), qe = randomQueryAr.end();
			double arglen = 0.0;
			for (; qi != qe; ++qi)
			{
				result_matches.push_back( std::vector<RandomQuery::Match>());
				if (!qi->execute( result_matches.back(), queryproc.get(), collection))
				{
					++nofQueriesFailed;
				}
				arglen += qi->arg.size();
			}
			duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
			std::cerr << "evaluated " << nofQueries << " random query operations in " << doubleToString(duration) << " seconds" << std::endl;
			arglen /= nofQueries;
			std::cerr << "average query size = " << doubleToString( arglen) << std::endl;

			qi = randomQueryAr.begin(), qe = randomQueryAr.end();
			std::vector<std::vector<RandomQuery::Match> >::const_iterator ri = result_matches.begin(), re = result_matches.end();
			std::size_t rsum = 0;
			std::size_t rcnt = 0;
			for (; ri != re && qi != qe; ++qi,++ri)
			{
				arglen += qi->arg.size();
				std::vector<RandomQuery::Match> expected_matches = qi->expectedMatches( collection);
				if (!compareMatches( *ri, expected_matches, collection))
				{
					std::cerr << "ERROR random query operation failed: " << qi->tostring( collection) << std::endl;
					return false;
				}
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "random query operation " << tostring( collection) << " " << matches.size() << " matches" << std::endl;
#endif
				if (++rcnt >= 100)
				{
					rsum += rcnt;
					rcnt = 0;
					std::cerr << ".";
				}
			}
			rsum += rcnt;
			std::cerr << std::endl;
			std::cerr << "verified " << rsum << " query results" << std::endl;
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


