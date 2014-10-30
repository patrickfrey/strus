#include "iterator/iteratorUnion.hpp"

using namespace strus;


IteratorUnion::IteratorUnion( std::size_t nofargs, const IteratorInterface** args)
	:m_docno(0)
{
	m_selected.reserve( nofargs);
	m_argar.reserve( nofargs);
	std::size_t ii=0;
	for (; ii<nofargs; ++ii)
	{
		if (args[ii])
		{
			m_selected.push_back(false);
			m_argar.push_back( args[ii]->copy());
			m_featureid.append( args[ii]->featureid());
		}
	}
	m_featureid.push_back( 'U');
}

IteratorUnion::IteratorUnion( const IteratorUnion& o)
	:m_docno(o.m_docno)
	,m_selected(o.m_selected)
	,m_featureid(o.m_featureid)
{
	std::size_t ii=0;
	m_argar.reserve( o.m_argar.size());
	for (; ii<o.m_argar.size(); ++ii)
	{
		if (o.m_argar[ii].get())
		{
			m_argar.push_back( o.m_argar[ ii]->copy());
		}
	}
}

std::vector<const IteratorInterface*> IteratorUnion::subExpressions( bool positive)
{
	std::vector<const IteratorInterface*> rt;
	if (positive)
	{
		rt.reserve( m_argar.size());
		std::size_t ii=0;
		for (; ii<m_argar.size(); ++ii)
		{
			rt.push_back( m_argar[ ii].get());
		}
	}
	return rt;
}

static inline Index selectSmallerNotNull( Index idx0, Index idx1)
{
	if (idx0 <= idx1)
	{
		if (idx0)
		{
			return idx0;
		}
		else
		{
			return idx1;
		}
	}
	else
	{
		if (idx1)
		{
			return idx1;
		}
		else
		{
			return idx0;
		}
	}
}

Index IteratorUnion::skipDoc( const Index& docno_)
{
	if (m_docno == docno_)
	{
		return m_docno;
	}
	m_docno = docno_;
	std::vector<IteratorReference>::const_iterator ai = m_argar.begin(), ae = m_argar.end();
	if (ai == ae) return 0;

	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		Index candidate = (*ai)->skipDoc( docno_);
		m_docno = selectSmallerNotNull( m_docno, candidate);
		m_selected[ aidx] = (m_docno == candidate);
	}
	return m_docno;
}

Index IteratorUnion::skipPos( const Index& pos_)
{
	std::vector<bool>::const_iterator si = m_selected.begin(), se = m_selected.end();
	Index pos = pos_;
	for (int aidx=0; si != se; ++si,++aidx)
	{
		if (*si)
		{
			pos = selectSmallerNotNull( pos, m_argar[ aidx]->skipPos( pos_));
		}
	}
	return pos;
}


