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
	WordVector( std::size_t nn, float initval) :std::vector<float>(nn,initval){}
	WordVector() :std::vector<float>(){}
	WordVector( const std::vector<float>& o) :std::vector<float>(o){}
#if __cplusplus >= 201103L
	WordVector( WordVector&& o)
		:std::vector<float>(o){}
	WordVector& operator= ( WordVector&& o)
		{std::vector<float>(o)::operator =(o); return *this;}
#endif

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

	WordVector operator+( const WordVector& o) const
	{
		WordVector rt;
		std::size_t ii = 0, nn = size() > o.size() ? o.size() : size();
		rt.reserve( nn);
		std::vector<float>::const_iterator wi = begin(), oi = o.begin();
		for (; ii < nn; ++ii,++wi,++oi)
		{
			rt.push_back( *wi + *oi);
		}
		return rt;
	}

	WordVector& operator+=( const WordVector& o)
	{
		std::size_t ii = 0, nn = size() > o.size() ? o.size() : size();
		std::vector<float>::iterator wi = begin();
		std::vector<float>::const_iterator oi = o.begin();
		for (; ii < nn; ++ii,++wi,++oi)
		{
			 *wi += *oi;
		}
		return *this;
	}

	WordVector& operator*=( double scalar)
	{
		std::vector<float>::iterator wi = begin(), we = end();
		for (; wi != we; ++wi)
		{
			 *wi *= scalar;
		}
		return *this;
	}
};

}//namespace
#endif

