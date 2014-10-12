#include "accumulator/accumulatorPrioritised.hpp"
#include <cstdlib>
#include <limits>
#include <set>

using namespace strus;

static Index ror(Index x, unsigned int moves)
{
	return (x >> moves) | (x << (sizeof(Index)*8 - moves));
}

static Index hash64shift( Index key)
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

static Index randomDocumentNumber( Index maxdocno, unsigned int no)
{
	return hash64shift( maxdocno+no) % (maxdocno+1);
}

double AccumulatorPrioritised::calcInitialWeight( const IteratorReference& itr)
{
	enum {Picks=5};
	double rt = 0.0;
	int ii=0;
	for (; ii<(int)Picks; ++ii)
	{
		Index docno = randomDocumentNumber( m_maxDocno, ii);
		if (itr->skipDoc( docno))
		{
			rt += itr->weight();
		}
		else if (itr->skipDoc( 0))
		{
			rt += itr->weight();
		}
	}
	rt *= (double)RecalculatePrioListLoopSize / (int)Picks;
	return rt;
}

AccumulatorPrioritised::AccumulatorPrioritised(
		Index maxDocno_,
		const std::vector<IteratorReference>& arg_)
	:m_maxDocno(maxDocno_)
	,m_loopCount(0)
	,m_weight(0.0)
{
	std::set<IteratorPrioritised> priolist;
	std::vector<IteratorReference>::const_iterator ai = arg_.begin(), ae = arg_.end();
	for (; ai != ae; ++ai)
	{
		IteratorPrioritised pitr( *ai, calcInitialWeight( *ai));
		priolist.insert( pitr);
	}
	std::set<IteratorPrioritised>::const_iterator pi = priolist.begin(), pe = priolist.end();
	for (; pi != pe; ++pi)
	{
		m_iterPrioList.push_back( *pi);
	}
}

void AccumulatorPrioritised::recalculatePriorityList()
{
	std::list<IteratorPrioritised>::const_iterator ai = m_iterPrioList.begin(), ae = m_iterPrioList.end();
	std::set<IteratorPrioritised> priolist;

	for (; ai != ae; ++ai)
	{
		priolist.insert( *ai);
	}
	m_iterPrioList.clear();
	std::set<IteratorPrioritised>::const_iterator pi = priolist.begin(), pe = priolist.end();
	for (; pi != pe; ++pi)
	{
		m_iterPrioList.push_back( IteratorPrioritised( pi->itr, pi->weight/2, pi->finished));
	}
}

bool AccumulatorPrioritised::nextRank( Index& docno_, int& state_, double& weight_)
{
	++m_loopCount;
	if (m_loopCount == RecalculatePrioListLoopSize)
	{
		m_loopCount = 0;
		recalculatePriorityList();
	}
AGAIN:
	std::list<IteratorPrioritised>::iterator ai = m_iterPrioList.begin(), ae = m_iterPrioList.end();
	if (ai == ae || ai->finished)
	{
		return false;
	}
	Index nextDocno = ai->itr->skipDoc( docno_);
	if (!nextDocno)
	{
		IteratorPrioritised pitr = *ai;
		if (pitr.finished)
		{
			return false;
		}
		pitr.finished = true;
		m_iterPrioList.erase( m_iterPrioList.begin());
		m_iterPrioList.push_back( pitr);
		state_ += 1;
		docno_ = 0;
		goto AGAIN;
	}
	if (m_visited.find( nextDocno) != m_visited.end())
	{
		docno_ = nextDocno+1;
		goto AGAIN;
	}
	m_visited.insert( nextDocno);
	docno_ = nextDocno;
	m_weight = ai->itr->weight();

	for (++ai; ai != ae; ++ai)
	{
		Index nextDocno = ai->itr->skipDoc( docno_);
		double weight = ai->itr->weight();

		if (!ai->finished)
		{
			ai->weight += weight;
		}
		if (nextDocno == docno_)
		{
			m_weight += weight;
		}
	}
	weight_ = m_weight;
	return true;
}

Index AccumulatorPrioritised::skipDoc( const Index& docno)
{
	std::list<IteratorPrioritised>::iterator ai = m_iterPrioList.begin(), ae = m_iterPrioList.end();
	if (ai == ae)
	{
		return 0;
	}
	Index min_docno = ai->itr->skipDoc( docno);
	m_weight = 0.0;

	for (++ai; ai != ae; ++ai)
	{
		Index next_docno = ai->itr->skipDoc( docno);
		if (next_docno <= min_docno && next_docno != 0)
		{
			double weight = ai->itr->weight();
			if (next_docno == min_docno)
			{
				m_weight += weight;
			}
			else
			{
				min_docno = next_docno;
				m_weight = weight;
			}
			ai->weight += weight;
		}
	}
	return min_docno;
}

double AccumulatorPrioritised::weight()
{
	return m_weight;
}


