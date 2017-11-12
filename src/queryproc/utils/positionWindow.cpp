/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "positionWindow.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <cstdio>
#include <cstring>

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG

#ifdef STRUS_LOWLEVEL_DEBUG
static printWindow( unsigned int nofelem, PostingIteratorInterface* itrar, Index* posar)
{
	unsigned int ii=0;
	for (; si!=se && ii<nofelem; ++ii)
	{
		if (ii) printf( " ");
		printf( "[%s %u]", itrar[ii]->featureid(), posar[ii]);
	}
}
#endif

PositionWindow::PositionWindow()
	:m_arsize(0)
	,m_range(0)
	,m_cardinality(0)
	,m_windowsize(0)
	,m_isnew_bitset(0)
	,m_evaluationType(MaxWin)
{}

PositionWindow::PositionWindow(
		PostingIteratorInterface** args,
		std::size_t nofargs,
		unsigned int range_,
		unsigned int cardinality_,
		Index firstpos_,
		EvaluationType evaluationType_)
	:m_isnew_bitset(0)
{
	init( args, nofargs, range_, cardinality_, firstpos_, evaluationType_);
}

void PositionWindow::init(
		PostingIteratorInterface** args,
		std::size_t nofargs,
		unsigned int range_,
		unsigned int cardinality_,
		Index firstpos_,
		EvaluationType evaluationType_)
{
	m_arsize = 0;
	m_range = range_;
	m_cardinality = (cardinality_>0?cardinality_:nofargs);
	m_windowsize = 0;
	m_isnew_bitset = strus::utils::BitSet( nofargs);
	m_evaluationType = evaluationType_;

	if (nofargs > MaxNofArguments)
	{
		throw strus::runtime_error(_TXT("too many arguments for position window (max %u): %u"),
						(unsigned int)MaxNofArguments, (unsigned int)nofargs);
	}
	if (nofargs == 0)
	{
		throw strus::runtime_error(_TXT("too few arguments for position window (min 1): %u"),
						(unsigned int)nofargs);
	}
	std::size_t ai = 0, ae = nofargs;
	for (; ai != ae; ++ai)
	{
		m_itrar[ ai] = args[ai];
		Index wpos = m_itrar[ ai] ? m_itrar[ ai]->skipPos( firstpos_) : 0;
		if (wpos)
		{
			// Insert element:
			std::size_t pi = 0, pe = m_arsize;
			for (; pi != pe && m_posar[pi] < wpos; ++pi){}
			std::memmove( m_posar+pi+1, m_posar+pi, (pe-pi)*sizeof(Index));
			std::memmove( m_window+pi+1, m_window+pi, (pe-pi)*sizeof(PostingIteratorInterface*));
			++m_arsize;
			m_posar[ pi] = wpos;
			m_window[ pi] = ai;
			m_isnew_bitset <<= 1;
			m_isnew_bitset.set(0);
		}
	}
	m_windowsize = (m_evaluationType == MinWin) ? getMinWinSize() : getMaxWinSize();
}

unsigned int PositionWindow::getMinWinSize()
{
	if (m_arsize < m_cardinality) return 0;
	unsigned int rt = m_posar[ m_cardinality-1] - m_posar[ 0];
	return rt <= m_range ? rt : 0;
}

unsigned int PositionWindow::getMaxWinSize()
{
	if (m_arsize < m_cardinality) return 0;
	unsigned int ai = m_cardinality-1;
	if (m_posar[ ai] - m_posar[ 0] > (Index)m_range) return 0;
	for (++ai; ai < m_arsize && m_posar[ ai] - m_posar[ 0] <= (Index)m_range; ++ai){}
	return ai;
}

bool PositionWindow::advance( const Index& advancepos)
{
	if (m_arsize < m_cardinality)
	{
		m_windowsize = 0;
		return false;
	}
	// Calculate how many positions we can skip with the 
	// first element without loosing any window of a size
	// within the defined proximity range:
	Index posdiff = m_posar[m_cardinality-1] - m_posar[0];
	Index skipsize = posdiff > (Index)m_range ? (posdiff - (Index)m_range) : 1;

	// Change the position of the first element in the sliding window by the calculated size:
	std::size_t idx = m_window[ 0];
	PostingIteratorInterface* itr = m_itrar[ idx];
	Index apos = m_posar[0] + skipsize;

	if (apos < advancepos)
	{
		apos = advancepos;
	}
	apos = itr ? itr->skipPos( apos) : 0;
	if (apos)
	{
		// Rearrange array to be sorted again by positions:
		std::size_t pi = 0, pe = m_arsize;
		for (++pi; pi != pe && m_posar[pi] < apos; ++pi)
		{
			m_posar[ pi-1] = m_posar[ pi];
			m_window[ pi-1] = m_window[ pi];
		}
		m_posar[ pi-1] = apos;
		m_window[ pi-1] = idx;

		m_isnew_bitset >>= 1;					//... remove first bit
		m_isnew_bitset.insert( pi-1);				//... insert new position bit
	}
	else
	{
		// Remove first element:
		m_isnew_bitset >>= 1;					//... remove first bit
		--m_arsize;
		if (m_arsize)
		{
			std::memmove( &m_posar[0], &m_posar[1], m_arsize * sizeof(*m_posar));
			std::memmove( &m_window[0], &m_window[1], m_arsize * sizeof(*m_window));
		}
	}
	// Return, if there is valid window left:
	m_windowsize = (m_evaluationType == MinWin) ? getMinWinSize() : getMaxWinSize();
	return m_arsize >= m_cardinality;
}

		
bool PositionWindow::first()
{
	while (!m_windowsize)
	{
		if (!advance()) return false;
	}
	return true;
}

bool PositionWindow::next()
{
	m_isnew_bitset.reset();
	do
	{
		if (!advance()) return false;
	}
	while (!m_windowsize);
	return true;
}

bool PositionWindow::skip( const Index& pos_)
{
	m_isnew_bitset.reset();
	do
	{
		if (!advance( pos_)) return false;
	}
	while (!m_windowsize);
	return true;
}

