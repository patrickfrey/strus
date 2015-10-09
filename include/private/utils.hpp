/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
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


class ThreadId
{
public:
	typedef boost::thread::id Type;
	static Type get()
	{
		return boost::this_thread::get_id();
	}
};

#define STRUS_STATIC_ASSERT( cond ) BOOST_STATIC_ASSERT((cond))
#define CACHELINE_SIZE 64

}} //namespace
#endif


