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

namespace strus
{

/// \brief Interface for a transaction on the local storage opened by a peer for updating global statistic changes
class PeerStorageTransactionInterface
{
public:
	/// \brief Destructor
	/// \note The destructor should do an automatic rollback if not issued yet
	virtual ~PeerStorageTransactionInterface(){}

	/// \brief Prepare to update the total number of documents inserted
	/// \param[in] increment value to use for update (positive or negative)
	/// \note throws if the update was not successful
	virtual void updateNofDocumentsInsertedChange(
			int increment)=0;

	/// \brief Prepare to update the global document frequency used for calculations
	/// \param[in] termtype type of the term
	/// \param[in] termvalue value of the term
	/// \param[in] increment change (positive or negative value)
	/// \note throws if the operation was not successful
	virtual void updateDocumentFrequencyChange(
			const char* termtype,
			const char* termvalue,
			int increment)=0;

	/// \brief Commit of the transaction
	virtual void commit()=0;

	/// \brief Rollback of the transaction
	virtual void rollback()=0;
};
}//namespace
#endif

