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
#include "forwardIterator.hpp"
#include "storage.hpp"
#include "indexPacker.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"

using namespace strus;

ForwardIterator::ForwardIterator( const Storage* storage_, DatabaseInterface* database_, const std::string& type_)
	:m_database(database_)
	,m_forwardBlockStorage(0)
	,m_curblock(0)
	,m_blockitr(0)
	,m_docno(0)
	,m_typeno(storage_->getTermType( type_))
	,m_curpos(0)
{
	if (m_typeno == 0) throw std::runtime_error( std::string("unknown term type name '") + type_ + "'");
}

ForwardIterator::~ForwardIterator()
{
	if (m_forwardBlockStorage) delete m_forwardBlockStorage;
}

void ForwardIterator::skipDoc( const Index& docno_)
{
	if (m_docno != docno_ || !m_forwardBlockStorage)
	{
		if (m_forwardBlockStorage) delete m_forwardBlockStorage;
		m_forwardBlockStorage = 0;
		m_forwardBlockStorage
			= new BlockStorage<ForwardIndexBlock>( 
				m_database, DatabaseKey::ForwardIndexPrefix,
				BlockKey( m_typeno, docno_), false);
		m_docno = docno_;
		m_curblock = 0;
		m_curblock_lastpos = 0;
		m_curblock_firstpos = 0;
		m_blockitr = 0;
		m_curpos = 0;
	}
}

Index ForwardIterator::skipPos( const Index& firstpos_)
{
	if (!m_forwardBlockStorage || !m_docno) return 0;
	if (!m_curblock || firstpos_ < m_curblock_firstpos || firstpos_ > m_curblock_lastpos)
	{
		m_curblock = m_forwardBlockStorage->load( firstpos_);
		if (!m_curblock)
		{
			m_curblock_lastpos = 0;
			m_curblock_firstpos = 0;
			m_blockitr = 0;
			m_curpos = 0;
			return 0;
		}
		else
		{
			m_curblock_lastpos = m_curblock->id();
			m_curblock_firstpos = m_curblock->position_at( m_curblock->charptr());
			m_blockitr = m_curblock->charptr();
			m_curpos = m_curblock_firstpos;
			if (m_curpos >= firstpos_)
			{
				return m_curpos;
			}
		}
	}
	else if (m_curpos > firstpos_ || m_curpos == 0 || !m_blockitr)
	{
		m_curpos = m_curblock_firstpos;
		m_blockitr = m_curblock->charptr();
	}
	if (0!=(m_blockitr = m_curblock->upper_bound( firstpos_, m_blockitr)))
	{
		m_curpos = m_curblock->position_at( m_blockitr);
	}
	else
	{
		m_curpos = 0;
	}
	return m_curpos;
}

std::string ForwardIterator::fetch()
{
	if (!m_blockitr || !m_curblock)
	{
		throw std::runtime_error("internal: forward iterator fetch called without a term selected");
	}
	return std::string( m_curblock->value_at( m_blockitr));
}


