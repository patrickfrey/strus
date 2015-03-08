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
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>

namespace strus {
namespace utils {

std::string tolower( const std::string& val);
std::string trim( const std::string& val);
bool caseInsensitiveEquals( const std::string& val1, const std::string& val2);
bool caseInsensitiveStartsWith( const std::string& val, const std::string& prefix);
int toint( const std::string& val);
std::string tostring( int val);

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

}} //namespace
#endif


