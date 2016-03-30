/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus storage library
/// \file storage.hpp
#ifndef _STRUS_STORAGE_LIB_HPP_INCLUDED
#define _STRUS_STORAGE_LIB_HPP_INCLUDED

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;

class StorageInterface;

StorageInterface* createStorage( ErrorBufferInterface* errorhnd);

}//namespace
#endif

