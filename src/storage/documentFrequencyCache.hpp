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
#ifndef _STRUS_LVDB_DOCUMENT_FREQUENCY_CACHE_HPP_INCLUDED
#define _STRUS_LVDB_DOCUMENT_FREQUENCY_CACHE_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/utils.hpp"
#include "strus/private/internationalization.hpp"
#include <cstring>
#include <stdexcept>
#include <vector>

namespace strus {

class DocumentFrequencyCache
{
public:
	class Batch;

	DocumentFrequencyCache()
	{}
	~DocumentFrequencyCache()
	{}

	void writeBatch( const Batch& batch);

	GlobalCounter getValue( const Index& typeno, const Index& termno) const;

	void clear();

public:
	class Batch
	{
	public:
		class Increment;

		void put( const Index& typeno, const Index termno, const GlobalCounter& increment)
		{
			m_ar.push_back( Increment( typeno, termno, increment));
		}
		void put( const Increment& o)
		{
			m_ar.push_back( o);
		}
		typedef std::vector<Increment>::const_iterator const_iterator;

		const_iterator begin() const	{return m_ar.begin();}
		const_iterator end() const	{return m_ar.end();}

	public:
		class Increment
		{
		public:
			Increment( const Index& typeno_, const Index termno_, const GlobalCounter& value_)
				:typeno(typeno_),termno(termno_),value(value_){}
			Increment( const Increment& o)
				:typeno(o.typeno),termno(o.termno),value(o.value){}

			Index typeno;
			Index termno;
			GlobalCounter value;
		};

	private:
		std::vector<Increment> m_ar;
	};

private:
	enum {
		MaxNofTermTypes=256,		///< maximum number of term types
		InitArraySize=256		///< initial size of the cache for one term type
	};

	class CounterArray
	{
	public:
		~CounterArray()
		{
			if (m_ar) std::free( m_ar);
		}

		
		explicit CounterArray( std::size_t size_)
			:m_size(getAllocSize(size_))
		{
			m_ar = (GlobalCounter*) std::calloc( m_size, sizeof(GlobalCounter));
			if (!m_ar) throw std::bad_alloc();
		}

		explicit CounterArray( const CounterArray& o, std::size_t size_)
			:m_ar(0),m_size(getAllocSize(size_))
		{
			std::size_t copy_size = (size_ < o.m_size)?size_:o.m_size;
			if (m_size * sizeof(GlobalCounter) < m_size) throw std::bad_alloc();
			m_ar = (GlobalCounter*)std::malloc( m_size * sizeof(GlobalCounter));
			if (!m_ar) throw std::bad_alloc();

			std::memcpy( m_ar, o.m_ar, copy_size * sizeof(GlobalCounter));
			std::memset( m_ar + copy_size, 0, m_size - copy_size);
		}

		const GlobalCounter& operator[]( std::size_t idx) const		{if (idx >= m_size) throw strus::runtime_error( _TXT( "array bound read (document frequency cache)")); return m_ar[idx];}
		GlobalCounter& operator[]( std::size_t idx)			{if (idx >= m_size) throw strus::runtime_error( _TXT( "array bound write (document frequency cache)")); return m_ar[idx];}

		std::size_t size() const					{return m_size;}

	private:
		static std::size_t getAllocSize( std::size_t neededSize=InitArraySize)
		{
			std::size_t mm = InitArraySize;
			while (mm && mm < neededSize)
			{
				mm *= 2;
			}
			if (!mm) throw std::bad_alloc();
			return mm;
		}

	private:
		GlobalCounter* m_ar;
		std::size_t m_size;
	};

	typedef utils::SharedPtr<CounterArray> CounterArrayRef;

	void doIncrement( const Batch::Increment& incr);
	void doRevertIncrement( const Batch::Increment& incr);

private:
	utils::Mutex m_mutex;
	CounterArrayRef m_ar[ MaxNofTermTypes];
};

}//namespace
#endif


