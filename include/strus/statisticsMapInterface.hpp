/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a map of global statistics (in case of a distributed index)
/// \file statisticsMapInterface.hpp
#ifndef _STRUS_STATISTICS_MAP_INTERFACE_HPP_INCLUDED
#define _STRUS_STATISTICS_MAP_INTERFACE_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "strus/storage/termStatisticsChange.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Interface for a map of global statistics (in case of a distributed index)
/// \note The interface implements part of the statistics builder interface for the writing of stats
class StatisticsMapInterface
{
public:
	/// \brief Destructor
	virtual ~StatisticsMapInterface(){}

	/// \brief Propagate a change of the number of document inserted
	/// \param[in] increment positive or negative (decrement) value of the local change of the collection size
	virtual void addNofDocumentsInsertedChange( int increment)=0;

	/// \brief Propagate a change in the df (document frequency)
	/// \param[in] termtype type of the term
	/// \param[in] termvalue value of the term
	/// \param[in] increment positive or negative (decrement) value of the local change of the document frequency
	/// \return true on success, false in case of an error (memory allocation error)
	virtual void addDfChange( const char* termtype, const char* termvalue, int increment)=0;

	/// \brief Propagate a blob with some statistics to encode with a viewer associated with this statistics map
	/// \param[in] msgptr pointer to message
	/// \param[in] msgsize size of the message blob in bytes
	/// \return true on success, false in case of an error (memory allocation error)
	virtual bool processStatisticsMessage( const void* msgptr, std::size_t msgsize)=0;

	/// \brief Get the total number of documents stored in the map
	/// \return the number of documents in the collection
	virtual GlobalCounter nofDocuments() const=0;

	/// \brief Get the df (document frequency) stored in the map
	/// \param[in] termtype type of the term
	/// \param[in] termvalue value of the term
	/// \return the document frequency
	virtual GlobalCounter df( const std::string& termtype, const std::string& termvalue) const=0;

	/// \brief Get the list of types known
	/// \return the list of types
	virtual std::vector<std::string> types() const=0;
};
}//namespace
#endif

