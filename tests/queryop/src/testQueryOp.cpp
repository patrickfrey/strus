/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/reference.hpp"
#include "strus/lib/error.hpp"
#include "strus/lib/queryproc.hpp"
#include "strus/lib/filelocator.hpp"
#include "strus/base/fileio.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/storage/index.hpp"
#include "private/errorUtils.hpp"
#include <string>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <memory>

#undef STRUS_LOWLEVEL_DEBUG
static strus::ErrorBufferInterface* g_errorhnd = 0;

class ErathosthenesSievePostingIterator
	:public strus::PostingIteratorInterface
{
public:
	explicit ErathosthenesSievePostingIterator( unsigned int divisor_, strus::Index maxdocno_, strus::Index maxposno_)
		:m_divisor(divisor_),m_maxdocno(maxdocno_),m_docno(0),m_maxposno(maxposno_),m_posno(0)
	{
		snprintf( m_featureid, sizeof(m_featureid), "E%u", m_divisor);
	}

	virtual ~ErathosthenesSievePostingIterator(){}

	virtual strus::Index skipDoc( const strus::Index& docno_)
	{
		if (!m_divisor) return m_docno=0;
		unsigned int lo = (unsigned int)docno_ >= (m_divisor*2) ? docno_ : (m_divisor*2);
		strus::Index rt = ((lo-1) / m_divisor) * m_divisor + m_divisor;
		rt = (rt > m_maxdocno)?0:rt;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "CHILD " << m_featureid << " SKIP DOC " << rt << std::endl;
#endif
		return m_docno = rt;
	}

	virtual strus::Index skipDocCandidate( const strus::Index& docno_)
	{
		if (!m_divisor) return m_docno=0;
		unsigned int lo = (unsigned int)docno_ >= (m_divisor*2) ? docno_ : (m_divisor*2);
		strus::Index rt = ((lo-1) / m_divisor) * m_divisor + m_divisor;
		rt = (rt > m_maxdocno)?0:rt;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "CHILD " << m_featureid << " SKIP DOC CANDIDATE " << rt << std::endl;
#endif
		return m_docno = rt;
	}

	virtual strus::Index skipPos( const strus::Index& firstpos)
	{
		if (!m_divisor) return m_posno=0;
		unsigned int lo = (unsigned int)firstpos >= (m_divisor*2) ? firstpos : (m_divisor*2);
		strus::Index rt = ((lo-1) / m_divisor) * m_divisor + m_divisor;
		rt = (rt > m_maxposno)?0:rt;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "CHILD " << m_featureid << " SKIP POS " << rt << std::endl;
#endif
		return m_posno = rt;
	}

	virtual const char* featureid() const
	{
		return m_featureid;
	}

	virtual strus::GlobalCounter documentFrequency() const
	{
		return (m_maxdocno / m_divisor);
	}

	virtual int frequency()
	{
		return (m_maxposno / m_divisor);
	}

	virtual strus::Index docno() const
	{
		return m_docno;
	}

	virtual strus::Index posno() const
	{
		return m_posno;
	}

	virtual strus::Index length() const
	{
		return m_posno ? 1:0;
	}

private:
	unsigned int m_divisor;
	char m_featureid[ 32];
	strus::Index m_maxdocno;
	strus::Index m_docno;
	strus::Index m_maxposno;
	strus::Index m_posno;
};


static bool isPrime( strus::Index val)
{
	if (val <= 1) return false;
	strus::Index half = val >> 2;
	while (half*half > val)
	{
		half >>= 1;
	}
	strus::Index mi=2,me = (half + 1) << 1;

	for (; mi < me; ++mi)
	{
		if ((val / mi) * mi == val) return false;
	}
	return true;
}

static strus::Index nextNonPrimeNumber( strus::Index val)
{
	if (val <= 4) return 4;
	for (;isPrime(val);++val){}
	return val;
}

static void testUnionJoinErathosthenes( const strus::QueryProcessorInterface* qpi)
{
	const strus::PostingJoinOperatorInterface* join = qpi->getPostingJoinOperator( "union");
	enum {NofArguments=61};
	typedef strus::Reference<strus::PostingIteratorInterface> PostingIteratorReference;
	std::vector<PostingIteratorReference> args;
	strus::Index maxno = ((NofArguments+1)*(NofArguments+1));

	unsigned int aidx = 1;
	args.push_back( new ErathosthenesSievePostingIterator( 0, maxno, maxno));
	args.push_back( new ErathosthenesSievePostingIterator( 0, maxno, maxno));
	for (; aidx<=NofArguments; ++aidx)
	{
		args.push_back( new ErathosthenesSievePostingIterator( aidx+1, maxno, maxno));
	}
	strus::Reference<strus::PostingIteratorInterface> result( join->createResultIterator( args, 0/*range*/, 0/*cardinality*/));

	strus::Index curr_docno = 0;
	strus::Index next_docno = 0;
	do
	{
		next_docno = result->skipDoc( curr_docno);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "SKIP DOC " << curr_docno << " = " << next_docno << std::endl;
#endif
		if (curr_docno)
		{
			strus::Index dn = curr_docno;
			for (; dn < next_docno; ++dn)
			{
				std::cout << "skipped (prime number): " << dn << std::endl;
			}
		}
		{
			strus::Index next_result = nextNonPrimeNumber( curr_docno);
			if (next_result > maxno)
			{
				if (next_docno != 0)
				{
					throw strus::runtime_error("unexpected document number in join: found %u != expected %u", next_docno, 0);
				}
			}
			else
			{
				if (next_docno != next_result)
				{
					throw strus::runtime_error("unexpected document number in join: found %u != expected %u", next_docno, next_result);
				}
			}
			curr_docno = next_docno?(next_docno+1):0;
		}
		strus::Index curr_posno = 0;
		strus::Index next_posno = 0;
		do
		{
			next_posno = result->skipPos( curr_posno);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "SKIP POS " << curr_posno << " = " << next_posno << std::endl;
#endif
			strus::Index next_result = nextNonPrimeNumber( curr_posno);
			if (next_result > maxno)
			{
				if (next_posno != 0)
				{
					throw strus::runtime_error("unexpected position number in join: found %u != expected %u", next_posno, 0);
				}
			}
			else
			{
				if (next_posno && next_posno < next_result)
				{
					throw strus::runtime_error("unexpected position number in join: found %u != expected %u", next_posno, next_result);
				}
			}
			curr_posno = next_posno?(next_posno+1):0;
		}
		while (curr_posno != 0);
	}
	while (curr_docno != 0);
}

struct JoinOpResult
{
	unsigned int docno;
	unsigned int posno[64];
};

static JoinOpResult testResult_IntersectWithCardinality[] =
{
	{ 6, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 10, {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 0 }},
	{ 12, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 15, {15, 30, 45, 60, 75, 90, 0 }},
	{ 18, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 20, {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 0 }},
	{ 24, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 30, {6, 10, 12, 15, 18, 20, 24, 30, 30, 36, 40, 42, 45, 48, 50, 54, 60, 60, 66, 70, 72, 75, 78, 80, 84, 90, 90, 96, 100, 0 }},
	{ 36, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 40, {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 0 }},
	{ 42, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 45, {15, 30, 45, 60, 75, 90, 0 }},
	{ 48, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 50, {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 0 }},
	{ 54, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 60, {6, 10, 12, 15, 18, 20, 24, 30, 30, 36, 40, 42, 45, 48, 50, 54, 60, 60, 66, 70, 72, 75, 78, 80, 84, 90, 90, 96, 100, 0 }},
	{ 66, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 70, {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 0 }},
	{ 72, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 75, {15, 30, 45, 60, 75, 90, 0 }},
	{ 78, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 80, {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 0 }},
	{ 84, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 90, {6, 10, 12, 15, 18, 20, 24, 30, 30, 36, 40, 42, 45, 48, 50, 54, 60, 60, 66, 70, 72, 75, 78, 80, 84, 90, 90, 96, 100, 0 }},
	{ 96, {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 0 }},
	{ 100, {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 0 }},
	{ 0, {0}}
};

static void testIntersectWithCardinality( const strus::QueryProcessorInterface* qpi)
{
	const strus::PostingJoinOperatorInterface* join = qpi->getPostingJoinOperator( "intersect");
	enum {NofArguments=3,Cardinality=2};
	typedef strus::Reference<strus::PostingIteratorInterface> PostingIteratorReference;
	std::vector<PostingIteratorReference> args;
	strus::Index maxno = 100;

	args.push_back( new ErathosthenesSievePostingIterator( 2, maxno, maxno));
	args.push_back( new ErathosthenesSievePostingIterator( 3, maxno, maxno));
	args.push_back( new ErathosthenesSievePostingIterator( 5, maxno, maxno));
	strus::Reference<strus::PostingIteratorInterface> result( join->createResultIterator( args, 0/*range*/, 2/*cardinality*/));

	strus::Index curr_docno = 0;
	strus::Index next_docno = 0;
	unsigned int tidx = 0;
	do
	{
		next_docno = result->skipDoc( curr_docno);
		strus::Index expected_docno = testResult_IntersectWithCardinality[ tidx++].docno;
		if (next_docno != expected_docno)
		{
			std::ostringstream msg;
			msg << "expected document '" << expected_docno
				<< "' instead of '" << next_docno << "' in test intersect with cardinality";
			throw std::runtime_error( msg.str());
		}
		if (!next_docno) break;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "SKIP DOC " << curr_docno << " = " << next_docno << std::endl;
#endif
		strus::Index curr_posno = 0;
		strus::Index next_posno = 0;
		unsigned int pidx = 0;
		do
		{
			next_posno = result->skipPos( curr_posno);
			strus::Index expected_posno = testResult_IntersectWithCardinality[ tidx-1].posno[ pidx++];
			if (next_posno != expected_posno)
			{
				std::ostringstream msg;
				msg << "expected position '" << expected_posno
					<< "' instead of '" << next_posno << "' in test intersect with cardinality, document " << expected_docno;
				throw std::runtime_error( msg.str());
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "SKIP POS " << curr_posno << " = " << next_posno << std::endl;
#endif
			curr_posno = next_posno?(next_posno+1):0;
		}
		while (curr_posno != 0);
		if (testResult_IntersectWithCardinality[ tidx-1].posno[ pidx] != 0)
		{
			throw std::runtime_error( "test output intersect with cardinality is not as expected");
		}
		curr_docno = next_docno?(next_docno+1):0;
	}
	while (curr_docno != 0);
	if (testResult_IntersectWithCardinality[ tidx-1].docno != 0)
	{
		throw std::runtime_error( "test output intersect with cardinality is not as expected");
	}
}

#define RUN_TEST( idx, TestName, qpi)\
	try\
	{\
		test ## TestName( qpi);\
		std::cerr << "Executing test (" << idx << ") " << #TestName << " [OK]" << std::endl;\
	}\
	catch (const std::runtime_error& err)\
	{\
		std::cerr << "Error in test (" << idx << ") " << #TestName << ": " << err.what() << std::endl;\
		return -1;\
	}\
	catch (const std::bad_alloc& err)\
	{\
		std::cerr << "Out of memory in test (" << idx << ") " << #TestName << std::endl;\
		return -1;\
	}\


int main( int argc, const char* argv[])
{
	try
	{
		unsigned int ii = 1;
		unsigned int test_index = 0;
		for (; argc > (int)ii; ++ii)
		{
			if (std::strcmp( argv[ii], "-T") == 0)
			{
				++ii;
				if (argc == (int)ii)
				{
					std::cerr << "option -T expects an argument" << std::endl;
					return -1;
				}
				test_index = atoi( argv[ ii]);
			}
			else if (std::strcmp( argv[ii], "-h") == 0)
			{
				std::cerr << "usage: testQueryOp [options]" << std::endl;
				std::cerr << "options:" << std::endl;
				std::cerr << "  -h      :print usage" << std::endl;
				std::cerr << "  -T <i>  :execute only test with index <i>" << std::endl;
			}
			else if (argv[ii][0] == '-')
			{
				std::cerr << "unknown option " << argv[ii] << std::endl;
				return -1;
			}
			else
			{
				std::cerr << "unexpected argument" << std::endl;
				return -1;
			}
		}
		g_errorhnd = strus::createErrorBuffer_standard( stderr, 1, NULL/*debug trace interface*/);
		if (!g_errorhnd) return -1;
		strus::Reference<strus::FileLocatorInterface> filelocator( strus::createFileLocator_std( g_errorhnd));
		if (!filelocator.get()) throw std::runtime_error("error creating file locator");

		strus::Reference<strus::QueryProcessorInterface> qpi = strus::createQueryProcessor( filelocator.get(), g_errorhnd);
		if (!qpi.get())
		{
			throw strus::runtime_error("failed to create query processor instance");
		}
		if (g_errorhnd->hasError())
		{
			throw strus::runtime_error("error initializing strus objects");
		}
		unsigned int ti=test_index?test_index:1;
		for (;;++ti)
		{
			switch (ti)
			{
				case 1: RUN_TEST( ti, UnionJoinErathosthenes, qpi.get() ) break;
				case 2: RUN_TEST( ti, IntersectWithCardinality, qpi.get() ) break;
				default: return 0;
			}
			if (test_index) break;
		}
	}
	catch (const std::runtime_error& err)
	{
		const char* errmsg = g_errorhnd->fetchError();
		std::cerr << "Error: " << err.what() << (errmsg?": ":0) << (errmsg?errmsg:"") << std::endl;
		return -1;
	}
	catch (const std::bad_alloc& err)
	{
		std::cerr << "Out of memory" << std::endl;
		return -1;
	}
}


