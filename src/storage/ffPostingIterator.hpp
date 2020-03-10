/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_FF_POSTING_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_FF_POSTING_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"
#include "strus/storage/termStatistics.hpp"
#include "strus/reference.hpp"
#include "ffIterator.hpp"
#include "indexSetIterator.hpp"

namespace strus {
/// \brief Forward declaration
class MetaDataReader;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

class FfPostingIterator
	:public PostingIteratorInterface
{
public:
	FfPostingIterator( 
			const StorageClient* storage_,
			const DatabaseClientInterface* database,
			const Index& termtypeno,
			const Index& termvalueno,
			const char* termstr,
			const TermStatistics& stats_,
			ErrorBufferInterface* errorhnd_);

	virtual ~FfPostingIterator(){}

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index skipDoc( const Index& docno_);
	virtual Index skipDocCandidate( const Index& docno_);
	virtual Index skipPos( const Index& firstpos_);

	virtual int frequency();

	virtual GlobalCounter documentFrequency() const;

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_ffIterator.posno();
	}

	virtual Index length() const
	{
		return 1;
	}

private:
	Index skipDoc_impl( const Index& docno_);

private:
	IndexSetIterator m_docnoIterator;
	FfIterator m_ffIterator;

	Index m_docno;
	Index m_termtypeno;
	Index m_termvalueno;
	std::string m_featureid;
	ErrorBufferInterface* m_errorhnd;	///< buffer for error reporting
};


class FfNoIndexSetPostingIterator
	:public PostingIteratorInterface
{
public:
	FfNoIndexSetPostingIterator( 
			const StorageClient* storage_,
			const DatabaseClientInterface* database,
			const Index& termtypeno,
			const Index& termvalueno,
			const char* termstr,
			const TermStatistics& stats_,
			ErrorBufferInterface* errorhnd_);

	virtual ~FfNoIndexSetPostingIterator(){}

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index skipDoc( const Index& docno_);
	virtual Index skipDocCandidate( const Index& docno_);
	virtual Index skipPos( const Index& firstpos_);

	virtual int frequency();

	virtual GlobalCounter documentFrequency() const;

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_ffIterator.posno();
	}

	virtual Index length() const
	{
		return 1;
	}

private:
	Index skipDoc_impl( const Index& docno_);

private:
	FfIterator m_ffIterator;
	Index m_docno;
	Index m_termtypeno;
	Index m_termvalueno;
	std::string m_featureid;
	ErrorBufferInterface* m_errorhnd;	///< buffer for error reporting
};


}//namespace
#endif

