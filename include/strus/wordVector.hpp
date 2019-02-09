/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Vector type used for word embedding vector calculations
/// \file "wordVector.hpp"
#ifndef _STRUS_WORD_VECTOR_INCLUDED
#define _STRUS_WORD_VECTOR_INCLUDED
#include <vector>
#include <string>
#include <cstdio>
#include <limits>

namespace strus {

class WordVector :public std::vector<float>
{
public:
	WordVector() :std::vector<float>(){}
	WordVector( const std::vector<float>& o) :std::vector<float>(o){}

	std::string tostring( const char* separator, std::size_t maxNofElements=std::numeric_limits<std::size_t>::max()) const
	{
		std::string rt;
		int ei = 0, ee = size() > maxNofElements ? maxNofElements : size();
		for (; ei != ee; ++ei)
		{
			if (ei) rt.append(separator);
			char buf[ 32];
			std::snprintf( buf, sizeof(buf), "%.5f", operator[]( ei));
			rt.append( buf);
		}
		return rt;
	}
};

}//namespace
#endif

