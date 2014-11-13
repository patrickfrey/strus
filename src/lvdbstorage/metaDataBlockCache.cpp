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
#include "metaDataBlockCache.hpp"

using namespace strus;

MetaDataBlockCache::MetaDataBlockCache( leveldb::DB* db_)
	:m_db(db_)
{}

void MetaDataBlockCache::BlockArray::resize( std::size_t nofBlocks_)
{
	boost::shared_array<MetaDataBlockCache::BlockRef>
		new_ar( new MetaDataBlockCache::BlockRef[ nofBlocks_]);
	std::size_t ii=0;
	std::size_t nn=(nofBlocks_ < arsize)?nofBlocks_:arsize;
	std::size_t varidx = aridx(varname_);
	for (;ii<nn;++ii)
	{
		new_ar[ii] = ar[varidx][ii];
	}
	arsize = nofBlocks_;
	ar = new_ar;
}

MetaDataBlockRef MetaDataBlockCache::BlockArray::operator[]( std::size_t idx) const
{
	if (idx >= arsize)
	{
		return MetaDataBlockRef();
	}
	return ar.get()[ idx];
}

MetaDataBlockRef& MetaDataBlockCache::BlockArray::operator[]( std::size_t idx)
{
	if (idx >= arsize)
	{
		resize( idx + 1000);
	}
	return ar.get()[ idx];
}

void MetaDataBlockCache::resetBlock( Index docno_, char varname_)
{
	std::size_t bn = (std::size_t)(docno_ / MetaDataBlock::MetaDataBlockSize);
	if (bn < m_arsize)
	{
		m_ar[ aridx(varname_)][ bn].ref.reset();
	}
}

float MetaDataBlockCache::getValue( Index docno_, char varname_)
{
	MetaDataBlockRef blkref;
	std::size_t bn = (std::size_t)(docno_ / MetaDataBlock::MetaDataBlockSize);
	if (bn < m_arsize)
	{
		blkref = m_ar[ aridx(varname_)][ bn];
	}
	if (!blkref.get())
	{
		BlockRef& ref = m_ar[ aridx(varname_)][ bn].ref;
		boost::mutex::scoped_lock( ref.mutex);
		ref.reset( MetaDataReader::readBlockFromDB( 
				m_db, MetaDataBlock::blockno( docno_), m_varname));
		blkref = ref.ref;
	}
	return blkref->getValue( docno_);
}



