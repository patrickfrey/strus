#include "iterator/postingIteratorUnionWeighted.hpp"
#include <utility>

using namespace strus;


IteratorUnionWeighted::IteratorUnionWeighted( std::size_t nofargs, const PostingIteratorInterface** args)
	:IteratorUnion( nofargs, args)
{
	m_weightitr = m_weightmap.end();
}


IteratorUnionWeighted::IteratorUnionWeighted( const IteratorUnionWeighted& o)
	:IteratorUnion(o)
	,m_weightmap(o.m_weightmap)
{
	m_weightitr = m_weightmap.find( o.posno());
}


Index IteratorUnionWeighted::skipDoc( const Index& docno_)
{
	Index rt = IteratorUnion::skipDoc( docno_);
	m_weightmap.clear();
	std::size_t si = 0, se = nofargs();
	for (; si != se; ++si)
	{
		Index idx = 0;
		for (;0!=(idx=skipPos( idx))
			&& rt < (unsigned int)std::numeric_limits<short>::max(); ++idx)
		{
			m_weightmap[ idx] += 1.0f/se;
		}
	}
	return rt;
}


Index IteratorUnionWeighted::skipPos( const Index& pos_)
{
	m_weightitr = m_weightmap.upper_bound( pos_);
	if (m_weightitr == m_weightmap.end()) return 0;
	return m_weightitr->second;
}

float IteratorUnionWeighted::weight() const
{
	if (m_weightitr == m_weightmap.end()) return 0.0;
	return m_weightitr->second;
}
