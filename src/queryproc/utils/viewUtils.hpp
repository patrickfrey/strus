/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Utilities for introspection
#ifndef _STRUS_QUERYPROC_VIEW_UTILS_HPP_INCLUDED
#define _STRUS_QUERYPROC_VIEW_UTILS_HPP_INCLUDED

namespace strus {

template<typename Value>
static StructView paramView( const char* name, const Value& value)
{
	return StructView()(name,value);
}

}//namespace
#endif

