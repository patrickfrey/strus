/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Some basic text normalization for collectors of text from the forward index
#ifndef _STRUS_QUERYPROC_TEXT_COLLECTOR_NORMLIZER_HPP_INCLUDED
#define _STRUS_QUERYPROC_TEXT_COLLECTOR_NORMLIZER_HPP_INCLUDED
#include <string>

namespace strus {

/// \brief Replace some unicode characters in a text by spaces
/// \note Useful to strip some punctuation characters away from text in the forward index without sophisticated configuration
/// \param[in] text text to process
/// \param[in] stripchrs list of unicode characters to replace by spaces
std::string stripForwardIndexText( const std::string& text, const std::string& stripchrs);

}//namespace
#endif

