/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus statsproc library
/// \file "statsproc.hpp"
#ifndef _STRUS_STORAGE_STD_STATISTICS_PROCESSOR_LIB_HPP_INCLUDED
#define _STRUS_STORAGE_STD_STATISTICS_PROCESSOR_LIB_HPP_INCLUDED

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class StatisticsProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

StatisticsProcessorInterface* createStatisticsProcessor( ErrorBufferInterface* errorhnd);

}//namespace
#endif

