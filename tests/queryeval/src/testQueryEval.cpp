/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "primeFactorCollection.hpp"
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

typedef strus::test::PrimeFactorCollection PrimeFactorCollection;
typedef strus::test::PrimeFactorDocumentBuilder PrimeFactorDocumentBuilder;
typedef strus::test::Storage Storage;
typedef strus::test::Feature Feature;

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
			const PrimeFactorDocumentBuilder& documentBuilder,
			const char* method,
			const std::vector<int>& query)
	{
		if (0==std::strcmp(method,"bm25"))
		{
			return getRanklist( documentBuilder,
					calculateWeights_bm25( 
						documentBuilder.primeFactorCollection, query));
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
				"docid", summarizer.release(),
				std::vector<strus::QueryEvalInterface::FeatureParameter>());
		}{
			strus::Reference<strus::SummarizerFunctionInstanceInterface>
				summarizer( sumfunci->createInstance( qpi.get()));
			if (!summarizer.get()) throw std::runtime_error(g_errorhnd->fetchError());
			summarizer->addStringParameter( "name", "description");

			qei->addSummarizerFunction(
				"description", summarizer.release(),
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
			qei->addWeightingFunction( function.release(), featureParameters);
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

	static std::vector<ExpectedResultElement> getRanklist( const PrimeFactorDocumentBuilder& documentBuilder, const std::vector<WeightedResultElement>& elements_)
	{
		std::vector<ExpectedResultElement> rt;
		std::vector<WeightedResultElement> elements = cutRanklist( elements_);
		std::vector<WeightedResultElement>::const_iterator ei = elements.begin(), ee = elements.end();
		for (; ei != ee; ++ei)
		{
			rt.push_back( ExpectedResultElement( PrimeFactorCollection::docid( ei->docno()), ei->weight()));
		}
		return rt;
	}

	static std::vector<WeightedResultElement> calculateWeights_bm25( const PrimeFactorCollection& primeFactorCollection, const std::vector<int>& query)
	{
		std::vector<WeightedResultElement> rt;
		int docno = 0, ni = PrimeFactorCollection::MinNumber, ne = primeFactorCollection.maxNumber();
		for (; ni < ne; ++ni,++docno)
		{
			double score = 0.0;
			BM25Param p;
			double N = primeFactorCollection.size();
			double doclen = primeFactorCollection.factorList( ni).size();
			double reldoclen = (doclen+1) / p.avgdoclen;
			std::map<int,int> ffMap = primeFactorCollection.queryFeatToFfMap( ni, query);
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
					double df = primeFactorCollection.frequency( fi->first);
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
	Storage storage( g_fileLocator, g_errorhnd);
	storage.open( "path=storage", true);
	storage.defineMetaData( PrimeFactorDocumentBuilder::metadata());

	PrimeFactorDocumentBuilder documentBuilder( nofNumbers, g_verbose, g_errorhnd);
	if (g_verbose)
	{
		if (nofNumbers)
		{
			std::cerr << strus::string_format( "create collection for numbers from %d to %d with their prime factors as features", PrimeFactorCollection::MinNumber, nofNumbers+PrimeFactorCollection::MinNumber-1) << std::endl;
		}
		int ni = 0, ne = nofNumbers;
		for (; ni != ne; ++ni)
		{
			int df = documentBuilder.primeFactorCollection.frequency( ni+PrimeFactorCollection::MinNumber);
			if (df) std::cerr << strus::string_format( "df %d = %d", ni+PrimeFactorCollection::MinNumber, df) << std::endl;
		}
	}
	if (g_verbose) std::cerr << "* inserting documents of generated collection" << std::endl;
	documentBuilder.insertCollection( storage.sci.get(), g_random, StorageCommitSize, PrimeFactorDocumentBuilder::InsertMode, true);

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
			int prime = documentBuilder.primeFactorCollection.randomPrime( g_random);
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
		Storage::destroy( "path=storage", g_fileLocator, g_errorhnd);
	}
	delete g_fileLocator;
	delete g_errorhnd;
	return rt;
}


