/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for packing/unpacking messages with statistics (distributed index)
/// \file statisticsProcessorInterface.hpp
#ifndef _STRUS_STATISTICS_PROCESSOR_INTERFACE_HPP_INCLUDED
#define _STRUS_STATISTICS_PROCESSOR_INTERFACE_HPP_INCLUDED
#include "strus/timeStamp.hpp"
#include <string>

namespace strus
{

/// \brief Forward declaration
class StatisticsViewerInterface;
/// \brief Forward declaration
class StatisticsBuilderInterface;
/// \brief Forward declaration
class StatisticsMapInterface;
/// \brief Forward declaration
class StatisticsIteratorInterface;

/// \brief Interface for packing/unpacking messages with statistics used for query evaluation
/// \note this interface is used for distributing a search index
class StatisticsProcessorInterface
{
public:
	/// \brief Destructor
	virtual ~StatisticsProcessorInterface(){}

	/// \brief Creates a viewer for the contents of a statistics message
	/// \param[in] msgptr pointer to the packed statistics message blob (not necessarily copied by the viewer, lifetime assumed longer than that of viewer)
	/// \param[in] msgsize size of the packed statistics message blob in bytes
	/// \return the viewer object (with ownership returned) or NULL in case of a memory allocation error
	virtual StatisticsViewerInterface* createViewer( const void* msgptr, std::size_t msgsize) const=0;

	/// \brief Get an iterator on the stored statistics messages after a timestamp from the storage (after commit)
	/// \param[in] timestamp minimum data iterated statistics message should have
	/// \return the message or an empty blob indicating the end of messages or an error to check with the error buffer interface
	virtual StatisticsIteratorInterface* createIterator( const std::string& path, const TimeStamp& timestamp) const=0;

	/// \brief Creates a builder for a statistics message
	/// \param[in] path file path to use for storing files with the statistics
	/// \return the builder object (with ownership returned) or NULL in case of a memory allocation error
	virtual StatisticsBuilderInterface* createBuilder( const std::string& path) const=0;

	/// \brief Creates a map for global statistics
	/// \note You can implement the map in your own way, this is just an example implementation provided by the statistics processor
	/// \return the map object (with ownership returned) or NULL in case of a memory allocation error
	virtual StatisticsMapInterface* createMap() const=0;
};

}//namespace
#endif

