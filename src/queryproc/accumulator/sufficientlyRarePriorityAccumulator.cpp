#include "sufficientlyRarePriorityAccumulator.hpp"
#include <cstdlib>
#include <limits>
#include <set>

using namespace strus;

Index ror(Index x, unsigned int moves)
{
	return (x >> moves) | (x << (sizeof(Index)*8 - moves));
}

Index hash64shift( Index key)
{
	key = (~key) + (key << 21);
	key = key ^ ror( key, 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ ror(key, 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ ror( key, 28);
	key = key + (key << 31);
	return key;
}

Index randomDocumentNumber( Index maxdocno, unsigned int no)
{
	return hash64shift( maxdocno+no) % (maxdocno+1);
}

static double calcInitialWeight( Index maxdocno, const IteratorReference& itr)
{
	double rt = 0.0;
	std::size_t ii=0;
	for (; ii<5; ++ii)
	{
		Index docno = randomDocumentNumber( maxdocno, ii);
		Index follow_docno = itr->skipDoc( docno);
		double ww = 1.0 + follow_docno - docno;
		rt += 1.0 / (ww * ww);
	}
	return rt;
}

SufficientlyRarePriorityAccumulator::SufficientlyRarePriorityAccumulator( Index maxDocno_, const std::vector<IteratorReference>& arg_)
	:m_maxDocno(maxDocno_),m_count(0),m_weight(0.0)
{
	std::vector<IteratorReference>::const_iterator ai = arg_.begin(), ae = arg_.end();
	for (; ai != ae; ++ai)
	{
		PrioritisedIterator pitr( *ai, calcInitialWeight( m_maxDocno, *ai));
		m_iterPrioList.push_back( pitr);
		pitr.priority = 0.0;
		m_nextPrioList.push_back( pitr);
	}
}

void SufficientlyRarePriorityAccumulator::recalculatePriorityList()
{
	std::vector<PrioritisedIterator>::const_iterator pi = m_nextPrioList.begin(), pe = m_nextPrioList.end();
	std::set<PrioritisedIterator> priolist;

	for (; pi != pe; ++pi)
	{
		priolist.insert( *pi);
	}

	m_nextPrioList.clear();
	m_iterPrioList.clear();
	std::set<PrioritisedIterator>::const_iterator ai = priolist.begin(), ae = priolist.end();
	for (; ai != ae; ++ai)
	{
		m_iterPrioList.push_back( *ai);
		m_nextPrioList.push_back( *ai);
		if (ai->priority < std::numeric_limits<double>::max())
		{
			m_nextPrioList.back().priority = 0.0;
		}
	}
}

bool SufficientlyRarePriorityAccumulator::nextRank( Index& docno_, int& state_, double& weight_)
{
	++m_count;
	if (m_count == 20)
	{
		recalculatePriorityList();
	}
AGAIN:
	std::vector<PrioritisedIterator>::iterator ai = m_iterPrioList.begin(), ae = m_iterPrioList.end();
	std::vector<PrioritisedIterator>::iterator ni = m_nextPrioList.begin(), ne = m_nextPrioList.end();
	if (ai == ae)
	{
		return 0;
	}
	Index nextDocno = ai->itr->skipDoc( docno_);
	ai->set( docno_, nextDocno);
	if (!nextDocno)
	{
		PrioritisedIterator pitr = *ai;
		if (pitr.priority >= std::numeric_limits<double>::max())
		{
			return false;
		}
		pitr.priority = std::numeric_limits<double>::max();
		m_iterPrioList.erase( m_iterPrioList.begin());
		m_iterPrioList.push_back( pitr);
		m_nextPrioList.erase( m_nextPrioList.begin());
		m_nextPrioList.push_back( pitr);
		state_ += 1;
		goto AGAIN;
	}
	docno_ = nextDocno;
	m_weight = ai->itr->weight();

	for (++ai,++ni; ai != ae && ni != ne; ++ai,++ni)
	{
		Index nextDocno = ai->itr->skipDoc( docno_);
		double ww = nextDocno - docno_;
		if (ni->priority < std::numeric_limits<double>::max())
		{
			ni->priority += ww * ww;
		}
		if (nextDocno == docno_)
		{
			m_weight += ai->itr->weight();
		}
	}
	weight_ = m_weight;
	return true;
}

Index SufficientlyRarePriorityAccumulator::skipDoc( const Index& docno)
{
	std::vector<PrioritisedIterator>::iterator ai = m_iterPrioList.begin(), ae = m_iterPrioList.end();
	std::vector<PrioritisedIterator>::iterator ni = m_nextPrioList.begin(), ne = m_nextPrioList.end();
	if (ai == ae)
	{
		return 0;
	}
	Index min_docno = ai->itr->skipDoc( docno);
	m_weight = 0.0;

	for (++ni,++ai; ai != ae && ni != ne; ++ai,++ni)
	{
		Index next_docno = ai->itr->skipDoc( docno);
		if (next_docno <= min_docno && next_docno != 0)
		{
			if (next_docno == min_docno)
			{
				m_weight += ai->itr->weight();
			}
			else
			{
				min_docno = next_docno;
				m_weight = ai->itr->weight();
			}
			double ww = next_docno - docno;
			if (ni->priority < std::numeric_limits<double>::max())
			{
				ni->priority += ww * ww;
			}
		}
	}
	return min_docno;
}

double SufficientlyRarePriorityAccumulator::weight()
{
	return m_weight;
}


