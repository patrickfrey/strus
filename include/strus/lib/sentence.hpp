/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus query sentence parser
/// \file storage.hpp
#ifndef _STRUS_SENTENCE_LIB_HPP_INCLUDED
#define _STRUS_SENTENCE_LIB_HPP_INCLUDED
#include <string>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class SentenceAnalyzerInstanceInterface;

SentenceAnalyzerInstanceInterface* createSentenceAnalyzerInstance_std( ErrorBufferInterface* errorhnd);

}//namespace
#endif

