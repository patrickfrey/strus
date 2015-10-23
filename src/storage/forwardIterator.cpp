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
#include "strus/databaseClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

ForwardIterator::ForwardIterator( const StorageClient* storage_, const DatabaseClientInterface* database_, const std::string& type_, ErrorBufferInterface* errorhnd_)
	:m_database(database_)
	,m_dbadapter()
	,m_blockitr(0)
	,m_docno(0)
	,m_typeno(storage_->getTermType( type_))
	,m_curpos(0)
	,m_errorhnd(errorhnd_)
{
	if (m_typeno == 0) throw strus::runtime_error( _TXT( "unknown term type name '%s'"), type_.c_str());
}

ForwardIterator::~ForwardIterator()
{}

void ForwardIterator::skipDoc( const Index& docno_)
{
	try
	{
		if ((m_docno != docno_ || !m_docno) || !m_dbadapter.get())
		{
			m_dbadapter.reset(
				new DatabaseAdapter_ForwardIndex::Cursor(
					m_database, m_typeno, docno_));
			m_docno = docno_;
			m_curblock.clear();
			m_curblock_lastpos = 0;
			m_curblock_firstpos = 0;
			m_blockitr = 0;
			m_curpos = 0;
		}
	}
	CATCH_ERROR_MAP( _TXT("error forward iterator skip document: %s"), *m_errorhnd);
}

Index ForwardIterator::skipPos( const Index& firstpos_)
{
	try
	{
		if (!m_dbadapter.get() || !m_docno) return 0;
		if (m_curblock.empty() || firstpos_ < m_curblock_firstpos || firstpos_ > m_curblock_lastpos)
		{
			if (!m_dbadapter->loadUpperBound( firstpos_, m_curblock))
			{
				m_curblock_lastpos = 0;
				m_curblock_firstpos = 0;
				m_blockitr = 0;
				m_curpos = 0;
				return 0;
			}
			else
			{
				m_curblock_lastpos = m_curblock.id();
				m_curblock_firstpos = m_curblock.position_at( m_curblock.charptr());
				m_blockitr = m_curblock.charptr();
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
			m_blockitr = m_curblock.charptr();
		}
		if (0!=(m_blockitr = m_curblock.upper_bound( firstpos_, m_blockitr)))
		{
			m_curpos = m_curblock.position_at( m_blockitr);
		}
		else
		{
			m_curpos = 0;
		}
		return m_curpos;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error forward iterator skip position: %s"), *m_errorhnd, 0);
}

std::string ForwardIterator::fetch()
{
	try
	{
		if (!m_blockitr || m_curblock.empty())
		{
			throw strus::runtime_error( _TXT( "forward iterator fetch called without a term selected"));
		}
		return std::string( m_curblock.value_at( m_blockitr));
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error forward iterator fetch value: %s"), *m_errorhnd, std::string());
}


