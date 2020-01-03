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
#include "strus/index.hpp"
#include "structBlock.hpp"
#include "documentBlockIteratorTemplate.hpp"
#include "databaseAdapter.hpp"

namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StorageClient;

class StructIterator
	:public StructIteratorInterface
{
public:
	StructIterator( const StorageClient* storage_, const DatabaseClientInterface* database_, ErrorBufferInterface* errorhnd_);
	virtual ~StructIterator();

	virtual void skipDoc( const Index& docno_);

	virtual int levels() const;

	virtual Index docno() const;

	virtual IndexRange skipPos( int level, const Index& firstpos);

	virtual IndexRange field( int level) const;

	virtual StructureLinkArray links( int level) const;

private:
	const StorageClient* m_storage;					///< storage instance
	const DatabaseClientInterface* m_database;			///< database handle to create cursor
	Reference<DatabaseAdapter_StructBlock::Reader> m_dbadapter;	///< database cursor
	StructBlock m_curblock;						///< structure block of the current document
	StructBlock::FieldScanner m_scanner[ StructBlock::MaxFieldLevels];///< scanner of structure instances for every level
	Index m_docno;							///< current document number
	int m_levels;							///< number of structure levels in current document
	ErrorBufferInterface* m_errorhnd;				///< error buffer for exception free interface
};

}//namespace
#endif


