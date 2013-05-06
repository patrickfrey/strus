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
#ifndef _STRUS_KCF_PERSISTENT_LIST_HPP_INCLUDED
#define _STRUS_KCF_PERSISTENT_LIST_HPP_INCLUDED
#include "strus/position.hpp"
#include "blktable.hpp"
#include <cstdio>
#include <string>
#include <cstring>

namespace strus
{

///\class PersistentListBase
///\brief Base (non intrusive) implementation of a persistent list of POD types
class PersistentListBase
	:public BlockTable
{
public:
	enum {NofBlockElements=127};
	PersistentListBase( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t elementsize_, bool writemode_=false);

	virtual ~PersistentListBase();

	static void create( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t elementsize_);

	void open();

	void push_back( const void* element);

	void reset();

public:
	class iterator
	{
	public:
		iterator( PersistentListBase* ref_);
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
		PersistentListBase* m_ref;
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


///\class PersistentList
///\tparam Element
///\brief Implementation of a persistent list of POD types
template <class Element>
class PersistentList
	:public PersistentListBase
{
public:
	PersistentList( const std::string& type_, const std::string& name_, const std::string& path_, bool writemode_=false)
		:PersistentListBase( type_, name_, path_, sizeof(Element), writemode_){}

	static void create( const std::string& type_, const std::string& name_, const std::string& path_)
		{PersistentListBase::create( type_, name_, path_, sizeof(Element));}

	void push_back( const Element& element)
		{PersistentListBase::push_back( &element);}


	class iterator
		:public PersistentListBase::iterator
	{
	public:
		iterator( PersistentList* ref_)
			:PersistentListBase::iterator( ref_){}
		iterator()
			:PersistentListBase::iterator(){}
		iterator( const iterator& o)
			:PersistentListBase::iterator(o){}

		const Element& operator*()
			{return *(const Element*)PersistentListBase::iterator::operator*();}
	};
};

} //namespace
#endif


