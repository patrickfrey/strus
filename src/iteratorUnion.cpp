#include "strus/iteratorUnion.hpp"

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

IteratorUnion::IteratorUnion( const IteratorInterfaceR& first_, const IteratorInterfaceR& second_)
	:m_docno(0)
	,m_first(first_)
	,m_second(second_)
	,m_open_first(false)
	,m_open_second(false)
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



