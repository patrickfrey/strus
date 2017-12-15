/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "documentFrequencyCache.hpp"
#include "private/internationalization.hpp"
#include "strus/base/thread.hpp"
#include <cstdlib>

using namespace strus;

void DocumentFrequencyCache::doIncrement( const Batch::Increment& incr)
{
	if (!incr.typeno || incr.typeno > MaxNofTermTypes) throw strus::runtime_error( "%s", _TXT( "term type number out of range for document frequency cache"));
	if (!incr.termno) throw strus::runtime_error( "%s", _TXT( "term value number 0 passed to document frequency cache"));

	std::size_t typeidx = incr.typeno-1;
	std::size_t termidx = incr.termno-1;

	CounterArrayRef& dfar = m_ar[ typeidx];
	if (!dfar.get())
	{
		dfar.reset( new CounterArray( termidx+1));
	}
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

	CounterArrayRef& dfar = m_ar[ typeidx];
	(*dfar)[ termidx] -= incr.value;
}

void DocumentFrequencyCache::writeBatch( const Batch& batch)
{
	strus::scoped_lock lock( m_mutex);
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
		throw strus::runtime_error(_TXT("error in document frequency cache transaction: %s"), err.what());
	}
	catch (const std::bad_alloc& err)
	{
		be = bi; bi = batch.begin();
		for (; bi != be; ++bi)
		{
			doRevertIncrement( *bi);
		}
		throw strus::runtime_error( "%s", _TXT("out of memory in document frequency cache transaction"));
	}
}

Index DocumentFrequencyCache::getValue( const Index& typeno, const Index& termno) const
{
	if (!typeno || typeno > MaxNofTermTypes) throw strus::runtime_error( "%s", _TXT( "term type number out of range for document frequency cache"));
	if (!termno) throw strus::runtime_error( "%s", _TXT( "term value number 0 passed to document frequency cache"));

	std::size_t typeidx = typeno-1;
	std::size_t termidx = termno-1;

	CounterArrayRef dfar = m_ar[ typeidx];
	if (!dfar.get() || dfar->size() <= termidx) return 0;
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

