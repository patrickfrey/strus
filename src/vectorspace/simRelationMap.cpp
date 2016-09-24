/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for storing similarity relations
#include "simRelationMap.hpp"
#include <limits>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <set>

#define STRUS_LOWLEVEL_DEBUG

using namespace strus;

std::string SimRelationMap::tostring() const
{
	std::ostringstream buf;
	buf << m_mat << std::endl;
	return buf.str();
}

