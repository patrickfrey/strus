#include "iterator/iteratorUnion.hpp"

using namespace strus;

static inline Index selectSmallerNotNull( Index idx0, Index idx1)
{
	if (idx0 <= idx1)
	{
		if (idx0)
		{
			if (idx0 == idx1)
			{
				return idx0;
			}
			else
			{
				return idx0;
			}
		}
		else if (idx1)
		{
			return idx1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (idx1)
		{
			return idx1;
		}
		else //if (idx0) ... always true, because idx0 > idx1
		{
			return idx0;
		}
	}
}

IteratorUnion::IteratorUnion( const IteratorReference& first_, const IteratorReference& second_)
	:m_docno(0)
	,m_first(first_)
	,m_second(second_)
	,m_open_first(false)
	,m_open_second(false)
{}

IteratorUnion::IteratorUnion( const IteratorUnion& o)
	:m_docno(o.m_docno)
	,m_first(o.m_first->copy())
	,m_second(o.m_second->copy())
	,m_open_first(o.m_open_first)
	,m_open_second(o.m_open_second)
{}

Index IteratorUnion::skipDoc( const Index& docno_)
{
	Index docno_first = m_first->skipDoc( docno_);
	Index docno_second = m_second->skipDoc( docno_);

	Index rt = selectSmallerNotNull( docno_first, docno_second);
	if (rt)
	{
		m_docno = rt;
		m_open_first  = (docno_first == rt);
		m_open_second = (docno_second == rt);
	}
	return rt;
}

Index IteratorUnion::skipPos( const Index& pos_)
{
	Index pos_first = 0;
	Index pos_second = 0;

	if (m_open_first)
	{
		pos_first = m_first->skipPos( pos_);
	}
	if (m_open_second)
	{
		pos_second = m_second->skipPos( pos_);
	}
	return selectSmallerNotNull( pos_first, pos_second);
}

float IteratorUnion::weight() const
{
	float w1 = m_first->weight();
	float w2 = m_second->weight();
	return (w1 > w2)?w1:w2;
}


