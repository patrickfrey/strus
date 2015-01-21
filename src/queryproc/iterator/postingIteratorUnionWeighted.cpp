#include "iterator/postingIteratorUnionWeighted.hpp"
#include <utility>

using namespace strus;


IteratorUnionWeighted::IteratorUnionWeighted( const std::vector<Reference< PostingIteratorInterface> >& args)
	:IteratorUnion( args)
{
	m_weightitr = m_weightmap.end();
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
			&& rt < (Index)std::numeric_limits<short>::max(); ++idx)
		{
			m_weightmap[ idx] += 1.0f/se;
		}
	}
	return rt;
}

Index IteratorUnionWeighted::skipPos( const Index& pos_)
{
	m_weightitr = m_weightmap.upper_bound( pos_-1);
	if (m_weightitr == m_weightmap.end()) return 0;
	return m_weightitr->second;
}

float IteratorUnionWeighted::positionWeight() const
{
	if (m_weightitr == m_weightmap.end()) return 0.0;
	return m_weightitr->second;
}

