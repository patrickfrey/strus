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
/// \brief Interface for populating statistics to other peer storages and for pushing statistics of peer storages for local query evaluation.
/// \file StoragePeerInterface.hpp
#ifndef _STRUS_STORAGE_PEER_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_PEER_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Forward declaration
class PeerStorageTransactionInterface;
/// \brief Forward declaration
class StoragePeerClientInterface;


/// \brief Interface for populating statistics to other peer storages and for pushing statistics of peer storages for local query evaluation.
class StoragePeerInterface
{
public:
	/// \brief Destructor
	virtual ~StoragePeerInterface(){}

	/// \brief Pushes a message from another peer as part of a transaction to the storage client
	/// \param[in,out] transaction transaction object to work on
	/// \param[in] msgbody message sent from other peer
	/// \param[in] msgsize size of message in bytes
	virtual void pushPeerMessage( PeerStorageTransactionInterface* transaction, const char* msgbody, std::size_t msgsize) const;

	/// \brief Creates a client to populate statistics to other peers
	/// \param[in] configsource configuration string (not a filename!)
	/// \return the client object (with ownership returned)
	virtual StoragePeerClientInterface* createStoragePeerClient( const std::string& configsource) const=0;

	/// \brief Gets a configuration description (source string as used by the functions here)
	///	createStoragePeerClient(const std::string&)
	///	for the usage printed by programs using this implementation.
	virtual const char* getConfigDescription() const=0;

	/// \brief Get the list of known configuration parameter keys
	///	for verification of the configuration by programs using this implementation.
	virtual const char** getConfigParameters() const=0;
};

}//namespace
#endif

