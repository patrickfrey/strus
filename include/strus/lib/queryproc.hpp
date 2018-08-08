/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus query processor library (container for all types of functions needed for query evaluation)
/// \file queryproc.hpp
#ifndef _STRUS_QUERY_PROCESSOR_LIB_HPP_INCLUDED
#define _STRUS_QUERY_PROCESSOR_LIB_HPP_INCLUDED

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class FileLocatorInterface;

/// \brief Create a query processor with the functions and operators needed for query evaluation
/// \return the allocated processor
QueryProcessorInterface* createQueryProcessor( const FileLocatorInterface* filelocator, ErrorBufferInterface* errorhnd);

}//namespace
#endif

