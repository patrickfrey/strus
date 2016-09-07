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
struct Functor_INV {static uint64_t call( uint64_t aa)			{return ~aa;}};

/// \brief Structure for the similarity fingerprint used
class SimHash
{
public:
	SimHash()
		:m_ar(),m_size(0){}
	SimHash( const SimHash& o)
		:m_ar(o.m_ar),m_size(o.m_size){}
	SimHash( const std::vector<bool>& bv);
	SimHash( std::size_t size_, bool initval);

	/// \brief Get element value by index
	bool operator[]( std::size_t idx) const;
	/// \brief Set element with index to value
	void set( std::size_t idx, bool value);
	/// \brief Calculate distance (bits with different value)
	unsigned int dist( const SimHash& o) const;
	/// \brief Calculate the number of bits set to 1
	unsigned int count() const;
	/// \brief Test if the distance is smaller than a given dist
	bool near( const SimHash& o, unsigned int dist) const;
	/// \brief Get all indices of elements set to 1 or 0 (defined by parameter)
	std::vector<std::size_t> indices( bool what) const;

private:
	template <class Functor>
	SimHash BINOP( const SimHash& o) const
	{
		SimHash rt;
		std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end(), oi = o.m_ar.begin(), oe = o.m_ar.end();
		for (; ai != ae && oi != oe; ++ai, ++oi) rt.m_ar.push_back( Functor::call( *ai, *oi));
		for (; ai != ae; ++ai) rt.m_ar.push_back( Functor::call( *ai, 0));
		for (; oi != oe; ++oi) rt.m_ar.push_back( Functor::call( 0, *oi));
		rt.m_size = (o.m_size > m_size) ? o.m_size : m_size;
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
		if (o.m_size > m_size) m_size = o.m_size;
		return *this;
	}
	template <class Functor>
	SimHash UNOP() const
	{
		SimHash rt;
		rt.m_size = m_size;
		std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
		for (; ai != ae; ++ai) rt.m_ar.push_back( Functor::call( *ai));
		return rt;
	}
public:

	/// \brief Binary XOR
	SimHash operator ^ ( const SimHash& o) const	{return BINOP<Functor_XOR>( o);}
	/// \brief Binary AND
	SimHash operator & ( const SimHash& o) const	{return BINOP<Functor_AND>( o);}
	/// \brief Binary OR
	SimHash operator | ( const SimHash& o) const	{return BINOP<Functor_OR>( o);}
	/// \brief Unary negation
	SimHash operator ~ () const			{return UNOP<Functor_INV>();}
	/// \brief Assignment XOR
	SimHash& operator ^= ( const SimHash& o)	{return BINOP_ASSIGN<Functor_XOR>( o);}
	/// \brief Assignment AND
	SimHash& operator &= ( const SimHash& o)	{return BINOP_ASSIGN<Functor_AND>( o);}
	/// \brief Assignment OR
	SimHash& operator |= ( const SimHash& o)	{return BINOP_ASSIGN<Functor_OR>( o);}

	/// \brief Get the bit field as string of "0" and "1"
	std::string tostring() const;
	/// \brief Number of bits represented
	std::size_t size() const			{return m_size;}

private:
	enum {NofElementBits=64};
	std::vector<uint64_t> m_ar;
	std::size_t m_size;
};

}//namespace
#endif


