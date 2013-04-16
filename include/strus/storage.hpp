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
#ifndef _STRUS_STORAGE_HPP_INCLUDED
#define _STRUS_STORAGE_HPP_INCLUDED
#include "position.hpp"
#include <string>
#include <map>
#include <stdexcept>

namespace strus
{

struct PositionChunk
{
	struct StorageHandle;
	StorageHandle* m_handle;

	const Position* m_posar;
	unsigned int m_posarsize;
	unsigned int m_posidx;
	bool m_eof;

	PositionChunk()
		:m_handle(0),m_posar(0),m_posarsize(0),m_posidx(0),m_eof(true){}
};

///\class Storage
///\brief Interface for the storage of IR terms with their occurrencies
class Storage
{
public:
	typedef std::map<std::string, boost::vector<DocPosition> > TermPosMap=0;

	virtual DocNum storeDocument( const std::string& docid, const TermPosMap& doc)=0;

	virtual std::string getDocumentId( const DocNum& docnum)=0;
	virtual DocNum getDocumentNumber( const std::string& docid)=0;
	virtual TermNum getTermNumber( const std::string& type, const std::string& value)=0;

	virtual bool openIterator( PositionChunk& itr, const TermNum& termnum)=0;
	virtual bool nextIterator( PositionChunk& itr)=0;
	virtual void closeIterator( PositionChunk& itr)=0;
};

}//namespace
#endif

