/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
		StructTypePrefix='S',	///< [struct type string]      ->  [structno]
		DocIdPrefix='D',	///< [docid string]            ->  [docno]
		VariablePrefix='V',	///< [variable string]         ->  [index]
		AttributeKeyPrefix='A',	///< [attribute string]        ->  [index]
		UserNamePrefix='U',	///< [name string]             ->  [userno]
		TermTypeInvPrefix='K',	///< [typeno]                  ->  [type-string]
		TermValueInvPrefix='N',	///< [valueno]                 ->  [term-string]
		StructTypeInvPrefix='X',///< [structno]                ->  [struct-type-string]

		ForwardIndexPrefix='r',	///< [typeno,docno,position]   ->  [string]*
		PosinfoBlockPrefix='p',	///< [typeno,termno,docno]     ->  [pos]*
		FfBlockPrefix='o',	///< [typeno,termno,docno]     ->  [ff]
		StructBlockPrefix='s',	///< [docno]                   ->  [struct]*
		InverseTermPrefix='i',	///< [docno]                   ->  [typeno,termno,ff,firstpos]*

		UserAclBlockPrefix='u',	///< [userno,docno]            ->  [bit]*
		AclBlockPrefix='w',	///< [docno,userno]            ->  [bit]*
		DocListBlockPrefix='d',	///< [typeno,termno,docno]     ->  [bit]*

		DocMetaDataPrefix='m',	///< [docno/1K,nameid]         ->  [float/int32/short/char]*
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
			case StructTypePrefix: return "struct type";
			case DocIdPrefix: return "docid";
			case VariablePrefix: return "global variable";
			case AttributeKeyPrefix: return "document attribute name";
			case UserNamePrefix: return "user id";
			case TermTypeInvPrefix: return "term type inv";
			case TermValueInvPrefix: return "term value inv";
			case StructTypeInvPrefix: return "struct type inv";

			case ForwardIndexPrefix: return "forward index";
			case PosinfoBlockPrefix: return "posinfo posting block";
			case FfBlockPrefix: return "first access ff block";
			case StructBlockPrefix: return "structure block";
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

