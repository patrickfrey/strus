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
/// \brief Interface defining an input and an output queue for the peer messages of a StorageClient
/// \file peerMessageQueueInterface.hpp
#ifndef _STRUS_PEER_MESSAGE_QUEUE_INTERFACE_HPP_INCLUDED
#define _STRUS_PEER_MESSAGE_QUEUE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Forward declaration
class PeerMessageViewerInterface;
/// \brief Forward declaration
class PeerMessageBuilderInterface;
/// \brief Forward declaration
class PeerMessageProcessorInterface;

/// \brief Interface defining an input and an output queue for the peer messages of a storageClient
/// \note this interface is used for distributing a search index
class PeerMessageQueueInterface
{

public:
	/// \brief Destructor
	virtual ~PeerMessageQueueInterface(){}

	/// \brief Push a message blob from another peer storage
	/// \param[in] inmsg pointer to message blob from peer
	/// \param[in] inmsgsize size of msg blob in bytes
	/// \param[out] outmsg pointer to message to the sender peer of the last message pushed
	/// \param[out] outmsgsize size of outmsg in bytes
	/// \return true, if there is at least one reply message to fetch with fetchPeerReply(const char*&,std::size_t&)
	virtual void push( const char* inmsg, std::size_t inmsgsize, const char*& outmsg, std::size_t& outmsgsize)=0;

	/// \brief Fetches the next message to distribute to all other peers
	/// \param[in] msg pointer to message blob for other peers (interpretation depends on defined peer message processor)
	/// \param[in] msgsize size of msg blob in bytes
	/// \return false if there is no chunk of a message left or an error occurred
	virtual bool fetch( const char*& msg, std::size_t& msgsize)=0;
};

}//namespace
#endif

