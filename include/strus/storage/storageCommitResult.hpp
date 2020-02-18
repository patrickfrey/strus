/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Result structure of a storage transaction (queryable as boolean)
/// \file "storageCommitResult.hpp"
#ifndef _STRUS_STORAGE_COMMIT_RESULT_HPP_INCLUDED
#define _STRUS_STORAGE_COMMIT_RESULT_HPP_INCLUDED
#include "strus/storage/index.hpp"

namespace strus {

/// \class StorageCommitResult
/// \brief Result of a storage transaction (queryable as boolean)
class StorageCommitResult
{
public:
	/// \brief Default constructor
	StorageCommitResult()
		:m_success(false),m_nofDocumentsAffected(0){}
	/// \brief Copy constructor
	StorageCommitResult( const StorageCommitResult& o)
		:m_success(o.m_success),m_nofDocumentsAffected(o.m_nofDocumentsAffected){}

	/// \brief Assignment operator
	StorageCommitResult& operator=( const StorageCommitResult& o)
		{m_success=o.m_success;m_nofDocumentsAffected=o.m_nofDocumentsAffected; return *this;}

	/// \brief Constructor
	/// \param[in] nofDocumentsAffected_ number of documents affected by the transaction
	StorageCommitResult( bool success_, const strus::Index& nofDocumentsAffected_)
		:m_success(success_),m_nofDocumentsAffected(nofDocumentsAffected_){}

	/// \brief Get the number of documents affected by the transaction
	strus::Index nofDocumentsAffected() const	{return m_nofDocumentsAffected;}
	/// \brief Flag indicating the success of the transaction
	bool success() const				{return m_success;}

	/// \brief Cast to boolean indicating the success of the transaction
	operator bool() const				{return m_success;}

private:
	bool m_success;				///< flag indicating the success of the transaction
	strus::Index m_nofDocumentsAffected;	///< number of documents affected by the transaction
};

}//namespace
#endif

