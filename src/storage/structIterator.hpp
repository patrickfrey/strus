/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STRUCTURE_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/structIteratorInterface.hpp"
#include "structBlock.hpp"
#include "documentBlockIteratorTemplate.hpp"
#include "databaseAdapter.hpp"
#include "private/errorUtils.hpp"

namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StorageClient;

class StructIteratorImpl
	:public DocumentBlockIteratorTemplate<DatabaseAdapter_StructBlock::Cursor,StructBlock>
{
public:
	StructIteratorImpl( const StorageClient* storage_, const DatabaseClientInterface* database_, Index structno_)
		:DocumentBlockIteratorTemplate<DatabaseAdapter_StructBlock::Cursor,StructBlock>( DatabaseAdapter_StructBlock::Cursor(database_,structno_))
		,m_storage(storage_)
		,m_structureScanner()
		,m_memberScanner()
		,m_structno(structno_){}
	~StructIteratorImpl(){}

	Index skipDoc( const Index& docno_)
	{
		if (docno() && docno_ == docno()) return docno_;
		m_structureScanner.clear();
		m_memberScanner.clear();
		return DocumentBlockIteratorTemplate<DatabaseAdapter_StructBlock::Cursor,StructBlock>::skipDoc( docno_);
	}

	IndexRange skipPosSource( const Index& firstpos);
	IndexRange skipPosSink( const Index& firstpos);

	IndexRange source() const
	{
		return m_structureScanner.initialized()
			? IndexRange( m_structureScanner.current()->header_start, m_structureScanner.current()->header_end)
			: IndexRange(0,0);
	}
	IndexRange sink() const
	{
		return m_memberScanner.initialized()
			? IndexRange( m_memberScanner.current()->start, m_memberScanner.current()->end)
			: IndexRange(0,0);
	}

private:
	const StorageClient* m_storage;
	StructBlock::StructureScanner m_structureScanner;
	StructBlock::MemberScanner m_memberScanner;
	Index m_structno;
};


class StructIterator
	:public StructIteratorInterface
{
public:
	StructIterator( const StorageClient* storage_, const DatabaseClientInterface* database_, Index structno_, ErrorBufferInterface* errorhnd_)
		:m_impl(storage_,database_,structno_),m_errorhnd(errorhnd_){}
	virtual ~StructIterator(){}

	virtual Index skipDoc( const Index& docno);
	virtual IndexRange skipPosSource( const Index& firstpos);
	virtual IndexRange skipPosSink( const Index& firstpos);
	virtual IndexRange source() const;
	virtual IndexRange sink() const;

private:
	StructIteratorImpl m_impl;
	ErrorBufferInterface* m_errorhnd;
};


class NullStructIterator
	:public StructIteratorInterface
{
public:
	NullStructIterator(){}
	virtual ~NullStructIterator(){}

	virtual Index skipDoc( const Index& docno)
	{
		return 0;
	}
	virtual IndexRange skipPosSource( const Index& firstpos)
	{
		return IndexRange(0,0);
	}
	virtual IndexRange skipPosSink( const Index& firstpos)
	{
		return IndexRange(0,0);
	}
	virtual IndexRange source() const
	{
		return IndexRange(0,0);
	}
	virtual IndexRange sink() const
	{
		return IndexRange(0,0);
	}
};

}//namespace
#endif


