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
#ifndef _STRUS_STORAGE_PEER_TRANSACTION_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_PEER_TRANSACTION_INTERFACE_HPP_INCLUDED

namespace strus
{

/// \brief Interface for a transaction to populate changes of global statistics from the local storage to its peers
class StoragePeerTransactionInterface
{
public:
	/// \brief Destructor
	/// \note The destructor should do an automatic rollback if not issued yet
	virtual ~StoragePeerTransactionInterface(){}

	/// \brief Prepare to send the local change of the total number of document inserted to all other peers
	/// \param[in] increment positive or negative (decrement) value of the local change of the collection size
	/// \note throws if the send was not successful (the transport layer must guarantee that this change will be delivered, if the send operation was successful)
	virtual void populateNofDocumentsInsertedChange(
			int increment)=0;

	/// \brief Prepare to send the local change of the total document frequency of a term to all other peers
	/// \param[in] termtype type of the term
	/// \param[in] termvalue value of the term
	/// \param[in] increment positive or negative (decrement) value of the local change of the document frequency
	/// \param[in] isnew true, if the feature is new in the index of the sender. Triggers the receivers to send their value back for update
	/// \note throws if the operation was not successful
	virtual void populateDocumentFrequencyChange(
			const char* termtype,
			const char* termvalue,
			int increment,
			bool isnew)=0;

	/// \brief Query to commit of the transaction (two phase commit)
	/// \remark The query to commit does not have to guarantee that all peers get up to date immediately. But it has to guarantee that no update is lost (persistent queue or something similar)
	virtual void try_commit()=0;

	/// \brief Final commit of the transaction
	/// \remark This operation MUST NOT throw but a fatal exception that leads to an abort. It may issue an alert if the system operation to finalize the commit failed. The try_commit called before must guarantee that no data is lost.
	virtual void final_commit()=0;

	/// \brief Rollback of the transaction
	virtual void rollback()=0;
};
}//namespace
#endif

