/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Some utility functions that are centralised to control dependencies to boost
#ifndef _STRUS_UTILS_HPP_INCLUDED
#define _STRUS_UTILS_HPP_INCLUDED
#include <boost/dynamic_bitset.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <stdint.h>			///... boost atomic needs this
#include <boost/atomic/atomic.hpp>
#include <boost/static_assert.hpp>
#include <boost/unordered_map.hpp>

namespace strus {
namespace utils {

std::string tolower( const char* val);
std::string tolower( const std::string& val);
std::string trim( const std::string& val);
bool caseInsensitiveEquals( const std::string& val1, const std::string& val2);
bool caseInsensitiveStartsWith( const std::string& val, const std::string& prefix);
int toint( const std::string& val);
std::string tostring( int val);

void aligned_free( void *ptr);
void* aligned_malloc( std::size_t size, std::size_t alignment);

class BitSet
{
public:
	explicit BitSet( std::size_t size_)
		:m_impl(size_),m_size(size_){}
	BitSet( const BitSet& o)
		:m_impl(o.m_impl),m_size(o.m_size){}

	bool empty() const
	{
		return m_impl.empty();
	}
	void reset()
	{
		m_impl.reset();
	}
	void set( std::size_t idx)
	{
		m_impl.set( idx, true);
	}
	void unset( std::size_t idx)
	{
		m_impl.set( idx, false);
	}
	int first() const
	{
		int rt = m_impl.find_first();
		if (rt >= (int)m_size) return -1;
		return rt;
	}
	int next( std::size_t idx) const
	{
		int rt = m_impl.find_next( idx);
		if (rt >= (int)m_size) return -1;
		return rt;
	}
	BitSet& operator <<= (std::size_t n)
	{
		m_impl <<= n;
		return *this;
	}
	BitSet& operator >>= (std::size_t n)
	{
		m_impl >>= n;
		return *this;
	}
	void insert( std::size_t idx)
	{
		boost::dynamic_bitset<> left = (m_impl >> idx) << idx;
		boost::dynamic_bitset<> right = m_impl ^ left;
		m_impl = (left << 1) | right;
		m_impl.set( idx);
	}

private:
	boost::dynamic_bitset<> m_impl;
	std::size_t m_size;
};


template<typename Key, typename Elem>
class UnorderedMap
	:public boost::unordered_map<Key,Elem>
{
public:
	UnorderedMap(){}
	UnorderedMap( const UnorderedMap& o)
		:boost::unordered_map<Key,Elem>(){}
};

class DynamicBitset
{
public:
	DynamicBitset( std::size_t size)
		:m_impl(size){}

	bool test( std::size_t idx)
	{
		return m_impl.test( idx);
	}

	void set( std::size_t idx)
	{
		m_impl[ idx] = true;
	}
	
private:
	boost::dynamic_bitset<> m_impl;
};

typedef boost::mutex Mutex;
typedef boost::mutex::scoped_lock ScopedLock;
typedef boost::unique_lock<boost::mutex> UniqueLock;

template <class X>
class SharedPtr
	:public boost::shared_ptr<X>
{
public:
	SharedPtr( X* ptr)
		:boost::shared_ptr<X>(ptr){}
	SharedPtr( const SharedPtr& o)
		:boost::shared_ptr<X>(o){}
	SharedPtr()
		:boost::shared_ptr<X>(){}
};

template <class X>
class ScopedArray
	:public boost::scoped_array<X>
{
public:
	ScopedArray( X* ptr)
		:boost::scoped_array<X>(ptr){}
	ScopedArray( const ScopedArray& o)
		:boost::scoped_array<X>(o){}
	ScopedArray()
		:boost::scoped_array<X>(){}
};


template <typename IntegralCounterType>
class AtomicCounter
	:public boost::atomic<IntegralCounterType>
{
public:
	///\brief Constructor
	AtomicCounter( IntegralCounterType initialValue_=0)
		:boost::atomic<IntegralCounterType>(initialValue_)
	{}

	///\brief Increment of the counter
	void increment( IntegralCounterType val = 1)
	{
		boost::atomic<IntegralCounterType>::fetch_add( val, boost::memory_order_acquire);
	}

	///\brief Decrement of the counter
	void decrement( IntegralCounterType val = 1)
	{
		boost::atomic<IntegralCounterType>::fetch_sub( val, boost::memory_order_acquire);
	}

	///\brief Increment of the counter
	///\return the new value of the counter after the increment operation
	IntegralCounterType allocIncrement( IntegralCounterType val = 1)
	{
		return boost::atomic<IntegralCounterType>::fetch_add( val, boost::memory_order_acquire);
	}

	///\brief Increment of the counter
	///\return the current value of the counter
	IntegralCounterType value() const
	{
		return boost::atomic<IntegralCounterType>::load( boost::memory_order_acquire);
	}

	///\brief Initialization of the counter
	///\param[in] val the value of the counter
	void set( const IntegralCounterType& val)
	{
		boost::atomic<IntegralCounterType>::store( val);
	}

	///\brief Compare current value with 'testval', change it to 'newval' if matches
	///\param[in] testval the value of the counter
	///\param[in] newval the value of the counter
	///\return true on success
	bool test_and_set( IntegralCounterType testval, IntegralCounterType newval)
	{
		return boost::atomic<IntegralCounterType>::compare_exchange_strong( testval, newval, boost::memory_order_acquire, boost::memory_order_acquire);
	}
};


template <typename ValueType>
class AtomicSlot
	:public boost::atomic<ValueType>
{
public:
	///\brief Constructor
	AtomicSlot()
		:boost::atomic<ValueType>(0)
	{}

	///\brief Occupy the slot, if not yet occupied yet.
	///\param[in] val value to test
	///\return true on success
	bool set( const ValueType& val)
	{
		ValueType prev_val = 0;
		return boost::atomic<ValueType>::compare_exchange_strong( prev_val, val, boost::memory_order_acquire);
	}

	///\brief Reset the slot
	void reset()
	{
		ValueType val = 0;
		(void)boost::atomic<ValueType>::exchange( val, boost::memory_order_acquire);
	}

	///\brief Test if the current value equals the argument
	///\param[in] val value to test
	///\return true, if yes
	bool test( const ValueType& val) const
	{
		return boost::atomic<bool>::load( boost::memory_order_acquire) == val;
	}
};


class AtomicFlag
	:public boost::atomic<bool>
{
public:
	///\brief Constructor
	AtomicFlag( bool initialValue_=false)
		:boost::atomic<bool>(initialValue_)
	{}

	///\brief Set the flag, if the new value changes the current value.
	///\return true on success
	bool set( bool val)
	{
		bool prev_val = !val;
		return boost::atomic<bool>::compare_exchange_strong( prev_val, val, boost::memory_order_acquire);
	}

	///\brief Evaluate the current value
	///\return the current value
	bool test()
	{
		return boost::atomic<bool>::load( boost::memory_order_acquire);
	}
};

}} //namespace
#endif


