#include "strus/iterator.hpp"
#include <cstdlib>

using namespace strus;

StoragePositionIterator::StoragePositionIterator( Storage* storage_, const TermNumber& termnum_)
	:m_storage(storage_),m_termnum(termnum_)
{
	m_storage->openIterator( *this, m_termnum);
}

StoragePositionIterator::~StoragePositionIterator()
{
	m_storage->closeIterator( *this);
}

bool StoragePositionIterator::fetch()
{
	return m_storage->nextIterator( *this);
}


UnionPositionIterator::UnionPositionIterator( PositionIterator* term1_, PositionIterator* term2_)
	:m_term1(term1_),m_term2(term2_),m_memblocksize(term1_->m_posarsize+term2_->m_posarsize+1)
{
	m_posar = (Position*)std::calloc( m_memblocksize, sizeof(*m_posar));
	if (!m_posar) throw std::bad_alloc();
	getNextChunk();
}

UnionPositionIterator::~UnionPositionIterator()
{
	std::free( m_posar);
}

bool UnionPositionIterator::fetch()
{
	m_posarsize = 0;
	m_posidx = 0;
	getNextChunk();
	return (m_posidx < m_posarsize);
}

void UnionPositionIterator::getNextChunk()
{
	Position p1 = 0;
	Position p2 = 0;
	if (m_term1)
	{
		p1 = m_term1->get();
		if (p1 == 0 && !m_term1->m_eof) { m_term1->fetch(); p1 = m_term1->get();}
	}
	if (m_term2)
	{
		p2 = m_term2->get();
		if (p2 == 0 && !m_term2->m_eof) { m_term2->fetch(); p2 = m_term2->get();}
	}

	while (m_posarsize < m_memblocksize)
	{
		if (p1 == 0 || p2 == 0) break;
		if (p1 <= p2)
		{
			m_posar[ m_posarsize++] = p1;
			m_term1->next();
			if (p1 == p2)
			{
				m_term2->next();
				p2 = m_term2->get();
			}
			p1 = m_term1->get();
		}
		else
		{
			m_posar[ m_posarsize++] = p2;
			m_term2->next();
			p2 = m_term2->get();
		}
	}
	if (m_term2 && (!m_term1 || !m_term1->get()))
	{
		while (m_posarsize < m_memblocksize)
		{
			p2 = m_term2->get();
			if (p2 == 0) break;
			m_posar[ m_posarsize++] = p2;
			m_term2->next();
		}
	}
	if (m_term1 && (!m_term2 || !m_term2->get()))
	{
		while (m_posarsize < m_memblocksize)
		{
			p1 = m_term1->get();
			if (p1 == 0) break;
			m_posar[ m_posarsize++] = p1;
			m_term1->next();
		}
	}
}


IntersectionCutPositionIterator::IntersectionCutPositionIterator( PositionIterator* ths_, PositionIterator* oth_, int rangestart_, int range_, PositionIterator* cut_)
	:m_ths(ths_),m_oth(oth_),m_cut(cut_),m_rangestart(rangestart_),m_range(range_),m_memblocksize(m_ths->m_posarsize+1)
{
	m_posar = (Position*)std::calloc( m_memblocksize, sizeof(*m_posar));
	if (!m_posar) throw std::bad_alloc();
	getNextChunk();
}

IntersectionCutPositionIterator::~IntersectionCutPositionIterator()
{
	std::free( m_posar);
}

bool IntersectionCutPositionIterator::fetch()
{
	m_posarsize = 0;
	m_posidx = 0;
	getNextChunk();
	return (m_posidx < m_posarsize);
}

void IntersectionCutPositionIterator::getNextChunk()
{
	if (!m_ths || !m_oth) return;
	Position tp = m_ths->get();
	Position op = m_oth->get();
	Position np = m_cut?m_cut->get():0;
	if (tp == 0 && !m_ths->m_eof) { m_ths->fetch(); tp = m_ths->get();}
	if (op == 0 && !m_oth->m_eof) { m_oth->fetch(); op = m_oth->get();}
	if (np == 0 && m_cut && !m_cut->m_eof) { m_cut->fetch(); np = m_cut->get();}

	if (m_range >= 0)
	{
		while (m_posarsize < m_memblocksize)
		{
			if (op < tp + m_rangestart)
			{
				m_oth->next();
				op = m_oth->get();
				continue;
			}
			if (op <= tp + m_rangestart + m_range)
			{
				while (np != 0 && np < tp + m_rangestart)
				{
					m_cut->next();
					np = m_cut->get();
				}
				if (np == 0 || np > op)
				{
					m_posar[ m_posarsize++] = tp;
				}
			}
			m_ths->next();
			tp = m_ths->get();
		}
		m_eof = (op == 0 || tp == 0);
	}
	else
	{
		while (m_posarsize < m_memblocksize)
		{
			if (op < tp + m_rangestart + m_range)
			{
				m_oth->next();
				op = m_oth->get();
				continue;
			}
			if (op <= tp + m_rangestart)
			{
				while (np != 0 && np < tp + m_rangestart + m_range)
				{
					m_cut->next();
					np = m_cut->get();
				}
				if (np == 0 || np > tp + m_rangestart)
				{
					m_posar[ m_posarsize++] = tp;
				}
			}
			m_ths->next();
			tp = m_ths->get();
		}
		m_eof = (op == 0 || tp == 0);
	}
}



