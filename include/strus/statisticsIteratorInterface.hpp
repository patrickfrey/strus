/*
 * Copyright (c) 2014,2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a iterator on messages with statistics
/// \file statisticsIteratorInterface.hpp
#ifndef _STRUS_STATISTICS_ITERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_STATISTICS_ITERATOR_INTERFACE_HPP_INCLUDED
#include "strus/statisticsMessage.hpp"
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
	/// \return returned message is empty if there is no chunk of a message left or an error occurred
	virtual StatisticsMessage getNext()=0;
};

}//namespace
#endif

