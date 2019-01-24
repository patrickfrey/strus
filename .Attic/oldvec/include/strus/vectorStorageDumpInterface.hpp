/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for dumping the contents of a vector storage
/// \file vectorStorageDumpInterface.hpp
#ifndef _STRUS_VECTOR_STORGE_DUMP_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_STORGE_DUMP_INTERFACE_HPP_INCLUDED
#include "strus/databaseOptions.hpp"
#include "strus/index.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Interface for fetching the dump of a strus vector storage
class VectorStorageDumpInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageDumpInterface(){}

	/// \brief Fetch the next chunk of the dump
	/// \param[out] chunk pointer to the chunk
	/// \param[out] chunksize size of the chunk in bytes
	/// \return true, if there are more chunks left to return, false if EOF has been reached and no chunk is returned
	virtual bool nextChunk( const char*& chunk, std::size_t& chunksize)=0;
};
}//namespace
#endif

