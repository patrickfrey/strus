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
#include "strus/libstrus_storage.hpp"
#include "strus/libstrus_queryproc.hpp"
#include "strus/iteratorInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
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
#include <boost/scoped_ptr.hpp>

/// \brief Pseudo random generator 
enum {KnuthIntegerHashFactor=2654435761U};
#undef STRUS_LOWLEVEL_DEBUG

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
		return iv?((m_value % iv) + min_):min_;
	}

private:
	unsigned int m_value;
};

Random g_random;

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
	while (g_random.get( 1, 10) < 4)
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

		unsigned int pos = 1;
		while (pos <= size)
		{
			unsigned int term = g_random.get( 1, collection.termar.size()) - 1;
			occurrencear.push_back( Occurrence( term, pos));
			if (g_random.get( 0, 4) > 1) pos++;
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
					++termDocumentFrequencyMap[ oi->term];
				}
				++termCollectionFrequencyMap[ oi->term];
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
				unsigned int nofMatchingDocs = termDocumentFrequencyMap[ oi->term];
				unsigned int nofMatchesInDoc = matchcount[ oi->term];
				unsigned int docLen = doc.occurrencear.size();

				doc.weightmap[ oi->term] = tfIdf( collSize, nofMatchingDocs, nofMatchesInDoc, docLen, avgDocLen);
			}
		}
	}

	TermCollection termCollection;
	std::vector<RandomDoc> docar;
};

struct RandomQuery
{
	RandomQuery( const RandomCollection& collection)
		:flags(0)
	{
	AGAIN:
		operation = (Operation)g_random.get( 0, NofOperations);
		std::size_t pickDocIdx = g_random.get( 0, collection.docar.size());
		const RandomDoc& pickDoc = collection.docar[ pickDocIdx];

		switch (operation)
		{
			case Intersect:
			{
				unsigned int pi = g_random.get( 0, pickDoc.occurrencear.size());
				for (; pi+1 < pickDoc.occurrencear.size(); ++pi)
				{
					if (pickDoc.occurrencear[pi].pos == pickDoc.occurrencear[pi+1].pos)
					{
						arg.push_back( pickDoc.occurrencear[pi+0].term);
						arg.push_back( pickDoc.occurrencear[pi+1].term);
						break;
					}
				}
				if (arg.empty())
				{
					goto AGAIN;
				}
			}
			case Union:
			{
				unsigned int nn = g_random.get( 1, 5);
				unsigned int ii = 0;

				for (; ii<nn; ++ii)
				{
					unsigned int pickOccIdx = g_random.get( 0, pickDoc.occurrencear.size());
					const RandomDoc::Occurrence& pickOcc = pickDoc.occurrencear[ pickOccIdx];
					arg.push_back( pickOcc.term);
				}
				break;
			}
			case CutInRange:
			{
				bool withCut = g_random.get( 0, 1);
				bool withFirstRange = g_random.get( 0, 1);
				bool withFirstCut = g_random.get( 0, 1);
				bool withLastCut = g_random.get( 0, 1);
				flags |= (withFirstRange?0x1:0x0);
				flags |= (withFirstCut?0x2:0x0);
				flags |= (withLastCut?0x4:0x0);

				unsigned int minRange = 0;
				if (!withFirstRange) minRange++;
				if (!withFirstCut) minRange++;
				if (!withLastCut) minRange++;
				if (minRange >= pickDoc.occurrencear.size())
				{
					goto AGAIN;
				}
				unsigned int pickOccIdx = g_random.get( 0, pickDoc.occurrencear.size());
				const RandomDoc::Occurrence& pickOcc = pickDoc.occurrencear[ pickOccIdx];

				unsigned int rangeOccIdx = g_random.get( 0, pickDoc.occurrencear.size() - pickOccIdx);
				const RandomDoc::Occurrence& rangeOcc = pickDoc.occurrencear[ rangeOccIdx];
				if (rangeOcc.term == pickOcc.term)
				{
					withFirstRange = false;
				}
				if (pickOcc.pos + minRange > rangeOcc.pos) goto AGAIN;
				
				for (unsigned int ti = pickOccIdx+(withFirstRange?0:1); ti < rangeOccIdx; ++ti)
				{
					if (pickDoc.occurrencear[ ti].term == rangeOcc.term)
					{
						goto AGAIN;
					}
				}
				arg.push_back( pickOcc.term);
				arg.push_back( rangeOcc.term);
				arg.push_back( rangeOcc.pos - pickOcc.pos);
				if (withCut)
				{
					unsigned int cutOccIdx = pickOccIdx + g_random.get( 0, rangeOccIdx - pickOccIdx);
					const RandomDoc::Occurrence& cutOcc = pickDoc.occurrencear[ cutOccIdx];

					arg.push_back( cutOcc.term);
				}
				break;
			}
		}
	}
	RandomQuery( const RandomQuery& o)
		:operation(o.operation),arg(o.arg){}

	struct Match
	{
		unsigned int docno;
		unsigned int pos;

		Match( const Match& o)
			:docno(o.docno),pos(o.pos){}
		Match( unsigned int docno_, unsigned int pos_)
			:docno(docno_),pos(pos_){}
	};

	std::vector<Match> evaluate( const RandomCollection& collection) const
	{
		std::vector<Match> rt;
		std::vector<RandomDoc>::const_iterator di = collection.docar.begin(), de = collection.docar.end();
		for (unsigned int docno=1; di != de; ++di,++docno)
		{
			std::vector<RandomDoc::Occurrence>::const_iterator oi = di->occurrencear.begin(), oe = di->occurrencear.end();
			for (; oi != oe; ++oi)
			{
				switch (operation)
				{
					case Intersect:
					{
						unsigned int term1 = arg[0];
						unsigned int term2 = arg[1];
						if (oi->term == term1)
						{
							std::vector<RandomDoc::Occurrence>::const_iterator fi = oi;
							for (++fi; fi != oe && fi->pos == oi->pos; ++fi)
							{
								if (fi->term == term2)
								{
									rt.push_back( Match( docno, oi->pos));
									break;
								}
							}
						}
						break;
					}
					case Union:
						if (oi->term == arg[0]
						||  oi->term == arg[1])
						{
							rt.push_back( Match( docno, oi->pos));
						}
						break;

					case CutInRange:
					{
						unsigned int term1 = arg[0];
						unsigned int term2 = arg[1];
						bool withFirstRange = (flags&0x1)!=0;
						bool withFirstCut = (flags&0x2)!=0;
						bool withLastCut = (flags&0x4)!=0;
						unsigned int range = arg[2];
						unsigned int cut = (arg.size() > 3)?arg[3]:0;

						if (oi->term == term1)
						{
							unsigned int cutPos = 0;
							unsigned int matchPos = 0;
							std::vector<RandomDoc::Occurrence>::const_iterator fi = oi;
							for (++fi; fi != oe && fi->pos <= oi->pos + range; ++fi)
							{
								if (fi->term == cut && !cutPos)
								{
									if (withFirstCut || fi->pos != oi->pos)
									{
										cutPos = fi->pos;
									}
								}
								if (fi->term == term2 && !matchPos)
								{
									if (withFirstRange || fi->pos != oi->pos)
									{
										matchPos = fi->pos;
									}
								}
							}
							if (matchPos)
							{
								if (!withLastCut || matchPos != cutPos)
								{
									rt.push_back( Match( docno, oi->pos));
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

	bool compareMatches( const std::vector<Match>& matchar, strus::IteratorInterface* itr) const
	{
		std::vector<Match>::const_iterator mi = matchar.begin(), me = matchar.end();
		for (; mi != me; ++mi)
		{
			
		}
		return true;
	}

	enum Operation
	{
		Intersect,
		Union,
		CutInRange
	};
	enum {NofOperations=3};
	static const char* operationName( Operation op)
	{
		static const char* ar[] = {"intersect","union","cirange"};
		return ar[op];
	}
	const char* operationName() const
	{
		return operationName( operation);
	}

	Operation operation;
	std::vector<unsigned int> arg;
	unsigned int flags;
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

		std::size_t totNofOccurrencies = 0;
		std::size_t totNofFeatures = 0;
		std::size_t totNofDocuments = 0;
		std::size_t totTermStringSize = 0;

		std::vector<RandomDoc>::const_iterator di = collection.docar.begin(), de = collection.docar.end();
		for (; di != de; ++di,++totNofDocuments)
		{
			typedef boost::scoped_ptr<strus::StorageInterface::TransactionInterface> Transaction;

			Transaction transaction( storage->createTransaction( di->docid));
			std::vector<RandomDoc::Occurrence>::const_iterator oi = di->occurrencear.begin(), oe = di->occurrencear.end();

			for (; oi != oe; ++oi,++totNofOccurrencies)
			{
				TermCollection::Term& term = collection.termCollection.termar[ oi->term-1];
				transaction->addTermOccurrence( term.type, term.value, oi->pos);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "term type '" << term.type << "' value '" << term.value << " pos " << oi->pos << std::endl;
#endif
				totTermStringSize += term.value.size();
			}
			std::map<unsigned int, float>::const_iterator wi = di->weightmap.begin(), we = di->weightmap.end();
			for (; wi != we; ++wi,++totNofFeatures)
			{
				TermCollection::Term& term = collection.termCollection.termar[ wi->first-1];
				transaction->setTermWeight( term.type, term.value, wi->second);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "weigth type '" << term.type << "' value '" << term.value << " pos " << wi->second << std::endl;
#endif
			}
			transaction->commit();

#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "inserted document '" << di->docid << "' size " << di->occurrencear.size() << std::endl;
#endif
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
		{
			std::clock_t start;
			double duration;
		
			start = std::clock();

			std::vector<RandomQuery>::const_iterator qi = randomQueryAr.begin(), qe = randomQueryAr.end();
			for (; qi != qe; ++qi)
			{
				switch (qi->operation)
				{
					case RandomQuery::Union:
					case RandomQuery::Intersect:
					{
						unsigned int nofitr = qi->arg.size();
						const strus::IteratorInterface* itr[ 8];
						for (unsigned int ai=0; ai<nofitr; ++ai)
						{
							TermCollection::Term& term = collection.termCollection.termar[ qi->arg[ai]];
							itr[ ai] = queryproc->createIterator( term.type, term.value);
						}
						std::vector<std::string> options;
						std::string opname( qi->operationName());
						strus::IteratorInterface* res = 
							queryproc->createIterator(
									opname, options, std::size_t(nofitr), &itr[0]);

						if (!qi->compareMatches( qi->evaluate( collection), res))
						{
							throw std::runtime_error( "random query failed");
						}
						for (unsigned int ai=0; ai<nofitr; ++ai)
						{
							delete itr[ai];
						}
						delete res;
						break;
					}
					case RandomQuery::CutInRange:
						break;
				}
			}
			duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
			std::cerr << "evaluated " << nofQueries << " random queries in " << timeToString(duration) << " millieconds" << std::endl;
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


