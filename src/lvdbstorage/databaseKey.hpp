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
#ifndef _STRUS_LVDB_DATABASE_KEY_HPP_INCLUDED
#define _STRUS_LVDB_DATABASE_KEY_HPP_INCLUDED
#include "strus/index.hpp"
#include <utility>

namespace strus {

class DatabaseKey
{
public:
	enum KeyPrefix
	{
		TermTypePrefix='t',	///< [type string]             ->  [typeno]
		TermValuePrefix='i',	///< [term string]             ->  [termno]
		DocIdPrefix='d',	///< [docid string]            ->  [docno]
		LocationPrefix='o',	///< [type,term,docno]         ->  [pos]*
		InversePrefix='r',	///< [docno,typeno,position]   ->  [term string]*
		VariablePrefix='v',	///< [variable string]         ->  [index]
		DocMetaDataPrefix='m',	///< [docno/1K,nameid]         ->  [float]*
		DocAttributePrefix='a',	///< [docno,nameid]            ->  [string]
		DocFrequencyPrefix='f'	///< [type,term]               ->  [index]
	};

public:
	explicit DatabaseKey( char prefix=0);
	DatabaseKey( char prefix, const Index& idx);
	DatabaseKey( char prefix, char prefix2, const Index& idx);
	DatabaseKey( char prefix, const Index& idx, char prefix2);
	DatabaseKey( char prefix, const Index& idx, const Index& idx2);
	DatabaseKey( const DatabaseKey& o);

	void addElem( const Index& index);
	void addPrefix( char prefix);

	void resize( std::size_t n);

	const char* ptr() const			{return m_buf;}
	std::size_t size() const		{return m_size;}

	Index elem( std::size_t pos) const;

private:
	enum {MaxKeySize=64};
	char m_buf[ MaxKeySize];
	std::size_t m_size;
};

}
#endif

