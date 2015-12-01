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
/// \brief Implementation of input and an output queue for the peer messages of the StorageClient
/// \file peerMessageQueue.hpp
#ifndef _STRUS_PEER_MESSAGE_QUEUE_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_PEER_MESSAGE_QUEUE_IMPLEMENTATION_HPP_INCLUDED
#include "strus/peerMessageQueueInterface.hpp"
#include "strus/reference.hpp"
#include "storageClient.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class PeerMessageViewerInterface;
/// \brief Forward declaration
class PeerMessageBuilderInterface;
/// \brief Forward declaration
class PeerMessageProcessorInterface;

/// \brief Implementation of input and an output queue for the peer messages of the StorageClient
class PeerMessageQueue
	:public PeerMessageQueueInterface
{

public:
	PeerMessageQueue(
			StorageClient* storage_,
			DatabaseClientInterface* database_,
			const PeerMessageProcessorInterface* proc_,
			ErrorBufferInterface* errorhnd_);
	virtual ~PeerMessageQueue();

	virtual void start( bool sign);
	virtual void push( const char* inmsg, std::size_t inmsgsize, const char*& outmsg, std::size_t& outmsgsize);
	virtual bool fetch( const char*& msg, std::size_t& msgsize);

	virtual const PeerMessageProcessorInterface* getMessageProcessor() const
	{
		return m_proc;
	}

private:
	StorageClient* m_storage;					///< storage related
	DatabaseClientInterface* m_database;				///< database related
	const PeerMessageProcessorInterface* m_proc;			///< interface for processing messages (packing,unpacking)
	Reference<PeerMessageBuilderInterface> m_peerMessageBuilder;	///< peer message builder
	std::string m_peerReplyMessageBuffer;				///< buffer for peer message replies
	std::vector<std::string> m_bufferedPeerMessageList;		///< queue of peer messages that came in before the initial statistics were processed at startup.
	std::size_t m_bufferedPeerMessageIdx;				///< fetch index in m_bufferedPeerMessageList queue
	ErrorBufferInterface* m_errorhnd;				///< error buffer for exception free interface
};

}//namespace
#endif

