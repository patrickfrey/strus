/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
/// \brief For not getting a dependency to boost just because of shared_ptr, we provide a substitute implementation here

#ifndef _STRUS_SHARED_PTR_HPP_INCLUDED
#define _STRUS_SHARED_PTR_HPP_INCLUDED
#include "strus/iteratorInterface.hpp"
#include <string>
#include <boost/shared_ptr.hpp>

namespace strus
{

template<class T>
class shared_ptr
	:public boost::shared_ptr<T>
{
public:
	shared_ptr()
		:boost::shared_ptr() {}
	shared_ptr( T* s)
		:boost::shared_ptr(s) {}

	shared_ptr( const shared_ptr& o)
		:boost::shared_ptr(o) {}

	shared_ptr& operator=(const shared_ptr& o) 
	{
		boost::shared_ptr<T>::operator=(o);
		return *this;
	}
};

}//namespace
#endif

