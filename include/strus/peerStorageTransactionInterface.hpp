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
#ifndef _STRUS_PEER_STORAGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#define _STRUS_PEER_STORAGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Interface for a transaction of global statistic changes
class PeerStorageTransactionInterface
{
public:
	/// \brief Destructor
	/// \note The destructor should do an automatic rollback if not issued yet
	virtual ~PeerStorageTransactionInterface(){}

	/// \brief Try to send the local change of the collection size to all other peers
	/// \param[in] increment positive or negative (decrement) value of the local change of the collection size
	/// \note throws if the send was not successful (the transport layer must guarantee that this change will be delivered, if the send operation was successful)
	virtual void populateCollectionSizeChange(
			int increment) const=0;

	/// \brief Try to send the local change of the document frequency of a term to all other peers
	/// \param[in] termtype type of the term
	/// \param[in] termvalue value of the term
	/// \param[in] increment positive or negative (decrement) value of the local change of the document frequency
	/// \note throws if the send was not successful (the transport layer must guarantee that this change will be delivered, if the send operation was successful)
	virtual void populateDocumentFrequencyChange(
			const std::string& termtype,
			const std::string& termvalue,
			int increment) const=0;

	
	/// \brief Commit of the transaction
	/// \remark The commit does not have to guarantee that all peers got up to date. But it has to ensure that all messages will be delivered.
	virtual void commit()=0;

	/// \brief Rollback of the transaction
	/// \remark The commit does not have to guarantee that all peers got up to date. But it has to ensure that all messages will be delivered.
	virtual void rollback()=0;
};
}//namespace
#endif

