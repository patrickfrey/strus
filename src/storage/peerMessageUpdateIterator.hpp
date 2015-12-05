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
/// \brief Implementation of the iterators on peer messages of the local storage for updating other peers
/// \file peerMessageUpdateIterator.hpp
#ifndef _STRUS_PEER_MESSAGE_UPDATE_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_PEER_MESSAGE_UPDATE_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/peerMessageIteratorInterface.hpp"

namespace strus
{

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class StorageClient;

/// \brief Interface for a iterator on peer messages of the local storage (updates) for other peers
/// \note this interface is used for distributing a search index
class PeerMessageUpdateIterator
	:public PeerMessageIteratorInterface
{

public:
	PeerMessageUpdateIterator(
			StorageClient* storage_,
			ErrorBufferInterface* errorhnd_);

	virtual ~PeerMessageUpdateIterator(){}

	virtual bool getNext( const char*& msg, std::size_t& msgsize);

private:
	StorageClient* m_storage;					///< storage related
	ErrorBufferInterface* m_errorhnd;				///< error buffer for exception free interface
};

}//namespace
#endif

