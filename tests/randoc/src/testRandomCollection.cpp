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
#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"
#include "strus/lib/queryproc.hpp"
#include "strus/lib/queryeval.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/base/local_ptr.hpp"
#include "random.hpp"
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
#include <stdio.h>
#include <memory>
#include "strus/base/stdint.h"

#undef STRUS_LOWLEVEL_DEBUG
#undef STRUS_GENERATE_READABLE_NAMES

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

#ifdef STRUS_GENERATE_READABLE_NAMES
static const char* readableNameIndexString( std::size_t maxval)
{
	if (maxval < 10) return "%01u";
	if (maxval < 100) return "%02u";
	if (maxval < 1000) return "%03u";
	if (maxval < 10000) return "%04u";
	if (maxval < 100000) return "%05u";
	if (maxval < 1000000) return "%06u";
	if (maxval < 10000000) return "%07u";
	return "%09u";
}
#else
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
#endif

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
#ifdef STRUS_GENERATE_READABLE_NAMES
			char termformatbuf[ 64];
			char termnamebuf[ 64];

			snprintf( termformatbuf, sizeof(termformatbuf), "T%s", readableNameIndexString( nofTerms-1));
			snprintf( termnamebuf, sizeof(termnamebuf), termformatbuf, (unsigned int)termSet.size());
			termSet.insert( termnamebuf);
#else
			termSet.insert( randomTerm());
#endif
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

	static const char* getDocIdFormatString( unsigned int nofDocs)
	{
		if (nofDocs < 10)        return "doc%1u";
		if (nofDocs < 100)       return "doc%02u";
		if (nofDocs < 1000)      return "doc%03u";
		if (nofDocs < 10000)     return "doc%04u";
		if (nofDocs < 100000)    return "doc%05u";
		if (nofDocs < 1000000)   return "doc%06u";
		if (nofDocs < 10000000)  return "doc%07u";
		if (nofDocs < 100000000) return "doc%08u";
		throw std::runtime_error("too many documents for random document collection");
	}

	explicit RandomDoc( unsigned int docid_, unsigned int nofDocs_, const TermCollection& collection, unsigned int size)
	{
		const char* docid_formatstring = getDocIdFormatString( nofDocs_);
		char docidstr[ 64];
		snprintf( docidstr, sizeof(docidstr), docid_formatstring, docid_);
		docid.append( docidstr);

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

	float IDF = ::log10( collSize / (nofMatchingDocs + 1.0));
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
		for (unsigned int di = 0; di < nofDocuments; ++di)
		{
			unsigned int tiny_docsize  = g_random.get( 2, 3 + (maxDocumentSize/16)) + (maxDocumentSize/16);
			unsigned int small_docsize = g_random.get( 2, 3 + (maxDocumentSize/8)) + (maxDocumentSize/8);
			unsigned int med_docsize   = g_random.get( 2, 3 + (maxDocumentSize/4)) + (maxDocumentSize/4);
			unsigned int big_docsize   = g_random.get( 2, 3 + (maxDocumentSize/2)) + (maxDocumentSize/2);

			unsigned int docsize = g_random.get( 2, maxDocumentSize, 4, tiny_docsize, small_docsize, med_docsize, big_docsize);
			docar.push_back( RandomDoc( di+1, nofDocuments, termCollection, docsize));
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

	std::string docSummary( std::size_t docidx_, unsigned int pos_, unsigned int range) const
	{
		std::ostringstream rt;
		const RandomDoc& doc = docar[ docidx_];
		std::vector<RandomDoc::Occurrence>::const_iterator
			oi = doc.occurrencear.begin(), oe = doc.occurrencear.end();
		for (; oi != oe; ++oi)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			rt << " " << oi->pos << ": " << termCollection.termar[ oi->term-1].tostring() << " (" << oi->term << ")";
			rt << std::endl;
#else
			if (oi->pos+1 >= pos_ && oi->pos <= pos_ + range)
			{
				rt << " " << oi->pos << ": " << termCollection.termar[ oi->term-1].tostring() << " (" << oi->term << ")";
				rt << std::endl;
			}
#endif
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
		cardinality = 0;
		operation = (Operation)g_random.get( 0, NofOperations);
		std::size_t pickDocIdx = g_random.get( 0, collection.docar.size());
		const RandomDoc& pickDoc = collection.docar[ pickDocIdx];

		switch (operation)
		{
			case Contains:
			{
				cardinality = 1;
				unsigned int pi = g_random.get( 0, pickDoc.occurrencear.size());
				arg.push_back( pickDoc.occurrencear[pi].term);

				while (arg.size() < (unsigned int)MaxNofArgs)
				{
					unsigned int sel = g_random.get( 0, 4);
					if (sel == 0)
					{
						pi = g_random.get( 0, pickDoc.occurrencear.size());
						arg.push_back( pickDoc.occurrencear[pi].term);
						cardinality += 1;
					}
					else if (sel <= 2)
					{
						unsigned int nofTerms = collection.termCollection.termar.size();
						unsigned int termidx = 1+g_random.get( 0, nofTerms);
						arg.push_back( termidx);
					}
					else
					{
						break;
					}
				}
				break;
			}
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
				if (operation == StructWithin || operation == Within)
				{
					while (maxNofPicks > 4) maxNofPicks /= 2;
					// ... avoid too big ranges for within because we create all permutations
					//	of configurations in the test. The operator implementation is smarter
					//	though, but we want to test with a different solution of the
					//	problem without any optimizations.
				}
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
		:operation(o.operation),arg(o.arg),range(o.range),cardinality(o.cardinality){}

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

		bool operator < (const Match& o) const
		{
			if (docno == o.docno) return pos < o.pos;
			return docno < o.docno;
		}
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

	static void getPermutations_( std::vector< std::vector<unsigned char> >& res, unsigned char* ar, unsigned char pos, unsigned char size)
	{
		if (pos == size)
		{
			// when we have a permutation, we add it to the result:
			res.push_back( std::vector<unsigned char>( ar, ar+(std::size_t)size));
		}
		else for (unsigned char ii=pos; ii<size; ++ii)
		{
			// select an element and permute the others:
			std::swap( ar[ ii], ar[ pos]);
			getPermutations_( res, ar, pos+1, size);
			std::swap( ar[ ii], ar[ pos]);
		}
	}

	static std::vector< std::vector<unsigned char> > getPermutations( std::size_t size)
	{
		unsigned char ar[ 256];
		if (size > 256) throw std::runtime_error("permutation exceeds maximum size");
		for (unsigned int ii=0; ii<size; ++ii)
		{
			ar[ ii] = (unsigned char)ii;
		}
		std::vector< std::vector<unsigned char> > rt;
		getPermutations_( rt, ar, 0, (unsigned char)size);
		return rt;
	}

	std::vector<Match> expectedMatches( const RandomCollection& collection, const std::vector<strus::Index>& docnomap) const
	{
		std::vector<Match> rt;
		std::vector<RandomDoc>::const_iterator di = collection.docar.begin(), de = collection.docar.end();
		for (unsigned int docidx=0; di != de; ++di,++docidx)
		{
			unsigned int docno = docnomap[ docidx];

			if (operation == Contains)
			{
				// ... no position involved, handle document matches:
				std::vector<unsigned int>::const_iterator ai = arg.begin(), ae = arg.end();
				unsigned int cardinality_cnt = 0;
				for (; ai!=ae; ++ai)
				{
					std::vector<RandomDoc::Occurrence>::const_iterator oi = di->occurrencear.begin(), oe = di->occurrencear.end();
					for (; oi != oe && oi->term != *ai; ++oi){}
					if (oi != oe)
					{
						//... match found
						cardinality_cnt += 1;
					}
				}
				if (cardinality_cnt >= cardinality)
				{
					rt.push_back( Match( docno, 1));
				}
			}
			std::vector<RandomDoc::Occurrence>::const_iterator oi = di->occurrencear.begin(), oe = di->occurrencear.end();
			for (; oi != oe; (void)skipNextPosition(oi,oe))
			{
				switch (operation)
				{
					case Contains:
					{
						// ... already handled in document matches
						break;
					}
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
								rt.push_back( Match( docno, lastmatchpos));
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
						// Get all permutations and try to match sequence for each:
						std::vector< std::vector<unsigned char> >
							permutations = getPermutations( arg.size() - argidx);
						std::vector< std::vector<unsigned char> >::const_iterator
							pi = permutations.begin(), pe = permutations.end();
						for (; pi != pe; ++pi)
						{
							std::vector<unsigned char>::const_iterator
								ei = pi->begin(), ee = pi->end();
							if (!matchTerm( oi, oe, arg[argidx+*ei], oi->pos)) continue;

							std::vector<RandomDoc::Occurrence>::const_iterator fi = oi;
							(void)skipNextPosition(fi,oe);

							unsigned int lastmatchpos = oi->pos;
							for (++ei; ei != ee && fi != oe && fi->pos <= lastpos; (void)skipNextPosition(fi,oe))
							{
								if (matchTerm( fi, oe, arg[argidx+*ei], fi->pos))
								{
									lastmatchpos = fi->pos;
									ei++;
								}
							}
							// Check if matched and check the structure delimiter term
							if (ei == ee
							&& !(delimiter_term && matchTerm( oi, oe, delimiter_term, lastmatchpos)))
							{
								if (range >= 0)
								{
									rt.push_back( Match( docno, oi->pos));
									break; //... we need only one result per permutation
								}
								else
								{
									rt.push_back( Match( docno, lastmatchpos));
									break; //... we need only one result per permutation
								}
							}
						}
						break;
					}
				}
			}
		}
		std::sort( rt.begin(), rt.end());
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
			rt << " range " << range;
		}
		if (cardinality)
		{
			rt << " cardinality " << cardinality;
		}
		for (unsigned int ai=0; ai<arg.size(); ++ai)
		{
			const TermCollection::Term& term = collection.termCollection.termar[ arg[ai]-1];
			rt << " " << term.tostring() << " (" << arg[ai] << ")";
		}
		return rt.str();
	}

	bool execute( std::vector<Match>& result, strus::StorageClientInterface* storage, strus::QueryProcessorInterface* queryproc, const RandomCollection& collection) const
	{
		unsigned int nofitr = arg.size();
		std::vector<strus::Reference<strus::PostingIteratorInterface> > itrar;
		for (unsigned int ai=0; ai<nofitr; ++ai)
		{
			const TermCollection::Term& term = collection.termCollection.termar[ arg[ai]-1];
			strus::Reference<strus::PostingIteratorInterface> itr(
				storage->createTermPostingIterator( term.type, term.value, 1));
			if (!itr.get())
			{
				std::cerr << "ERROR term not found [" << arg[ai] << "]: " << term.type << " '" << term.value << "'" << std::endl;
				std::cerr << "ERROR random query operation failed: " << tostring( collection) << std::endl;
				return false;
			}
			itrar.push_back( itr);
		}
		std::string opname( operationName());
		const strus::PostingJoinOperatorInterface* joinop =
			queryproc->getPostingJoinOperator( opname);
		if (!joinop)
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		strus::PostingIteratorInterface* res = 
			joinop->createResultIterator( itrar, range, cardinality);
		if (!res)
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		result = resultMatches( res);
		delete res;
		return true;
	}

	enum Operation
	{
		Contains,
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
		static const char* ar[] = {"contains","intersect","union","diff","within","within_struct","sequence","sequence_struct"};
		return ar[op];
	}
	const char* operationName() const
	{
		return operationName( operation);
	}

	Operation operation;
	std::vector<unsigned int> arg;
	int range;
	unsigned int cardinality;
};

static std::size_t getDocidx( const std::vector<strus::Index>& docnomap, const strus::Index& docno)
{
	std::vector<strus::Index>::const_iterator di = docnomap.begin(), de = docnomap.end();
	for (; di != de; ++di)
	{
		if (*di == docno) return di-docnomap.begin();
	}
	throw std::runtime_error( "internal: document not found (docno -> docidx map)");
}

static bool compareMatches( const std::vector<RandomQuery::Match>& res, const std::vector<RandomQuery::Match>& matchar, const RandomCollection& collection, const std::vector<strus::Index>& docnomap)
{
	enum {MaxSummarySize=100};
	std::vector<RandomQuery::Match>::const_iterator mi = matchar.begin(), me = matchar.end();
	std::vector<RandomQuery::Match>::const_iterator ri = res.begin(), re = res.end();
	for (; mi != me && ri != re; ++mi,++ri)
	{
		if (mi->docno < ri->docno || (mi->docno == ri->docno && mi->pos < ri->pos))
		{
			std::size_t docidx = getDocidx( docnomap, mi->docno);
			std::string docid = collection.docar[ docidx].docid;
			std::cerr << "ERROR match missed [diff] in document " << docid << " docno (" << mi->docno << ") at " << mi->pos << std::endl;
			std::cerr << "summary: " << std::endl;
			std::cerr << collection.docSummary( docidx, mi->pos, MaxSummarySize);
			return false;
		}
		if (mi->docno > ri->docno || (mi->docno == ri->docno && mi->pos > ri->pos))
		{
			std::size_t docidx = getDocidx( docnomap, ri->docno);
			std::string docid = collection.docar[ docidx].docid;
			std::cerr << "ERROR unexpected match [diff] in doc " << docid << " docno (" << ri->docno << ") at " << ri->pos << std::endl;
			std::cerr << "summary: " << std::endl;
			std::cerr << collection.docSummary( getDocidx( docnomap, ri->docno), ri->pos, MaxSummarySize);
			return false;
		}
	}
	if (mi != me)
	{
		std::size_t docidx = getDocidx( docnomap, mi->docno);
		std::string docid = collection.docar[ docidx].docid;
		std::cerr << "ERROR match missed [eof] in doc " << docid << " docno (" << mi->docno << ") at " << mi->pos << std::endl;
		std::cerr << "summary: " << std::endl;
		std::cerr << collection.docSummary( getDocidx( docnomap, mi->docno), mi->pos, MaxSummarySize);
		return false;
	}
	if (ri != re)
	{
		std::size_t docidx = getDocidx( docnomap, ri->docno);
		std::string docid = collection.docar[ docidx].docid;
		std::cerr << "ERROR unexpected match [eof] in doc " << docid << " docno (" << ri->docno << ") at " << ri->pos << std::endl;
		std::cerr << collection.docSummary( getDocidx( docnomap, ri->docno), ri->pos, MaxSummarySize);
		return false;
	}
	return true;
}

/// \brief the assignement of document numbers is not transparent and the document numbers get not assigned
///	in ascending order of insertion. We have to query the storage to get the document numbers assigned.
static std::vector<strus::Index> getDocnoMap(
		const strus::StorageClientInterface* storage,
		const std::vector<RandomDoc>& docar)
{
	std::vector<strus::Index> rt;
	rt.resize( docar.size(), 0);
	std::size_t docidx=0;
	for (; docidx<docar.size(); ++docidx)
	{
		strus::Index docno = storage->documentNumber( docar[ docidx].docid);
		if (docno <= 0 || docno > (strus::Index)rt.size()) throw std::runtime_error( std::string( "unexpected document number for docid ") + docar[ docidx].docid);
		if (rt[ docidx] != 0) throw std::runtime_error( std::string( "duplicated document number for docid ") + docar[ docidx].docid);
		rt[ docidx] = docno;
	}
	return rt;
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
	unsigned int val = (unsigned int)::floor( val_ * 1000);
	unsigned int val_sec = val / 1000;
	unsigned int val_ms = val % 1000;
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
	g_errorhnd = strus::createErrorBuffer_standard( stderr, 1);
	if (!g_errorhnd)
	{
		std::cerr << "construction of error buffer failed" << std::endl;
		return -1;
	}
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

		strus::local_ptr<strus::DatabaseInterface> dbi( strus::createDatabaseType_leveldb( g_errorhnd));
		if (!dbi.get())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		strus::local_ptr<strus::StorageInterface> sti( strus::createStorageType_std( g_errorhnd));
		if (!sti.get() || g_errorhnd->hasError())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		(void)dbi->destroyDatabase( config);
		(void)g_errorhnd->fetchError();

		sti->createStorage( config, dbi.get());
		{
			const strus::StatisticsProcessorInterface* statisticsMessageProc = 0;
			strus::local_ptr<strus::StorageClientInterface>
				storage( sti->createClient( config, dbi.get(), statisticsMessageProc));
			if (!storage.get())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}

			RandomCollection collection( nofFeatures, nofDocuments, maxDocumentSize);
	
			strus::Index totNofOccurrencies = 0;
			strus::Index totNofDocuments = 0;
			strus::Index totTermStringSize = 0;
			unsigned int insertIntervallSize = 1000;
			unsigned int insertIntervallCnt = 0;
	
			typedef strus::local_ptr<strus::StorageTransactionInterface> StorageTransaction;
			StorageTransaction transaction( storage->createTransaction());
			if (!transaction.get())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}
			std::vector<RandomDoc>::const_iterator di = collection.docar.begin(), de = collection.docar.end();
			for (; di != de; ++di,++totNofDocuments)
			{
				typedef strus::local_ptr<strus::StorageDocumentInterface> StorageDocument;
	
				StorageDocument doc( transaction->createDocument( di->docid));
				if (!doc.get())
				{
					throw std::runtime_error( g_errorhnd->fetchError());
				}
				std::vector<RandomDoc::Occurrence>::const_iterator oi = di->occurrencear.begin(), oe = di->occurrencear.end();
	
				for (; oi != oe; ++oi,++totNofOccurrencies)
				{
					const TermCollection::Term& term = collection.termCollection.termar[ oi->term-1];
					doc->addForwardIndexTerm( term.type, term.value, oi->pos);
					doc->addSearchIndexTerm( term.type, term.value, oi->pos);
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "term [" << oi->term << "] type '" << term.type << "' value '" << term.value << "' pos " << oi->pos << std::endl;
#endif
					totTermStringSize += term.value.size();
				}
				doc->done();
	
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "inserted document '" << di->docid << "' size " << di->occurrencear.size() << std::endl;
#endif
				if (++insertIntervallCnt == insertIntervallSize)
				{
					insertIntervallCnt = 0;
					std::cerr << "inserted " << (totNofDocuments+1) << " documents, " << totTermStringSize <<" bytes " << std::endl;
				}
			}
			if (g_errorhnd->hasError())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}
			transaction->commit();

			// Calculate the map that assigns document ids from the random collection to document numbers:
			std::vector<strus::Index> docnomap = getDocnoMap( storage.get(), collection.docar); 
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "docid to docno map:";
			std::vector<strus::Index>::const_iterator ni=docnomap.begin(), ne=docnomap.end();
			for (; ni != ne; ++ni)
			{
				std::cout << " " << collection.docar[(ni-docnomap.begin())].docid << ":" << *ni;
			}
			std::cout << std::endl;
#endif
			std::cerr << "inserted collection with " << totNofDocuments << " documents, " << totNofOccurrencies << " occurrencies, " << totTermStringSize << " bytes" << std::endl;
			strus::local_ptr<strus::QueryProcessorInterface> 
				queryproc( strus::createQueryProcessor( g_errorhnd));
			if (!queryproc.get())
			{
				throw std::runtime_error( g_errorhnd->fetchError());
			}

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
				std::clock_t start = std::clock();
				unsigned int nofQueriesFailed = 0;
				std::vector<std::vector<RandomQuery::Match> > result_matches;
				std::vector<RandomQuery>::const_iterator qi = randomQueryAr.begin(), qe = randomQueryAr.end();
				double arglen = 0.0;
				for (; qi != qe; ++qi)
				{
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cout << "execute query " << qi->tostring( collection) << std::endl;
#endif
					result_matches.push_back( std::vector<RandomQuery::Match>());
					if (!qi->execute( result_matches.back(), storage.get(), queryproc.get(), collection))
					{
						++nofQueriesFailed;
					}
					arglen += qi->arg.size();
				}
				double duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
				std::cerr << "evaluated " << nofQueries << " random query operations in " << doubleToString(duration) << " seconds" << std::endl;
				arglen /= nofQueries;
				std::cerr << "average query size = " << doubleToString( arglen) << std::endl;

				if (randomQueryAr.size() != result_matches.size())
				{
					std::cerr << "ERROR number of queries and results do not match" << std::endl;
					return -1;
				}
				qi = randomQueryAr.begin(), qe = randomQueryAr.end();
				std::vector<std::vector<RandomQuery::Match> >::const_iterator
					ri = result_matches.begin(), re = result_matches.end();
				std::size_t rsum = 0;
				std::size_t rcnt = 0;
				for (; ri != re && qi != qe; ++qi,++ri)
				{
					arglen += qi->arg.size();
					std::vector<RandomQuery::Match> expected_matches
						= qi->expectedMatches( collection, docnomap);

#ifdef STRUS_LOWLEVEL_DEBUG
					std::cout << "query " << qi->tostring( collection) << std::endl;
					std::vector<RandomQuery::Match>::const_iterator
						xi = expected_matches.begin(),
						xe = expected_matches.end();
					for (; xi != xe; ++xi)
					{
						std::cout << "\texpected match docno " << xi->docno << " pos " << xi->pos << std::endl;
					}
					xi = ri->begin(), xe = ri->end();
					for (; xi != xe; ++xi)
					{
						std::cout << "\tresult match docno " << xi->docno << " pos " << xi->pos << std::endl;
					}
#endif
					// Compare the matches:
					if (!compareMatches( *ri, expected_matches, collection, docnomap))
					{
						std::cerr << "ERROR random query operation failed: " << qi->tostring( collection) << std::endl;
						return -1;
					}
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "random query operation " << qi->tostring( collection) << " " << result_matches.size() << " matches" << std::endl;
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
				return (nofQueriesFailed?2:0);
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
	return 4;
}


