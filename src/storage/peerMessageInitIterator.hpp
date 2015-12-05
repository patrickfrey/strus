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
/// \brief Implementation of the iterator on peer messages of the local storage for other peers in a registration phase
/// \file peerMessageInitIterator.hpp
#ifndef _STRUS_PEER_MESSAGE_INIT_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_PEER_MESSAGE_INIT_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/peerMessageIteratorInterface.hpp"
#include "strus/peerMessageBuilderInterface.hpp"
#include "strus/reference.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class PeerMessageProcessorInterface;
/// \brief Forward declaration
class PeerMessageBuilderInterface;

/// \brief Interface for a iterator on peer messages of the local storage (state) for other peers
/// \note this interface is used for distributing a search index
class PeerMessageInitIterator
	:public PeerMessageIteratorInterface
{

public:
	PeerMessageInitIterator(
			StorageClientInterface* storage_,
			DatabaseClientInterface* database_,
			ErrorBufferInterface* errorhnd_);

	virtual ~PeerMessageInitIterator(){}

	virtual bool getNext( const char*& msg, std::size_t& msgsize);

private:
	StorageClientInterface* m_storage;				///< storage related
	DatabaseClientInterface* m_database;				///< database related
	const PeerMessageProcessorInterface* m_proc;			///< interface for processing messages (packing,unpacking)
	Reference<PeerMessageBuilderInterface> m_peerMessageBuilder;	///< peer message builder
	ErrorBufferInterface* m_errorhnd;				///< error buffer for exception free interface
};

}//namespace
#endif

