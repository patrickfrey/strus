/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#ifndef _STRUS_KCF_DATABASE_HPP_INCLUDED
#define _STRUS_KCF_DATABASE_HPP_INCLUDED
#include "strus/storage.hpp"
#include "strus/position.hpp"
#include "keytable.hpp"
#include "blocktable.hpp"
#include "podvector.hpp"
#include <string>
#include <utility>
#include <boost/signals2/mutex.hpp>

namespace strus
{

///\class StorageDB
///\brief Implementation of the storage database
class StorageDB
{
public:
	enum
	{
		SmallBlockSize=128,
		IndexBlockSize=4096
	};
	struct IndexBlockHeader
	{
		char type;
		unsigned short fillpos;
	};
	struct INodeBlock
	{
		IndexBlockHeader hdr;
		struct Elem
		{
			Position pos;
			BlockNumber bno;
		};
		enum
		{
			NofElem = ((IndexBlockSize - sizeof( IndexBlockHeader)) / sizeof(Elem)),
			Size = ((int)NofElem * sizeof(Elem)
		};
		Elem ar[ NofElem];
	};
	struct PackBlock
	{
		IndexBlockHeader hdr;
		enum
		{
			NofElem = ((IndexBlockSize - sizeof( IndexBlockHeader)) / sizeof(char)),
			Size = ((int)NofElem * sizeof(char)
		};
		char ar[ Size];
	};

	StorageDB( const std::string& name_, const std::string& path_, bool writemode_=false);
	virtual ~StorageDB();

	void open();
	void close();

	void lock();
	void unlock();

	static void create( const std::string& name, const std::string& path=std::string());

	TermNumber findTermNumber( const std::string& type, const std::string& value) const;
	TermNumber insertTermNumber( const std::string& type, const std::string& value);

	DocNumber findDocumentNumber( const std::string& docid) const;
	DocNumber insertDocumentNumber( const std::string& docid);

	std::pair<std::string,std::string> getTerm( const TermNumber& tn);
	std::string getDocumentId( const DocNumber& dn);

	std::pair<BlockNumber,bool> getTermBlockNumber( const TermNumber& tn);
	void setTermBlockNumber( const TermNumber& tn, const BlockNumber& bn, bool isSmallBlock);

	BlockNumber allocSmallBlock();
	BlockNumber allocIndexBlock();

	void writeSmallBlock( const BlockNumber& idx, const void* data);
	void writeIndexBlock( const BlockNumber& idx, const void* data);

	void writePartialSmallBlock( const BlockNumber& idx, const void* data, std::size_t start);
	void writePartialIndexBlock( const BlockNumber& idx, const void* data, std::size_t start);

	void readSmallBlock( const BlockNumber& idx, void* data);
	void readIndexBlock( const BlockNumber& idx, void* data);

private:
	bool m_writemode;
	std::string m_name;
	std::string m_path;
	KeyTable m_termtable;
	PodVector<BlockNumber> m_termblockmap;
	KeyTable m_typetable;
	boost::mutex m_typetable_mutex;
	KeyTable m_docidtable;
	boost::mutex m_docidtable_mutex;
	BlockTable m_smallblktable;
	BlockTable m_indexblktable;
	File m_transaction_lock;
	void* m_nulledblock;
};

} //namespace
#endif


