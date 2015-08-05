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
/// \brief Client interface for forwarding statistics of other peer storages (distributed index)
/// \file storagePeerClientInterface.hpp
#ifndef _STRUS_STORAGE_PEER_CLIENT_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_PEER_CLIENT_INTERFACE_HPP_INCLUDED

namespace strus
{

/// \brief Forward declaration
class StoragePeerTransactionInterface;

/// \brief Client interface for forwarding statistics of other peers (distributed index)
class StoragePeerClientInterface
{
public:
	/// \brief Destructor
	virtual ~StoragePeerClientInterface(){}

	/// \brief Creates a new transaction object to distribute statistics to other peers
	/// \return return the pointer to the transaction object (with ownership - to be disposed with 'delete')
	virtual StoragePeerTransactionInterface* createTransaction() const=0;
};
}//namespace
#endif

