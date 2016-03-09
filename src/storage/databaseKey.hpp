/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_DATABASE_ADAPTOR_KEY_HPP_INCLUDED
#define _STRUS_DATABASE_ADAPTOR_KEY_HPP_INCLUDED
#include "strus/index.hpp"
#include "blockKey.hpp"
#include <utility>
#include <string>

namespace strus {

class DatabaseKey
{
public:
	enum KeyPrefix
	{
		TermTypePrefix='T',	///< [type string]             ->  [typeno]
		TermValuePrefix='I',	///< [term string]             ->  [valueno]
		DocIdPrefix='D',	///< [docid string]            ->  [docno]
		VariablePrefix='V',	///< [variable string]         ->  [index]
		AttributeKeyPrefix='A',	///< [attribute string]        ->  [index]
		UserNamePrefix='U',	///< [name string]             ->  [userno]
		TermTypeInvPrefix='K',	///< [typeno]                  ->  [type string]
		TermValueInvPrefix='N',	///< [valueno]                 ->  [term string]

		ForwardIndexPrefix='r',	///< [typeno,docno,position]   ->  [string]*
		PosinfoBlockPrefix='p',	///< [typeno,termno,docno]     ->  [pos]*
		InverseTermPrefix='i',	///< [docno]                   ->  [typeno,termno,ff,firstpos]*

		UserAclBlockPrefix='u',	///< [userno,docno]            ->  [bit]*
		AclBlockPrefix='w',	///< [docno,userno]            ->  [bit]*
		DocListBlockPrefix='d',	///< [typeno,termno,docno]     ->  [bit]*

		DocMetaDataPrefix='m',	///< [docno/1K,nameid]         ->  [float]*
		DocAttributePrefix='a',	///< [docno,nameid]            ->  [string]
		DocFrequencyPrefix='f',	///< [typeno,termno]           ->  [index]

		MetaDataDescrPrefix='M'	///< []                        ->  [string]
	};
	static const char* keyPrefixName( KeyPrefix i)
	{
		switch (i)
		{
			case TermTypePrefix: return "term type";
			case TermValuePrefix: return "term value";
			case DocIdPrefix: return "docid";
			case VariablePrefix: return "global variable";
			case AttributeKeyPrefix: return "document attribute name";
			case UserNamePrefix: return "user id";
			case TermTypeInvPrefix: return "term type inv";
			case TermValueInvPrefix: return "term value inv";

			case ForwardIndexPrefix: return "forward index";
			case PosinfoBlockPrefix: return "posinfo posting block";
			case InverseTermPrefix: return "inverse terminfo block";
			case UserAclBlockPrefix: return "user ACL block";
			case AclBlockPrefix: return "inverted ACL block";
			case DocListBlockPrefix: return "doc posting block";

			case DocMetaDataPrefix: return "metadata";
			case DocAttributePrefix: return "document attribute";
			case DocFrequencyPrefix: return "term document frequency";

			case MetaDataDescrPrefix: return "meta data description";
		}
		return 0;
	}

public:
	explicit DatabaseKey( char prefix);
	DatabaseKey( char prefix, const Index& idx);
	DatabaseKey( char prefix, const BlockKey& key, const Index& elemidx=0);
	DatabaseKey( char prefix, const std::string& varname);
	DatabaseKey( const DatabaseKey& o);

	char prefix() const			{return m_size?m_buf[0]:0;}

	const char* ptr() const			{return m_buf;}
	std::size_t size() const		{return m_size;}

public:
	void addElem( const std::string& var);
	void addElem( const Index& index);
	void addPrefix( char prefix);
	void resize( std::size_t n);

private:
	enum {MaxKeySize=64};
	char m_buf[ MaxKeySize];
	std::size_t m_size;
};

}
#endif

