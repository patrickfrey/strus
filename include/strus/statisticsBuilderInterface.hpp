/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a builder for a message to populate statistics (distributed index)
/// \file statisticsBuilderInterface.hpp
#ifndef _STRUS_STATISTICS_BUILDER_INTERFACE_HPP_INCLUDED
#define _STRUS_STATISTICS_BUILDER_INTERFACE_HPP_INCLUDED
#include "strus/timeStamp.hpp"
#include "strus/storage/statisticsMessage.hpp"
#include <cstdlib>
#include <string>

namespace strus
{
///\brief Forward declaration
class StatisticsIteratorInterface;

/// \brief Interface for a builder for a statistics message (distributed index)
class StatisticsBuilderInterface
{
public:
	/// \brief Destructor
	virtual ~StatisticsBuilderInterface(){}

	/// \brief Add a change of the number of document inserted
	/// \param[in] increment positive or negative (decrement) value of the local change of the collection size
	virtual void addNofDocumentsInsertedChange( int increment)=0;

	/// \brief Add a message propagating a change in the df (document frequency)
	/// \param[in] termtype type of the term
	/// \param[in] termvalue value of the term
	/// \param[in] increment positive or negative (decrement) value of the local change of the document frequency
	/// \return true on success, false in case of an error (memory allocation error)
	virtual void addDfChange( const char* termtype, const char* termvalue, int increment)=0;

	/// \brief Create an iterator on the elements of the elements of this before a commit or rollback
	/// \return the iterator or NULL if the operation failed
	/// \note the elements are moved with ownership to the created iterator, commit has no effect after this call and the iterator can be created only once
	virtual StatisticsIteratorInterface* createIteratorAndRollback()=0;

	/// \brief Transaction commit
	virtual bool commit()=0;

	/// \brief Rollback to the last state marked with 'start()'
	virtual void rollback()=0;

	/// \brief Release statistics that are older than a defined timestamp
	/// \param[in] timestamp minimum data a surviving (not deleted) statistics message should have
	virtual void releaseStatistics( const TimeStamp& timestamp)=0;
};
}//namespace
#endif

