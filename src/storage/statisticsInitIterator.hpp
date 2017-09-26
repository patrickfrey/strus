/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the iterator on statistics of the storage to populate in a registration/deregistration phase
/// \file statisticsInitIterator.hpp
#ifndef _STRUS_STATISTICS_INIT_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STATISTICS_INIT_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/statisticsIteratorInterface.hpp"
#include "strus/statisticsBuilderInterface.hpp"
#include "strus/reference.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StatisticsProcessorInterface;
/// \brief Forward declaration
class StatisticsBuilderInterface;

/// \brief Implementation of the iterator on statistics of the storage to populate in a registration/deregistration phase
class StatisticsInitIterator
	:public StatisticsIteratorInterface
{

public:
	/// \param[in] sign true = positive, false = negative, means all offsets are inverted and isnew is false too (deregistration)
	StatisticsInitIterator(
			StorageClientInterface* storage_,
			DatabaseClientInterface* database_,
			bool sign_,
			ErrorBufferInterface* errorhnd_);

	virtual ~StatisticsInitIterator(){}

	virtual bool getNext( const void*& msg, std::size_t& msgsize);

private:
	StorageClientInterface* m_storage;				///< storage related
	DatabaseClientInterface* m_database;				///< database related
	const StatisticsProcessorInterface* m_proc;			///< interface for processing messages (packing,unpacking)
	Reference<StatisticsBuilderInterface> m_statisticsBuilder;	///< statistics message builder
	ErrorBufferInterface* m_errorhnd;				///< error buffer for exception free interface
}; 

}//namespace
#endif

