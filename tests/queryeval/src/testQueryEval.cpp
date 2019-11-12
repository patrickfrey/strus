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
#include "strus/lib/filelocator.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/shared_ptr.hpp"
#include "strus/base/pseudoRandom.hpp"
#include "strus/base/stdint.h"
#include "strus/errorBufferInterface.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/queryEvalInterface.hpp"
#include "strus/queryInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/storageMetaDataTableUpdateInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/valueIteratorInterface.hpp"
#include "strus/queryResult.hpp"
#include "strus/reference.hpp"
#include "private/errorUtils.hpp"
#include <string>
#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <limits>
#include <algorithm>
#include <cmath>

static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::FileLocatorInterface* g_fileLocator = 0;
static bool g_verbose = false;
static strus::PseudoRandom g_random;
enum {MinRank=0, MaxNofRanks=100, MaxNofClasses=3, StorageCommitSize=100, MaxNofQueryTerms=12};

class PrimeFactors
{
public:
	enum {MinNumber=2};

	PrimeFactors( int nofNumbers)
		:m_ar( nofNumbers, std::vector<int>())
	{
		int mi = MinNumber, me=nofNumbers+MinNumber;
		for (; mi<me; ++mi)
		{
			std::vector<int>& tfactors = m_ar[ mi-MinNumber];
			if (tfactors.empty())
			{
				tfactors.push_back( mi);
				m_occurrencies[ mi] += 1;
			}
			int bi = MinNumber, be = mi+1;
			for (; bi < be && bi*mi < me; ++bi)
			{
				std::vector<int>& ufactors = m_ar[ (bi*mi)-MinNumber];
				std::vector<int>& bfactors = m_ar[ bi-MinNumber];
				if (ufactors.empty())
				{
					ufactors.insert( ufactors.end(), bfactors.begin(), bfactors.end());
					ufactors.insert( ufactors.end(), tfactors.begin(), tfactors.end());
					std::sort( ufactors.begin(), ufactors.end());
					std::set<int> uniqueset( ufactors.begin(), ufactors.end());
					std::set<int>::const_iterator ui = uniqueset.begin(), ue = uniqueset.end();
					for (; ui != ue; ++ui)
					{
						++m_occurrencies[ *ui];
					}
				}
			}
		}
	}

	int maxNumber() const
	{
		return m_ar.size() + MinNumber;
	}

	const std::vector<int>& factorList( int number) const
	{
		return m_ar[ number-MinNumber];
	}

	std::set<int> factorSet( int number) const
	{
		return std::set<int>( m_ar[ number-MinNumber].begin(), m_ar[ number-MinNumber].end());
	}

	std::map<int,int> factorMap( int number) const
	{
		std::map<int,int> rt;
		std::vector<int>::const_iterator fi = m_ar[ number-MinNumber].begin(), fe = m_ar[ number-MinNumber].end();
		for (; fi != fe; ++fi)
		{
			++rt[ *fi];
		}
		return rt;
	}

	std::map<int,int> queryFeatToFfMap( int number, const std::vector<int>& query) const
	{
		std::map<int,int> rt;
		const std::vector<int>& factors = factorList( number);
		std::vector<int>::const_iterator fi = factors.begin(), fe = factors.end();
		for (; fi != fe; ++fi)
		{
			std::vector<int>::const_iterator qi = query.begin(), qe = query.end();
			for (; qi != qe && *fi != *qi; ++qi){}
			if (qi != qe) ++rt[ *qi];
		}
		return rt;
	}

	int frequency( int number) const
	{
		std::map<int,int>::const_iterator oi = m_occurrencies.find( number);
		return (oi == m_occurrencies.end()) ? 0 : oi->second;
	}

	int size() const
	{
		return m_ar.size();
	}

	int randomPrime() const
	{
		for (;;)
		{
			int rt = g_random.get( PrimeFactors::MinNumber, (int)m_ar.size()+PrimeFactors::MinNumber);
			if (frequency( rt)) return rt;
		}
	}

	static std::string docid( int docno)
	{
		return strus::string_format( "D%d", docno + PrimeFactors::MinNumber);
	}

private:
	std::vector<std::vector<int> > m_ar;
	std::map<int,int> m_occurrencies;
};


class Storage
{
public:
	Storage(){}
	Storage( const Storage& o)
		:dbi(o.dbi),sti(o.sti),sci(o.sci){}
	~Storage(){}

	strus::Reference<strus::DatabaseInterface> dbi;
	strus::Reference<strus::StorageInterface> sti;
	strus::Reference<strus::StorageClientInterface> sci;

	void open( const char* options, bool reset);
	void close()
	{
		sci.reset();
		sti.reset();
		dbi.reset();
	}

	struct MetaDataDef
	{
		const char* key;
		const char* type;
	};
	void defineMetaData( MetaDataDef const* deflist);
	void dump();
};


struct Feature
{
	enum Kind
	{
		SearchIndex,
		ForwardIndex,
		Attribute,
		MetaData
	};
	static const char* kindName( Kind kind)
	{
		static const char* ar[] = {"searchindex","forwardindex","attribute","metadata"};
		return ar[ kind];
	}
	Kind kind;
	std::string type;
	std::string value;
	unsigned int pos;

	Feature( Kind kind_, const std::string& type_, const std::string& value_, unsigned int pos_=0)
		:kind(kind_),type(type_),value(value_),pos(pos_){}
	Feature( const Feature& o)
		:kind(o.kind),type(o.type),value(o.value),pos(o.pos){}
};


struct DocumentBuilder
{
	PrimeFactors primeFactors;
	int nofDocuments;

	explicit DocumentBuilder( int nofDocuments_)
		:primeFactors( nofDocuments_)
		,nofDocuments( nofDocuments_)
	{}

	std::vector<Feature> create( int docno) const
	{
		std::vector<Feature> rt;
		int number = docno + PrimeFactors::MinNumber;
		std::string description;
		std::string title = strus::string_format( "number %d", number);

		const std::vector<int>& fc = primeFactors.factorList( number);
		std::set<int> uc;
		int maxnum = 0;
		int minnum = 0;
		unsigned int ti=0, te=fc.size();
		for (ti=0; ti<te; ++ti)
		{
			uc.insert( fc[ti]);
			if (!minnum || minnum > fc[ti])
			{
				minnum = fc[ti];
			}
			if (maxnum < fc[ti])
			{
				maxnum = fc[ti];
			}
			if (ti) description.push_back( ' ');
			description.append( strus::string_format( "%u", fc[ti]));

			std::string searchtype = "word";
			std::string searchvalue = strus::string_format( "s%u", fc[ti]);
			rt.push_back( Feature( Feature::SearchIndex, searchtype, searchvalue, ti+1));

			std::string forwardtype = "orig";
			std::string forwardvalue = strus::string_format( "f%u", fc[ti]);
			rt.push_back( Feature( Feature::ForwardIndex, forwardtype, forwardvalue, ti+1));
		}
		rt.push_back( Feature( Feature::Attribute, "title", title));
		rt.push_back( Feature( Feature::Attribute, "docid", PrimeFactors::docid( docno)));
		rt.push_back( Feature( Feature::Attribute, "description", description));

		std::string sizestr = strus::string_format( "%u", (unsigned int)fc.size());
		std::string lostr = strus::string_format( "%u", fc.empty() ? 0:minnum);
		std::string histr = strus::string_format( "%u", fc.empty() ? 0:maxnum);
		std::string nnstr = strus::string_format( "%u", (unsigned int)uc.size());

		rt.push_back( Feature( Feature::MetaData, "doclen", sizestr));
		rt.push_back( Feature( Feature::MetaData, "lo", lostr));
		rt.push_back( Feature( Feature::MetaData, "hi", histr));
		rt.push_back( Feature( Feature::MetaData, "nn", nnstr));

		return rt;
	}
};


class ExpectedResultElement
{
public:
	ExpectedResultElement( const std::string& docid_, double weight_)
		:m_docid(docid_),m_weight(weight_){}
	ExpectedResultElement( const ExpectedResultElement& o)
		:m_docid(o.m_docid),m_weight(o.m_weight){}

	double weight() const			{return m_weight;}
	const std::string& docid() const	{return m_docid;}

private:
	std::string m_docid;
	double m_weight;
};

static bool isEqualRsv( double r1, double r2)
{
	double diff = r1 > r2 ? r1 - r2 : r2 - r1;
	return diff <= std::numeric_limits<float>::epsilon();
}

template <class ResElemType>
static std::vector<ResElemType> cutRanklist( const std::vector<ResElemType>& res)
{
	double cbuf[ MaxNofClasses];
	int ci=0, ce=MaxNofClasses;
	typename std::vector<ResElemType>::const_iterator ri = res.begin(), re = res.end();
	if (ri != re)
	{
		cbuf[ci++] = ri->weight();
		++ri;
	}
	while (ri != re)
	{
		if (!isEqualRsv( cbuf[ ci-1], ri->weight()))
		{
			if (ci == ce) break;
			cbuf[ci++] = ri->weight();
		}
		++ri;
	}
	return std::vector<ResElemType>( res.begin(), ri);
}


class QueryEval
{
public:
	struct BM25Param
	{
		double k1;
		double b;
		int64_t avgdoclen;

		BM25Param() :k1(1.5),b(0.75),avgdoclen(3){}
	};

	QueryEval( const strus::Reference<strus::StorageClientInterface> sci_, const char* method_)
		:method(method_)
		,qpi(strus::createQueryProcessor( g_fileLocator, g_errorhnd))
		,qei(strus::createQueryEval( g_errorhnd)),sci(sci_)
	{
		if (!qpi.get()) throw std::runtime_error( g_errorhnd->fetchError());
		if (!qei.get()) throw std::runtime_error( g_errorhnd->fetchError());

		if (0==std::strcmp( method, "bm25"))
		{
			instantiate_bm25();
		}
		else
		{
			throw std::runtime_error("test evaluation method not found");
		}
	}
	~QueryEval(){}

	const char* method;
	strus::Reference<strus::QueryProcessorInterface> qpi;
	strus::Reference<strus::QueryEvalInterface> qei;
	strus::Reference<strus::StorageClientInterface> sci;

	strus::QueryInterface* createQuery( const std::vector<std::string>& features)
	{
		strus::Reference<strus::QueryInterface> qry( qei->createQuery( sci.get()));
		if (!qry.get()) throw std::runtime_error( g_errorhnd->fetchError());

		if (0==std::strcmp( method, "bm25"))
		{
			std::vector<std::string>::const_iterator fi = features.begin(), fe = features.end();
			for (; fi != fe; ++fi)
			{
				qry->pushTerm( "word", *fi, 1);
				qry->defineFeature( "search", 1.0/*weight*/);
			}
		}
		else
		{
			throw std::runtime_error("test evaluation method not found");
		}
		return qry.release();
	}

	static std::vector<ExpectedResultElement>
		calculateExpectedRanklist(
			const DocumentBuilder& documentBuilder,
			const char* method,
			const std::vector<int>& query)
	{
		if (0==std::strcmp(method,"bm25"))
		{
			return getRanklist( documentBuilder,
					calculateWeights_bm25( 
						documentBuilder.primeFactors, query));
		}
		else
		{
			throw std::runtime_error("test evaluation method not found");
		}
	}

private:
	void instantiate_bm25()
	{
		qei->addSelectionFeature( "search");

		const strus::SummarizerFunctionInterface* sumfunci
			= qpi->getSummarizerFunction("attribute");
		if (!sumfunci)
		{
			throw std::runtime_error("summarizer function not found");
		}
		{
			strus::Reference<strus::SummarizerFunctionInstanceInterface>
				summarizer( sumfunci->createInstance( qpi.get()));
			if (!summarizer.get()) throw std::runtime_error(g_errorhnd->fetchError());
			summarizer->addStringParameter( "name", "docid");
			qei->addSummarizerFunction(
				"attribute", summarizer.release(),
				std::vector<strus::QueryEvalInterface::FeatureParameter>());
		}{
			strus::Reference<strus::SummarizerFunctionInstanceInterface>
				summarizer( sumfunci->createInstance( qpi.get()));
			if (!summarizer.get()) throw std::runtime_error(g_errorhnd->fetchError());
			summarizer->addStringParameter( "name", "description");

			qei->addSummarizerFunction(
				"attribute", summarizer.release(),
				std::vector<strus::QueryEvalInterface::FeatureParameter>());
		}
		const strus::WeightingFunctionInterface* weightfunci
			= qpi->getWeightingFunction("bm25");
		if (!weightfunci)
		{
			throw std::runtime_error("weighting function not found");
		}
		{
			BM25Param param;
			strus::Reference<strus::WeightingFunctionInstanceInterface>
				function( weightfunci->createInstance( qpi.get()));
			function->addNumericParameter( "k1", param.k1);
			function->addNumericParameter( "b", param.b);
			function->addNumericParameter( "avgdoclen", param.avgdoclen);

			std::vector<strus::QueryEvalInterface::FeatureParameter> featureParameters;
			featureParameters.push_back( strus::QueryEvalInterface::FeatureParameter( "match", "search"));
			qei->addWeightingFunction( "bm25", function.release(), featureParameters);
		}
	}

	class WeightedResultElement
	{
	public:
		WeightedResultElement()
			:m_weight(0.0),m_docno(-1){}
		WeightedResultElement( int docno_, double weight_)
			:m_weight(weight_),m_docno(docno_){}
		WeightedResultElement( const WeightedResultElement& o)
			:m_weight(o.m_weight),m_docno(o.m_docno){}

		int docno() const	{return m_docno;}
		double weight() const	{return m_weight;}

		bool operator < (const WeightedResultElement& o) const
		{
			return m_weight == o.m_weight ? m_docno < o.m_docno : m_weight < o.m_weight;
		}
		bool operator > (const WeightedResultElement& o) const
		{
			return m_weight == o.m_weight ? m_docno > o.m_docno : m_weight > o.m_weight;
		}

	private:
		double m_weight;
		int m_docno;
	};

	static std::vector<ExpectedResultElement> getRanklist( const DocumentBuilder& documentBuilder, const std::vector<WeightedResultElement>& elements_)
	{
		std::vector<ExpectedResultElement> rt;
		std::vector<WeightedResultElement> elements = cutRanklist( elements_);
		std::vector<WeightedResultElement>::const_iterator ei = elements.begin(), ee = elements.end();
		for (; ei != ee; ++ei)
		{
			rt.push_back( ExpectedResultElement( PrimeFactors::docid( ei->docno()), ei->weight()));
		}
		return rt;
	}

	static std::vector<WeightedResultElement> calculateWeights_bm25( const PrimeFactors& primeFactors, const std::vector<int>& query)
	{
		std::vector<WeightedResultElement> rt;
		int docno = 0, ni = PrimeFactors::MinNumber, ne = primeFactors.maxNumber();
		for (; ni < ne; ++ni,++docno)
		{
			double score = 0.0;
			BM25Param p;
			double N = primeFactors.size();
			double doclen = primeFactors.factorList( ni).size();
			double reldoclen = (doclen+1) / p.avgdoclen;
			std::map<int,int> ffMap = primeFactors.queryFeatToFfMap( ni, query);
			std::map<int,int> qfMap;
			std::vector<int>::const_iterator qi = query.begin(), qe = query.end();
			for (; qi != qe; ++qi)
			{
				if (ffMap.find( *qi) != ffMap.end()) ++qfMap[ *qi];
			}
			if (!ffMap.empty())
			{
				std::map<int,int>::const_iterator fi = ffMap.begin(), fe = ffMap.end();
				for (; fi != fe; ++fi)
				{
					double df = primeFactors.frequency( fi->first);
					double ff = fi->second;
					double qf = qfMap[ fi->first];

					double IDF = N <= 2*df ? 0.0 : log10((N - df + 0.5) / (df + 0.5));
					if (IDF < 0.00001)
					{
						IDF = 0.00001;
					}
					score += (qf * IDF * ff * (p.k1 + 1.0))
						/ (ff + p.k1 * (1.0 - p.b + p.b * reldoclen));
				}
				rt.push_back( WeightedResultElement( docno, score));
			}
		}
		if (rt.size() > MaxNofRanks)
		{
			// ... Get best N
			std::nth_element( rt.begin(), rt.begin() + MaxNofRanks, rt.end(), std::greater<WeightedResultElement>());
			rt.resize( MaxNofRanks);
		}
		std::sort( rt.begin(), rt.end(), std::greater<WeightedResultElement>());
		return rt;
	}
};


void Storage::open( const char* config, bool reset)
{
	dbi.reset( strus::createDatabaseType_leveldb( g_fileLocator, g_errorhnd));
	if (!dbi.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	sti.reset( strus::createStorageType_std( g_fileLocator, g_errorhnd));
	if (!sti.get() || g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	if (reset)
	{
		(void)dbi->destroyDatabase( config);
		(void)g_errorhnd->fetchError();
	
		if (!sti->createStorage( config, dbi.get()))
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
	}
	const strus::StatisticsProcessorInterface* statisticsMessageProc = 0;
	sci.reset( sti->createClient( config, dbi.get(), statisticsMessageProc));
	if (!sci.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
}

void Storage::defineMetaData( MetaDataDef const* deflist)
{
	strus::Reference<strus::StorageTransactionInterface> transaction( sci->createTransaction());
	if (!transaction.get()) throw strus::runtime_error( "failed to create transaction: %s", g_errorhnd->fetchError());
	strus::Reference<strus::StorageMetaDataTableUpdateInterface> update( transaction->createMetaDataTableUpdate());
	if (!update.get()) throw strus::runtime_error( "failed to create structure for declaring meta data: %s", g_errorhnd->fetchError());
	
	int di = 0;
	for (; deflist[di].key; ++di)
	{
		update->addElement( deflist[di].key, deflist[di].type);
	}
	update->done();
	if (!transaction->commit()) throw strus::runtime_error( "failed to commit meta data structure definition: %s", g_errorhnd->fetchError());
}

static void destroyStorage( const char* config)
{
	strus::shared_ptr<strus::DatabaseInterface> dbi;
	dbi.reset( strus::createDatabaseType_leveldb( g_fileLocator, g_errorhnd));
	if (!dbi.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	dbi->destroyDatabase( config);
}

void Storage::dump()
{
	strus::local_ptr<strus::StorageDumpInterface> chunkitr( sci->createDump( ""/*keyprefix*/));

	const char* chunk;
	std::size_t chunksize;
	std::string dumpcontent;
	while (chunkitr->nextChunk( chunk, chunksize))
	{
		dumpcontent.append( chunk, chunksize);
	}
	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	else
	{
		std::cout << dumpcontent << std::endl;
	}
}


static void printDocument( std::ostream& out, const std::string& docid, const std::vector<Feature>& featurelist)
{
	out << strus::string_format( "docid %s\n", docid.c_str());
	std::vector<Feature>::const_iterator fi = featurelist.begin(), fe = featurelist.end();
	for (; fi != fe; ++fi)
	{
		if (fi->pos)
		{
			out << strus::string_format( "\tpos %d: %s %s '%s'\n", fi->pos, Feature::kindName( fi->kind), fi->type.c_str(), fi->value.c_str());
		}
		else
		{
			out << strus::string_format( "\t%s %s '%s'\n", Feature::kindName( fi->kind), fi->type.c_str(), fi->value.c_str());
		}
	}
	out << std::endl; 
}

static void insertDocument( strus::StorageDocumentInterface* doc, const std::vector<Feature>& featurelist)
{
	std::vector<Feature>::const_iterator fi = featurelist.begin(), fe = featurelist.end();
	for (; fi != fe; ++fi)
	{
		switch (fi->kind)
		{
			case Feature::SearchIndex:
				doc->addSearchIndexTerm( fi->type, fi->value, fi->pos);
				break;
			case Feature::ForwardIndex:
				doc->addForwardIndexTerm( fi->type, fi->value, fi->pos);
				break;
			case Feature::Attribute:
				doc->setAttribute( fi->type, fi->value);
				break;
			case Feature::MetaData:
			{
				strus::NumericVariant::UIntType val = atoi(fi->value.c_str());
				doc->setMetaData( fi->type, val);
				break;
			}
		}
	}
	doc->done();
	
}

static void insertCollection( strus::StorageClientInterface* storage, const DocumentBuilder& documentBuilder)
{
	strus::local_ptr<strus::StorageTransactionInterface> transaction( storage->createTransaction());
	unsigned int di=0, de=documentBuilder.nofDocuments;
	for (; di != de; ++di)
	{
		if (di % StorageCommitSize == 0)
		{
			if (g_errorhnd->hasError())
			{
				throw strus::runtime_error( "insert failed: %s", g_errorhnd->fetchError());
			}
			if (!transaction->commit())
			{
				throw strus::runtime_error( "transaction failed: %s", g_errorhnd->fetchError());
			}
		}
		std::string docid = PrimeFactors::docid( di);
		strus::local_ptr<strus::StorageDocumentInterface>
			doc( transaction->createDocument( docid.c_str()));
		if (!doc.get()) throw strus::runtime_error("error creating document to insert");

		std::vector<Feature> feats = documentBuilder.create( di);
		insertDocument( doc.get(), feats);

		if (g_verbose) printDocument( std::cerr, docid, feats);
	}
	if (g_errorhnd->hasError())
	{
		throw strus::runtime_error( "insert failed: %s", g_errorhnd->fetchError());
	}
	if (!transaction->commit())
	{
		throw strus::runtime_error( "transaction failed: %s", g_errorhnd->fetchError());
	}
}

static std::string queryResultToString( const strus::QueryResult& res)
{
	std::ostringstream out;
	out << strus::string_format(
		"result (pass=%d, ranked=%d, visited=%d):\n",
		res.evaluationPass(),res.nofRanked(),res.nofVisited());
	std::vector<strus::ResultDocument> ranks( cutRanklist( res.ranks()));
	std::vector<strus::ResultDocument>::const_iterator
		ri = ranks.begin(), re = ranks.end();
	int ridx = 1;
	for (; ri != re; ++ri,++ridx)
	{
		std::vector<strus::SummaryElement>::const_iterator si = ri->summaryElements().begin(), se = ri->summaryElements().end();
		std::string summarystr;
		for (; si != se; ++si)
		{
			if (!summarystr.empty()) summarystr.append(", ");
			if (si->index() >= 0)
			{
				summarystr.append( strus::string_format("%s[%d]", si->name().c_str(), si->index()));
			}
			else
			{
				summarystr.append( si->name());
			}
			if (si->weight() != 1.0)
			{
				summarystr.append( strus::string_format("='%s' (%.4f)", si->value().c_str(), si->weight()));
			}
			else
			{
				summarystr.append( strus::string_format("='%s'", si->value().c_str()));
			}
		}
		out << strus::string_format( "%d %.5f %s\n", ridx, ri->weight(), summarystr.c_str());
	}
	return out.str();
}

static std::string expectedResultToString( const std::vector<ExpectedResultElement>& res)
{
	std::ostringstream out;
	out << "expected:\n";
	std::vector<ExpectedResultElement>::const_iterator ri = res.begin(), re = res.end();
	int ridx = 1;
	for (; ri != re; ++ri,++ridx)
	{
		out << strus::string_format( "%d %.5f docid=%s\n", ridx, ri->weight(), ri->docid().c_str());
	}
	return out.str();
}

static int randomNofQueryTerms()
{
	return g_random.get( 1, g_random.get( 1, g_random.get( 1, MaxNofQueryTerms+1) + 1) + 1);
}

std::string getResultDocumentDocid( const strus::ResultDocument& res)
{
	std::vector<strus::SummaryElement>::const_iterator si = res.summaryElements().begin(), se = res.summaryElements().end();
	for (; si != se; ++si)
	{
		if (si->name() == "docid") return si->value();
	}
	return std::string();
}

bool compareRanklist( const std::vector<strus::ResultDocument>& result, const std::vector<ExpectedResultElement>& expected)
{
	if (result.size() != expected.size()) return false;

	std::vector<ExpectedResultElement>::const_iterator
		ei = expected.begin(), ee = expected.end();
	std::vector<strus::ResultDocument>::const_iterator
		ri = result.begin(), re = result.end();
	for (; ri != re && ei != ee; ++ri,++ei)
	{
		if (!isEqualRsv( ri->weight(), ei->weight())) return false;
		if (getResultDocumentDocid( *ri) != ei->docid())
		{
			double weight = ri->weight();
			std::set<std::string> e_docids, r_docids;
			for (; ri != re && ei != ee
				&& isEqualRsv( ei->weight(), weight)
				&& isEqualRsv( ri->weight(), weight)
				; ++ri,++ei)
			{
				e_docids.insert( ei->docid());
				r_docids.insert( getResultDocumentDocid( *ri));
			}
			--ri; --ei;
			if (e_docids != r_docids) return false;
		}
	}
	return true;
}


static void testQueryEvaluation( int nofNumbers, const char* method, int nofQueries)
{
	Storage storage;
	storage.open( "path=storage", true);
	const Storage::MetaDataDef metadata[] = {{"doclen", "UINT8"},{"lo", "UINT16"},{"hi", "UINT16"},{"nn", "UINT8"},{0,0}};
	storage.defineMetaData( metadata);

	DocumentBuilder documentBuilder( nofNumbers);
	if (g_verbose)
	{
		if (nofNumbers)
		{
			std::cerr << strus::string_format( "create collection for numbers from %d to %d with their prime factors as features", PrimeFactors::MinNumber, nofNumbers+PrimeFactors::MinNumber-1) << std::endl;
		}
		int ni = 0, ne = nofNumbers;
		for (; ni != ne; ++ni)
		{
			int df = documentBuilder.primeFactors.frequency( ni+PrimeFactors::MinNumber);
			if (df) std::cerr << strus::string_format( "df %d = %d", ni+PrimeFactors::MinNumber, df) << std::endl;
		}
	}
	if (g_verbose) std::cerr << "* inserting documents of generated collection" << std::endl;
	insertCollection( storage.sci.get(), documentBuilder);

	QueryEval queryEval( storage.sci, method);
	if (g_verbose) std::cerr << "* evaluate queries" << std::endl;

	int qi=0, qe=nofQueries;
	for (; qi != qe; ++qi)
	{
		std::vector<int> featureidxar;
		std::vector<std::string> features;
		std::string featurestr;
		int ni = 0, ne = randomNofQueryTerms();
		for (; ni < ne; ++ni)
		{
			int prime = documentBuilder.primeFactors.randomPrime();
			featureidxar.push_back( prime);

			features.push_back( strus::string_format( "s%d", prime));

			if (!featurestr.empty()) featurestr.push_back(',');
			featurestr.append( features.back());
		}
		strus::Reference<strus::QueryInterface> qry( queryEval.createQuery( features));
		strus::QueryResult res = qry->evaluate( MinRank, MaxNofRanks);

		std::vector<ExpectedResultElement>
			expected = QueryEval::calculateExpectedRanklist(
				documentBuilder, method, featureidxar);

		bool cmpres = compareRanklist( cutRanklist( res.ranks()), expected);
		if (g_verbose || !cmpres)
		{
			std::string resstr = queryResultToString( res);
			std::string expstr = expectedResultToString( expected);
			std::cerr
			<< strus::string_format( "query '%s' evaluation result %d:", featurestr.c_str(), qi)
			<< std::endl << resstr << std::endl << expstr << std::endl;
		}
		if (!cmpres) throw std::runtime_error( "result not as expected");
	}
}

static void printUsage()
{
	std::cerr << "usage: testQueryEval [options] <nofdocs> <nofqueries> <method>" << std::endl;
	std::cerr << "options:" << std::endl;
	std::cerr << "  -h      :print usage" << std::endl;
	std::cerr << "  -V      :verbose output" << std::endl;
	std::cerr << "  -K      :keep artefacts, do not clean up" << std::endl;
	std::cerr << "<nofdocs>    :number of documents in the test collection" << std::endl;
	std::cerr << "<nofqueries> :number of random test queries (length 1..10) to perform" << std::endl;
	std::cerr << "<method>     :name of test" << std::endl;
}

int main( int argc, const char* argv[])
{
	int rt = 0;
	bool do_cleanup = true;
	int argi = 1;
	for (; argc > (int)argi; ++argi)
	{
		if (std::strcmp( argv[argi], "-K") == 0)
		{
			do_cleanup = false;
		}
		else if (std::strcmp( argv[argi], "-V") == 0)
		{
			g_verbose = true;
		}
		else if (std::strcmp( argv[argi], "-h") == 0)
		{
			printUsage();
			return 0;
		}
		else if (std::strcmp( argv[argi], "--") == 0)
		{
			++argi;
			break;
		}
		else if (argv[argi][0] == '-')
		{
			std::cerr << "unknown option " << argv[argi] << std::endl;
			return -1;
		}
		else
		{
			break;
		}
	}
	if (argc == 1)
	{
		printUsage();
		return 0;
	}
	g_errorhnd = strus::createErrorBuffer_standard( stderr, 1, NULL/*debug trace interface*/);
	if (!g_errorhnd) {std::cerr << "FAILED " << "strus::createErrorBuffer_standard" << std::endl; return -1;}
	g_fileLocator = strus::createFileLocator_std( g_errorhnd);
	if (!g_fileLocator) {std::cerr << "FAILED " << "strus::createFileLocator_std" << std::endl; return -1;}

	try
	{
		if (argi + 3 > argc) throw std::runtime_error( "too few arguments");
		if (argi + 3 < argc) throw std::runtime_error( "too many arguments");

		int nofNumbers = strus::numstring_conv::toint( argv[ argi+0], std::strlen(argv[ argi+0]), std::numeric_limits<int>::max());
		int nofQueries = strus::numstring_conv::toint( argv[ argi+1], std::strlen(argv[ argi+1]), std::numeric_limits<int>::max());
		const char* method = argv[ argi+2];

		testQueryEvaluation( nofNumbers, method, nofQueries);
		std::cerr << "OK" << std::endl;
	}
	catch (const std::bad_alloc&)
	{
		std::cerr << "out of memory" << std::endl;
		rt = -1;
		goto TESTS_DONE;
	}
	catch (const std::runtime_error& err)
	{
		const char* errmsg = g_errorhnd->fetchError();
		if (errmsg)
		{
			std::cerr << "ERROR " << err.what() << ", " << errmsg << std::endl;
		}
		else
		{
			std::cerr << "ERROR " << err.what() << std::endl;
		}
		rt = -1;
		goto TESTS_DONE;
	}

TESTS_DONE:
	if (do_cleanup)
	{
		destroyStorage( "path=storage");
	}
	delete g_fileLocator;
	delete g_errorhnd;
	return rt;
}


