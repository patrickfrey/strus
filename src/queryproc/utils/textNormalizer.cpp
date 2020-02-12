/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Some basic text normalization for collectors of text from the forward index
#include "textNormalizer.hpp"
#include "strus/base/utf8.hpp"

using namespace strus;

std::string strus::stripForwardIndexText( const std::string& text, const std::string& stripchrs)
{
	std::string rt;
	char const* ti = text.c_str();
	while (*ti)
	{
		unsigned char len = strus::utf8charlen( *ti);
		char const* si = std::strchr( stripchrs.c_str(), *ti);
		if (len <= 1)
		{
			if (!si)
			{
				if (*ti == ' ')
				{
					if (rt.empty() || rt[ rt.size()-1] != ' ')
					{
						rt.push_back( ' ');
					}
				}
				else
				{
					rt.push_back( *ti);
				}
			}
			else if (rt.empty() || rt[ rt.size()-1] != ' ')
			{
				rt.push_back( ' ');
			}
			++ti;
		}
		else
		{
			while (si && 0!=std::memcmp( si, ti, len))
			{
				si = std::strchr( si + len, *ti);
			}
			if (si)
			{
				if (rt.empty() || rt[ rt.size()-1] != ' ')
				{
					rt.push_back( ' ');
				}
			}
			else
			{
				rt.append( ti, len);
			}
			ti += len;
		}
	}
	if (!rt.empty() && rt[ rt.size()-1] == ' ')
	{
		rt.resize( rt.size() -1);
	}
	return rt;
}



