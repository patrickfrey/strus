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
#ifndef _STRUS_SUMMARIZER_MATCH_PHRASE_SLIDING_MATCH_WINDOW_HPP_INCLUDED
#define _STRUS_SUMMARIZER_MATCH_PHRASE_SLIDING_MATCH_WINDOW_HPP_INCLUDED
#include "strus/index.hpp"
#include <limits>
#include <set>
#include <algorithm>
#include <cmath>

namespace strus
{

class SlidingMatchWindow
{
public:
	struct Match
	{
		Index posno;
		float weight;
		unsigned int postingsidx;

		Match( const Index& posno_, float weight_, unsigned int postingsidx_)
			:posno(posno_),weight(weight_),postingsidx(postingsidx_){}
		Match( const Match& o)
			:posno(o.posno),weight(o.weight),postingsidx(o.postingsidx){}

		bool operator<( const Match& o) const
		{
			if (posno == o.posno) return (postingsidx < o.postingsidx);
			return (posno < o.posno);
		}
	};

	SlidingMatchWindow( unsigned int windowSize_, unsigned int nofWindows_, const std::set<Match>& matchset_)
		:m_windowSize(windowSize_),m_nofWindows(nofWindows_),m_matchset(matchset_)
	{
		calculate();
	}

	bool finished() const
	{
		return m_matchset.empty();
	}

	unsigned int firstPostingIdx() const
	{
		return m_matchset.begin()->postingsidx;
	}

	void push( unsigned int postingsidx_, const Index& posno_)
	{
		if (m_matchset.begin()->postingsidx != postingsidx_)
		{
			throw std::logic_error( "illegal call of SlidingMatchWindow::push");
		}
		float weight = m_matchset.begin()->weight;
		m_matchset.erase( m_matchset.begin());
		if (posno_)
		{
			m_matchset.insert( Match( posno_, weight, postingsidx_));
		}
		calculate();
	}

	struct Window
	{
		Index pos;
		unsigned int size;

		Window( const Index& pos_, unsigned int size_)
			:pos(pos_),size(size_){}
		Window( const Window& o)
			:pos(o.pos),size(o.size){}

		bool operator < (const Window& o) const
		{
			return pos < o.pos;
		}
	};

	std::vector<Window> getResult() const
	{
		std::vector<Window> rt;
		std::set<MatchWindow>::const_iterator wi = m_windows.begin(), we = m_windows.end();
		for (; wi != we; ++wi)
		{
			rt.push_back( Window( wi->m_posno, wi->m_size));
		}
		std::sort( rt.begin(), rt.end());
		std::vector<Window>::iterator ri = rt.begin(), re = rt.end();
		for (; ri != re; ++ri)
		{
			std::vector<Window>::iterator ri_next = ri;
			++ri_next;
			if (ri_next != re && (unsigned int)(ri_next->pos) <= (unsigned int)(ri->pos + ri->size))
			{
				if ((unsigned int)(ri_next->pos + ri_next->size) > (unsigned int)(ri->pos + ri->size))
				{
					ri->size = ri_next->pos + ri_next->size - ri->pos;
				}
				rt.erase( ri_next);
			}
		}
		return rt;
	}

private:
	void calculate()
	{
		std::set<Match>::const_iterator mi = m_matchset.begin(), me = m_matchset.end();
		if (mi == me) return;
		float posw = logf( mi->posno);

		Index pp = mi->posno;
		Index last_pp = mi->posno;
		float ww = mi->weight;
		float sqw = (ww*ww) / (m_windowSize+1);

		for (++mi; mi != me && (Index)(pp + m_windowSize) > mi->posno; ++mi)
		{
			ww += mi->weight;
			float sqw2 = (ww*ww) / (m_windowSize + mi->posno - pp + 1);
			if (sqw2 < sqw) break;
			sqw = sqw2;
			last_pp = mi->posno;
		}
		pushWindow( pp, last_pp - pp + 1, sqw - posw);
	}

	struct MatchWindow
	{
		Index m_posno;
		unsigned int m_size;
		float m_weight;

		MatchWindow( const Index& posno_, unsigned int size_, float weight_)
			:m_posno(posno_),m_size(size_),m_weight(weight_){}
		MatchWindow( const MatchWindow& o)
			:m_posno(o.m_posno),m_size(o.m_size),m_weight(o.m_weight){}

		bool operator<( const MatchWindow& o) const
		{
			if (m_weight + std::numeric_limits<float>::epsilon() < o.m_weight) return false;
			if (m_weight - std::numeric_limits<float>::epsilon() > o.m_weight) return true;
			if (m_posno != o.m_posno) return (m_posno > o.m_posno);
			return (m_size > o.m_size);
		}
	};

	void pushWindow( Index posno_, unsigned int size_, float weight_)
	{
		m_windows.insert( MatchWindow( posno_, size_, weight_));
		if (m_windows.size() >= m_nofWindows)
		{
			std::set<MatchWindow>::iterator it = m_windows.end();
			--it;
			m_windows.erase(it);
		}
	}

	unsigned int m_windowSize;
	unsigned int m_nofWindows;
	std::set<Match> m_matchset;
	std::set<MatchWindow> m_windows;
};


}//namespace
#endif


