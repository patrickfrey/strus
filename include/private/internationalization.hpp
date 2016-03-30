/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_INTERNATIONALIZATION_HPP_INCLUDED
#define _STRUS_INTERNATIONALIZATION_HPP_INCLUDED
#include <libintl.h>
#include <stdexcept>

#define _TXT(STRING) gettext(STRING)

namespace strus
{

/// \brief Substitute for std::runtime_error with arguments
/// \param[in] msg c printf format string
/// \param[in] nofargs number of arguments passed to be substituted in the format string
std::runtime_error runtime_error( const char* format, ...);

/// \brief Substitute for std::logic_error with arguments
/// \param[in] msg c printf format string
/// \param[in] nofargs number of arguments passed to be substituted in the format string
std::logic_error logic_error( const char* format, ...);

/// \brief Declare the message domain used by this package for the exception constructors declared in this module for gettext
void initMessageTextDomain();

}//namespace
#endif

