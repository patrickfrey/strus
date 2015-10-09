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
///\brief Class that manages the population of statistics to other peers
///\file initialStatsPopulateState.hpp
#ifndef _STRUS_STORAGE_INITIAL_STATISTICS_POPULATE_STATE_HPP_INCLUDED
#define _STRUS_STORAGE_INITIAL_STATISTICS_POPULATE_STATE_HPP_INCLUDED
#include "strus/peerMessageBuilderInterface.hpp"
#include "strus/peerMessageProcessorInterface.hpp"
#include "strus/index.hpp"
#include "strus/reference.hpp"
#include <vector>
#include <string>
#include <map>

namespace strus {

///\brief Forward declaration
class DatabaseClientInterface;


class InitialStatsPopulateState
{
public:
	///\brief Constructor
	InitialStatsPopulateState()
		:m_peermsgproc(0){}

	///\brief Initializes a run of this
	///\param[in] peermsgproc_ processor to use for packing/unpacking peer messages
	///\param[in] database_ database client interface to use (owned by caller)
	///\param[in] nofDocuments_ initial number of documents to announce, if doPopulateInitialState_ == true
	void init( const PeerMessageProcessorInterface* peermsgproc_, DatabaseClientInterface* database_, const Index& nofDocuments_);

	///\brief Check if there are more messages to fetch
	bool running() const	{return m_peermsgproc;}

	///\brief Fetch the next chunk to send to the peers
	/// \param[out] blk pointer to the message 
	/// \param[out] blksize size of message blk in bytes
	/// \return true, if there is a message returned to be sent, false if not
	bool fetchMessage( const char* blk, std::size_t blksize);

private:
	const PeerMessageProcessorInterface* m_peermsgproc;		///< reference to interface to other peer storages
	Reference<PeerMessageBuilderInterface> m_peerMessageBuilder;	///< reference to builder of messages to other peers
};
}
#endif


