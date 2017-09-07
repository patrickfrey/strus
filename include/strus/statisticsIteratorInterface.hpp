/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a iterator on messages with statistics
/// \file statisticsIteratorInterface.hpp
#ifndef _STRUS_STATISTICS_ITERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_STATISTICS_ITERATOR_INTERFACE_HPP_INCLUDED
#include <cstddef>

namespace strus
{

/// \brief Interface for an iterator on statistics
class StatisticsIteratorInterface
{

public:
	/// \brief Destructor
	virtual ~StatisticsIteratorInterface(){}

	/// \brief Fetches the next statistics message
	/// \param[in] msg pointer to message
	/// \param[in] msgsize size of msg blob in bytes
	/// \return false if there is no chunk of a message left or an error occurred
	virtual bool getNext( const void*& msg, std::size_t& msgsize)=0;
};

}//namespace
#endif

