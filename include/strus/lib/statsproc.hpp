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
#include <string>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class StatisticsProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Create an interface for processing global statistics
/// \param[in] nofBlocks number of blocks in the map
/// \param[in] msgChunkSize size of a message blob for transmitting statistics
/// \param[in] errorhnd error buffer interface
StatisticsProcessorInterface* createStandardStatisticsProcessor( int nofBlocks, int msgChunkSize, ErrorBufferInterface* errorhnd);

}//namespace
#endif

