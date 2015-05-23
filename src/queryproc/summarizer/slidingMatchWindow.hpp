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
#include "private/bitOperations.hpp"
#include <limits>
#include <set>
#include <algorithm>
#include <cmath>

namespace strus
{

class PosWeightTab
{
public:
	PosWeightTab()
	{
		unsigned int ii=0;
		for (; ii<TabSize; ++ii)
		{
			m_vec[ii] = (1-tanh( (float)ii/100));
		}
	}

	float operator[]( const Index& pos)
	{
		if (pos >= TabSize) return 0.0;
		return m_vec[ pos];
	}

private:
	enum {TabSize=200};
	float m_vec[TabSize];
};

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
		Index m_posno;
		unsigned int m_size;
		uint64_t m_selected;

		Window( const Index& posno_, unsigned int size_, uint64_t selected_)
			:m_posno(posno_),m_size(size_),m_selected(selected_){}
		Window( const Window& o)
			:m_posno(o.m_posno),m_size(o.m_size),m_selected(o.m_selected){}

		bool operator < (const Window& o) const
		{
			return m_posno < o.m_posno;
		}

		void setSelectedPos( const Index& posno_)
		{
			if (posno_ - m_posno > 64) return;
			m_selected |= ((uint64_t)1 << (posno_ - m_posno));
		}
		Index skipPos( Index posno_) const
		{
			if (posno_ >= m_posno + 64) return 0;
			if (posno_ < m_posno) posno_ = m_posno;
			uint64_t mask = ((uint64_t)1 << (posno_ - m_posno)) - 1;
			Index bp = BitOperations::bitScanForward( m_selected & ~mask);
			if (!bp) return 0;
			return (bp - 1 + m_posno);
		}
	};

	std::vector<Window> getResult() const
	{
		std::vector<Window> rt;
		std::set<MatchWindow>::const_iterator wi = m_windows.begin(), we = m_windows.end();
		for (; wi != we; ++wi)
		{
			rt.push_back( Window( wi->m_posno, wi->m_size, wi->m_selected));
		}
		std::sort( rt.begin(), rt.end());
		std::vector<Window>::iterator ri = rt.begin(), re = rt.end();

		while (ri != re)
		{
			std::vector<Window>::iterator ri_next = ri;
			++ri_next;
			if (ri_next != re && (unsigned int)(ri->m_posno + ri->m_size + 1) >= (unsigned int)(ri_next->m_posno))
			{
				//... ri reaches into ri_next
				ri->m_size = ri_next->m_posno + ri_next->m_size - ri->m_posno;
				Index pp = ri_next->skipPos( ri->m_posno);
				for (; pp; pp=ri_next->skipPos( pp+1))
				{
					ri->setSelectedPos( pp);
				}
				rt.erase( ri_next);
				re = rt.end();
			}
			else
			{
				++ri;
			}
		}
		return rt;
	}

private:
	void calculate()
	{
		static PosWeightTab posWeightTab;

		std::set<Match>::const_iterator mi = m_matchset.begin(), me = m_matchset.end();
		if (mi == me) return;
		float posw = posWeightTab[ mi->posno];

		Index pp = mi->posno;
		float ww = mi->weight;
		float sqw = (ww*ww) / (16 * m_windowSize+1);
		MatchWindow win( pp, 1, ww);
		win.setSelectedPos( pp);

		for (++mi; mi != me && (Index)(pp + m_windowSize) > mi->posno; ++mi)
		{
			float weight = mi->weight;
			std::set<Match>::const_iterator mi_next = mi;
			mi_next++;
			if (mi_next != me)
			{
				weight += mi_next->weight * (1/(mi_next->posno - mi->posno + 1));
			}
			ww += weight;
			float sqw2 = (ww*ww) / (16 * m_windowSize + mi->posno - pp + 1);
			if (sqw2 < sqw) break;
			sqw = sqw2;
			win.m_size = mi->posno - win.m_posno + 1;
			win.setSelectedPos( mi->posno);
		}
		win.m_weight = sqw + (sqw * posw)/8;
		pushWindow( win);
	}

	struct MatchWindow
		:public Window
	{
		float m_weight;

		MatchWindow( const Index& posno_, unsigned int size_, float weight_)
			:Window(posno_,size_,0),m_weight(weight_){}
		MatchWindow( const MatchWindow& o)
			:Window(o),m_weight(o.m_weight){}

		bool operator<( const MatchWindow& o) const
		{
			if (m_weight + std::numeric_limits<float>::epsilon() < o.m_weight) return false;
			if (m_weight - std::numeric_limits<float>::epsilon() > o.m_weight) return true;
			if (m_posno != o.m_posno) return (m_posno > o.m_posno);
			return (m_size > o.m_size);
		}
	};

	void pushWindow( const MatchWindow& win)
	{
		if (m_windows.size())
		{
			std::set<MatchWindow>::iterator it = m_windows.end();
			--it;
			if (it->m_weight < win.m_weight)
			{
				if (m_windows.size() >= m_nofWindows)
				{
					m_windows.erase(it);
				}
				m_windows.insert( win);
			}
			else if (m_windows.size() < m_nofWindows)
			{
				m_windows.insert( win);
			}
		}
		else
		{
			m_windows.insert( win);
		}
	}

	unsigned int m_windowSize;
	unsigned int m_nofWindows;
	std::set<Match> m_matchset;
	std::set<MatchWindow> m_windows;
};


}//namespace
#endif


