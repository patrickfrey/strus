/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_TERM_CONFIG_HPP_INCLUDED
#define _STRUS_TERM_CONFIG_HPP_INCLUDED
#include <string>

namespace strus {

struct TermConfig
{
	TermConfig( const TermConfig& o)
		:set(o.set),type(o.type),value(o.value){}
	TermConfig( const std::string& s, const std::string& t, const std::string& v)
		:set(s),type(t),value(v){}

	std::string set;	///< term set name
	std::string type;	///< term type name
	std::string value;	///< term value
};

}
#endif

