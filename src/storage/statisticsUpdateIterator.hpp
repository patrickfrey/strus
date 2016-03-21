/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the iterators on statistics of a storage for updating the global statistics
/// \file statisticsUpdateIterator.hpp
#ifndef _STRUS_STATISTICS_UPDATE_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STATISTICS_UPDATE_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/statisticsIteratorInterface.hpp"

namespace strus
{

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class StorageClient;

/// \brief Interface for a iterator on update of statistics of a storage for the global statistics
/// \note this interface is used for distributing a search index
class StatisticsUpdateIterator
	:public StatisticsIteratorInterface
{

public:
	StatisticsUpdateIterator(
			StorageClient* storage_,
			ErrorBufferInterface* errorhnd_);

	virtual ~StatisticsUpdateIterator(){}

	virtual bool getNext( const char*& msg, std::size_t& msgsize);

private:
	StorageClient* m_storage;					///< storage related
	ErrorBufferInterface* m_errorhnd;				///< error buffer for exception free interface
};

}//namespace
#endif

