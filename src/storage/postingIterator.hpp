/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "posinfoIterator.hpp"
#include "indexSetIterator.hpp"

namespace strus {
/// \brief Forward declaration
class MetaDataReader;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

class PostingIterator
	:public PostingIteratorInterface
{
public:
	PostingIterator( 
			const StorageClient* storage_,
			const DatabaseClientInterface* database,
			const Index& termtypeno,
			const Index& termvalueno,
			const char* termstr,
			ErrorBufferInterface* errorhnd_);

	virtual ~PostingIterator(){}

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index skipDoc( const Index& docno_);
	virtual Index skipDocCandidate( const Index& docno_);
	virtual Index skipPos( const Index& firstpos_);

	virtual unsigned int frequency();

	virtual Index documentFrequency() const;

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posinfoIterator.posno();
	}

private:
	IndexSetIterator m_docnoIterator;
	PosinfoIterator m_posinfoIterator;

	Index m_docno;
	std::string m_featureid;
	ErrorBufferInterface* m_errorhnd;	///< buffer for error reporting
};

}
#endif

