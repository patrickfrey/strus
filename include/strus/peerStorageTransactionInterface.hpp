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
/// \brief Interface for a transaction on the global statistics of the storage from another peer
/// \file peerStorageTransactionInterface.hpp
#ifndef _STRUS_PEER_STORAGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#define _STRUS_PEER_STORAGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Interface for a transaction on the global statistics of the storage from another peer
/// \note this interface is used for distributing a search index
class PeerStorageTransactionInterface
{

public:
	/// \brief Destructor
	virtual ~PeerStorageTransactionInterface(){}

	/// \brief Push a message from another peer storage
	/// \param[in] inmsg pointer to message from peer storage
	/// \param[in] inmsgsize size of msg blob in bytes
	/// \param[in] sign true, if the sign of the increments is positive, false if negative (inverted sign, decrement)
	virtual void push( const char* inmsg, std::size_t inmsgsize, bool sign=true)=0;

	/// \brief Transaction commit
	/// \param[out] outmsg pointer to message to the sender peer that invoked the transaction
	/// \param[out] outmsgsize size of outmsg in bytes
	/// \return true on success, false if the operation failed
	virtual bool commit( const char*& outmsg, std::size_t& outmsgsize)=0;

	/// \brief Transaction rollback
	virtual void rollback()=0;
};

}//namespace
#endif

