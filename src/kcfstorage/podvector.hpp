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
#ifndef _STRUS_KCF_POD_VECTOR_HPP_INCLUDED
#define _STRUS_KCF_POD_VECTOR_HPP_INCLUDED
#include "strus/position.hpp"
#include "blocktable.hpp"
#include <cstdio>
#include <string>
#include <cstring>

namespace strus
{

///\class PodVectorBase
///\brief Base (non intrusive) implementation of a persistent list of POD types
class PodVectorBase
	:public BlockTable
{
public:
	enum {NofBlockElements=127};
	PodVectorBase( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t elementsize_, bool writemode_=false);

	virtual ~PodVectorBase();

	static void create( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t elementsize_);

	void open();

	void set( const Index& idx, const void* element);
	void get( const Index& idx, void* element);

	Index push_back( const void* element);
	void reset();

public:
	class iterator
	{
	public:
		iterator( PodVectorBase* ref_);
		iterator();
		iterator( const iterator& o);
		iterator& operator++();
		const char* operator*();

		bool operator==( const iterator& o) const	{return isequal(o);}
		bool operator!=( const iterator& o) const	{return !isequal(o);}

	private:
		bool isequal( const iterator& o) const;
		void readNextBlock();

	private:
		PodVectorBase* m_ref;
		char* m_cur;
		Index m_curidx;
		std::size_t m_curpos;
		std::size_t m_cursize;
	};

	iterator begin()
	{
		return iterator(this);
	}
	iterator end()
	{
		return iterator();
	}

private:
	std::size_t blocksize() const;

private:
	friend class iterator;
	char* m_cur;
	Index m_lastidx;
	std::size_t m_elementsize;
};


///\class PodVector
///\tparam Element
///\brief Implementation of a persistent list of POD types
template <class Element>
class PodVector
	:public PodVectorBase
{
public:
	PodVector( const std::string& type_, const std::string& name_, const std::string& path_, bool writemode_=false)
		:PodVectorBase( type_, name_, path_, sizeof(Element), writemode_){}

	static void create( const std::string& type_, const std::string& name_, const std::string& path_)
		{PodVectorBase::create( type_, name_, path_, sizeof(Element));}

	Index push_back( const Element& element)
		{return PodVectorBase::push_back( &element);}

	void set( const Index& idx, const Element& element)
		{return PodVectorBase::set( idx, &element);}
	void get( const Index& idx, Element& element)
		{return PodVectorBase::get( idx, &element);}
	Element get( const Index& idx)
		{Element rt; PodVectorBase::get( idx, &rt); return rt;}

	class iterator
		:public PodVectorBase::iterator
	{
	public:
		iterator( PodVector* ref_)
			:PodVectorBase::iterator( ref_){}
		iterator()
			:PodVectorBase::iterator(){}
		iterator( const iterator& o)
			:PodVectorBase::iterator(o){}

		const Element& operator*()
			{return *(const Element*)PodVectorBase::iterator::operator*();}
	};
};

} //namespace
#endif


