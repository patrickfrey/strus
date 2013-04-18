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
#ifndef _STRUS_KCF_STORAGE_HPP_INCLUDED
#define _STRUS_KCF_STORAGE_HPP_INCLUDED
#include "strus/storage.hpp"

namespace strus
{

///\class StorageImpl
///\brief Implementation for the storage of IR terms with their occurrencies with kytocabinet and files for the blocks
class StorageImpl
	:public Storage
{
public:
	virtual DocNumber storeDocument( const Document& doc);

	virtual std::string getDocumentId( const DocNumber& docnum);
	virtual std::size_t getDocumentSize( const DocNumber& docnum);
	virtual DocNumber getDocumentNumber( const std::string& docid);
	virtual TermNumber getTermNumber( const std::string& type, const std::string& value);

	virtual bool openIterator( PositionChunk& itr, const TermNumber& termnum);
	virtual bool nextIterator( PositionChunk& itr);
	virtual void closeIterator( PositionChunk& itr);
};

} //namespace
#endif



