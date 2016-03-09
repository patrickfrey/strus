/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Interface for a builder for a message to populate statistics (distributed index)
/// \file statisticsBuilderInterface.hpp
#ifndef _STRUS_STATISTICS_BUILDER_INTERFACE_HPP_INCLUDED
#define _STRUS_STATISTICS_BUILDER_INTERFACE_HPP_INCLUDED
#include <cstdlib>
#include <string>

namespace strus
{

/// \brief Interface for a builder for a statistics message (distributed index)
class StatisticsBuilderInterface
{
public:
	/// \brief Destructor
	virtual ~StatisticsBuilderInterface(){}

	/// \brief Define the change of the number of document inserted
	/// \param[in] increment positive or negative (decrement) value of the local change of the collection size
	virtual void setNofDocumentsInsertedChange(
			int increment)=0;

	/// \brief Add a message propagating a change in the df (document frequency)
	/// \param[in] termtype type of the term
	/// \param[in] termvalue value of the term
	/// \param[in] increment positive or negative (decrement) value of the local change of the document frequency
	/// \return true on success, false in case of an error (memory allocation error)
	virtual void addDfChange(
			const char* termtype,
			const char* termvalue,
			int increment)=0;

	/// \brief Mark the current state that can be restored with a rollback
	virtual void start()=0;

	/// \brief Rollback to the last state marked with 'start()'
	virtual void rollback()=0;

	/// \brief Get the packed statistics message
	/// \param[out] blk pointer to the message 
	/// \param[out] blksize size of message blk in bytes
	/// \return true, if there is a message returned to be sent, false if not or an error occurred
	virtual bool fetchMessage( const char*& blk, std::size_t& blksize)=0;
};
}//namespace
#endif

