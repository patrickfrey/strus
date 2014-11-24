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
		TermValuePrefix='i',	///< [term string]             ->  [valueno]
		DocIdPrefix='d',	///< [docid string]            ->  [docno]
		ForwardIndexPrefix='r',	///< [docno,typeno,position]   ->  [string]*
		VariablePrefix='v',	///< [variable string]         ->  [index]
		DocMetaDataPrefix='m',	///< [docno/1K,nameid]         ->  [float]*
		DocAttributePrefix='a',	///< [docno,nameid]            ->  [string]
		DocFrequencyPrefix='f',	///< [typeno,termno]           ->  [index]
		DocnoBlockPrefix='b',	///< [typeno,termno,docno]     ->  [index,ff,weight]*
		PosinfoBlockPrefix='p',	///< [typeno,termno,docno]     ->  [pos]*
		AttributeKeyPrefix='A'	///< [attribute string]        ->  [index]
	};
	static const char* keyPrefixName( KeyPrefix i)
	{
		switch (i)
		{
			case TermTypePrefix: return "term type";
			case TermValuePrefix: return "term value";
			case DocIdPrefix: return "docid";
			case ForwardIndexPrefix: return "forward index";
			case VariablePrefix: return "global variable";
			case DocMetaDataPrefix: return "metadata";
			case DocAttributePrefix: return "document attribute";
			case DocFrequencyPrefix: return "term document frequency";
			case DocnoBlockPrefix: return "docno posting block";
			case PosinfoBlockPrefix: return "posinfo posting block";
		}
		return 0;
	}

public:
	explicit DatabaseKey( char prefix=0);
	DatabaseKey( char prefix, const char* variable);
	DatabaseKey( char prefix, const Index& idx);
	DatabaseKey( char prefix, char prefix2, const Index& idx);
	DatabaseKey( char prefix, const Index& idx, char prefix2);
	DatabaseKey( char prefix, const Index& idx, const Index& idx2);
	DatabaseKey( char prefix, const Index& idx, const Index& idx2, const Index& idx3);
	DatabaseKey( const DatabaseKey& o);

	void addElem( const std::string& var);
	void addElem( const Index& index);
	void addPrefix( char prefix);

	char prefix() const			{return m_size?m_buf[0]:0;}
	void resize( std::size_t n);

	const char* ptr() const			{return m_buf;}
	std::size_t size() const		{return m_size;}

	bool operator < (const DatabaseKey& o) const;

private:
	enum {MaxKeySize=64};
	char m_buf[ MaxKeySize];
	std::size_t m_size;
};

}
#endif

