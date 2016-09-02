/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Similarity hash structure
#ifndef _STRUS_VECTOR_SPACE_MODEL_SIMHASH_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_SIMHASH_HPP_INCLUDED
#include "strus/base/stdint.h"
#include <vector>
#include <string>

namespace strus {

struct Functor_OR  {static uint64_t call( uint64_t aa, uint64_t bb)	{return aa|bb;}};
struct Functor_AND {static uint64_t call( uint64_t aa, uint64_t bb)	{return aa&bb;}};
struct Functor_XOR {static uint64_t call( uint64_t aa, uint64_t bb)	{return aa^bb;}};
struct Functor_INV {static uint64_t call( uint64_t aa)			{return !aa;}};

class SimHash
{
public:
	SimHash()
		:m_ar(){}
	SimHash( const SimHash& o)
		:m_ar(o.m_ar){}
	SimHash( const std::vector<bool>& bv);

	unsigned int dist( const SimHash& o) const;

	template <class Functor>
	SimHash BINOP( const SimHash& o) const
	{
		SimHash rt;
		std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end(), oi = o.m_ar.begin(), oe = o.m_ar.end();
		for (; ai != ae && oi != oe; ++ai, ++oi) rt.m_ar.push_back( Functor::call( *ai, *oi));
		for (; ai != ae; ++ai) rt.m_ar.push_back( Functor::call( *ai, 0));
		for (; oi != oe; ++oi) rt.m_ar.push_back( Functor::call( 0, *oi));
		return rt;
	}
	template <class Functor>
	SimHash& BINOP_ASSIGN( const SimHash& o)
	{
		std::vector<uint64_t>::iterator ai = m_ar.begin(), ae = m_ar.end();
		std::vector<uint64_t>::const_iterator oi = o.m_ar.begin(), oe = o.m_ar.end();
		for (; ai != ae && oi != oe; ++ai, ++oi) *ai = Functor::call( *ai, *oi);
		for (; ai != ae; ++ai) *ai = Functor::call( *ai, *oi);
		for (; oi != oe; ++oi) m_ar.push_back( Functor::call( 0, *oi));
		return *this;
	}
	template <class Functor>
	SimHash UNOP() const
	{
		SimHash rt;
		std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
		for (; ai != ae; ++ai) rt.m_ar.push_back( Functor::call( *ai));
		return rt;
	}

	SimHash operator ^ ( const SimHash& o) const	{return BINOP<Functor_XOR>( o);}
	SimHash operator & ( const SimHash& o) const	{return BINOP<Functor_AND>( o);}
	SimHash operator | ( const SimHash& o) const	{return BINOP<Functor_OR>( o);}
	SimHash operator ~ () const			{return UNOP<Functor_INV>();}
	SimHash& operator ^= ( const SimHash& o)	{return BINOP_ASSIGN<Functor_XOR>( o);}
	SimHash& operator &= ( const SimHash& o)	{return BINOP_ASSIGN<Functor_AND>( o);}
	SimHash& operator |= ( const SimHash& o)	{return BINOP_ASSIGN<Functor_OR>( o);}

	std::string tostring() const;

private:
	enum {NofElementBits=64};
	std::vector<uint64_t> m_ar;
};

}//namespace
#endif


