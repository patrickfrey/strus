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
#ifndef _STRUS_LVDB_METADATA_DESCRIPTION_HPP_INCLUDED
#define _STRUS_LVDB_METADATA_DESCRIPTION_HPP_INCLUDED
#include "strus/index.hpp"
#include "metaDataElement.hpp"
#include <utility>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

class MetaDataDescription
{
public:
	MetaDataDescription();
	MetaDataDescription( leveldb::DB* db);
	MetaDataDescription( const std::string& str);
	MetaDataDescription( const MetaDataDescription& o);

	MetaDataDescription& operator=( const MetaDataDescription& o);

	std::string tostring() const;

	std::size_t nofElements() const
	{
		return m_ar.size();
	}

	std::size_t bytesize() const
	{
		return ((m_bytesize+3)>>2)<<2;	//... aligned to 4 bytes
	}

	bool defined( const std::string& name_);
	void add( MetaDataElement::Type type_, const std::string& name_);

	const MetaDataElement* get( int handle) const
	{
		if ((std::size_t)handle >= m_ar.size()) throw std::logic_error("array bound read in MetaDataDescription::get()");
		return &m_ar[ handle];
	}
	int getHandle( const std::string& name_) const;
	bool hasElement( const std::string& name_) const;

	void load( leveldb::DB* db);
	void store( leveldb::WriteBatch& batch);

	typedef std::vector< std::pair< const MetaDataElement*, const MetaDataElement*> > TranslationMap;
	TranslationMap getTranslationMap( const MetaDataDescription& o) const;

	std::vector<std::string> columns() const;

private:
	std::size_t m_bytesize;
	std::vector<MetaDataElement> m_ar;
	std::map<std::string,std::size_t> m_namemap;
};

}//namespace
#endif

