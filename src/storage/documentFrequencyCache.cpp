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
#include "documentFrequencyCache.hpp"
#include <cstdlib>

using namespace strus;

void DocumentFrequencyCache::doIncrement( const Batch::Increment& incr)
{
	if (!incr.typeno || incr.typeno > MaxNofTermTypes) throw std::runtime_error( "term type number out of range for document frequency cache");
	if (!incr.termno) throw std::runtime_error( "term value number 0 passed to document frequency cache");

	std::size_t typeidx = incr.typeno-1;
	std::size_t termidx = incr.termno-1;

	CounterArrayRef dfar = m_ar[ typeidx];
	if (termidx >= dfar->size())
	{
		dfar.reset( new CounterArray( *dfar, termidx+1));
	}
	(*dfar)[ termidx] += incr.value;
}

void DocumentFrequencyCache::doRevertIncrement( const Batch::Increment& incr)
{
	std::size_t typeidx = incr.typeno-1;
	std::size_t termidx = incr.termno-1;

	CounterArrayRef dfar = m_ar[ typeidx];
	(*dfar)[ termidx] -= incr.value;
}

void DocumentFrequencyCache::writeBatch( const Batch& batch)
{
	boost::mutex::scoped_lock lock( m_mutex);
	Batch::const_iterator bi = batch.begin(), be = batch.end();
	try
	{
		for (; bi != be; ++bi)
		{
			doIncrement( *bi);
		}
	}
	catch (const std::runtime_error& err)
	{
		be = bi; bi = batch.begin();
		for (; bi != be; ++bi)
		{
			doRevertIncrement( *bi);
		}
		throw err;
	}
	catch (const std::bad_alloc& err)
	{
		be = bi; bi = batch.begin();
		for (; bi != be; ++bi)
		{
			doRevertIncrement( *bi);
		}
		throw err;
	}
}

GlobalCounter DocumentFrequencyCache::getValue( const Index& typeno, const Index& termno) const
{
	if (!typeno || typeno > MaxNofTermTypes) throw std::runtime_error( "term type number out of range for document frequency cache");
	if (!termno) throw std::runtime_error( "term value number 0 passed to document frequency cache");

	std::size_t typeidx = typeno-1;
	std::size_t termidx = termno-1;

	CounterArrayRef dfar = m_ar[ typeidx];
	if (dfar->size() <= termidx) return 0;
	return (*dfar)[ termidx];
}

void DocumentFrequencyCache::clear()
{
	std::size_t ti = 0, te = MaxNofTermTypes;
	for (; ti != te; te++)
	{
		m_ar[ ti].reset();
	}
}

