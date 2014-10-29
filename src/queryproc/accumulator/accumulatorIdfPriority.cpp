#include "accumulator/accumulatorIdfPriority.hpp"
#include "weighting/createWeightingFunction.hpp"
#include <cstdlib>
#include <limits>
#include <set>
#include <stdexcept>
#include <cmath>

using namespace strus;

AccumulatorIdfPriority::AccumulatorIdfPriority( const StorageInterface* storage_)
	:m_storage(storage_)
	,m_estimatedNumberOfMatchesMap(new EstimatedNumberOfMatchesMap( storage_))
	,m_started(false)
{}

AccumulatorIdfPriority::AccumulatorIdfPriority( const AccumulatorIdfPriority& o)
	:m_storage(o.m_storage)
	,m_estimatedNumberOfMatchesMap( new EstimatedNumberOfMatchesMap( o.m_storage))
	,m_argumentList(o.m_argumentList)
	,m_argumentOrder(o.m_argumentOrder)
	,m_argumentIter(o.m_argumentIter)
	,m_visited(o.m_visited)
	,m_started(o.m_started)
{}

void AccumulatorIdfPriority::add(
		double factor,
		const std::string& function,
		const std::vector<float>& parameter,
		const IteratorInterface& iterator)
{
	if (m_started) throw std::runtime_error("internal: cannot add arguments to accumulator after evaluation started");

	WeightingFunctionReference weighting(
		createWeightingFunction(
			m_storage, m_estimatedNumberOfMatchesMap,
			function, parameter));
	IteratorReference itr( iterator.copy());

	m_argumentList.push_back( AccumulatorArgument( factor, weighting, itr));
}


bool AccumulatorIdfPriority::nextRank(
		Index& docno_,
		int& state_,
		double& weight_)
{
	if (!m_started)
	{
		std::vector<AccumulatorArgument>::const_iterator ai = m_argumentList.begin(), ae = m_argumentList.end();
		for (std::size_t aidx=0; ai != ae; ++ai,++aidx)
		{
			double td = m_estimatedNumberOfMatchesMap->get( *ai->itr);
			double idf;
			if (td != 0)
			{
				Index N = m_storage->nofDocumentsInserted();
				idf = log ((N + 0.5) / (td + 0.5));
				if (idf < 0.0)
				{
					idf = 0.0;
				}
			}
			else
			{
				idf = 0.0;
			}
			m_argumentOrder.insert( ArgumentRef( idf, aidx));
		}
		state_ = 0;
		m_started = true;
		m_argumentIter = m_argumentOrder.begin();
		if (m_argumentIter == m_argumentOrder.end())
		{
			return false;
		}
		m_argumentEnd = m_argumentIter;
		double cut_idf = m_argumentIter->idf - 1.5;

		for (; m_argumentEnd != m_argumentOrder.end(); ++m_argumentEnd)
		{
			if (cut_idf > m_argumentEnd->idf) break;
		}
	}
AGAIN:
	const AccumulatorArgument& arg = m_argumentList[ m_argumentIter->idx];
	Index nextDocno = arg.itr->skipDoc( docno_);
	if (!nextDocno)
	{
		if (state_ == 0 && m_argumentIter == m_argumentOrder.begin())
		{
			state_ = 1;
			docno_ = 0;
			goto AGAIN;
		}
		if (++m_argumentIter == m_argumentEnd)
		{
			return false;
		}
	}
	if (m_visited.find( nextDocno) != m_visited.end())
	{
		docno_ = nextDocno+1;
		goto AGAIN;
	}
	m_visited.insert( docno_ = nextDocno);
	std::vector<AccumulatorArgument>::iterator ai = m_argumentList.begin(), ae = m_argumentList.end();
	std::size_t idx = 0;
	weight_ = 0.0;

	if (state_ == 0 && m_argumentIter == m_argumentOrder.begin())
	{
		for (; ai != ae; ++ai,++idx)
		{
			if (idx != m_argumentIter->idx)
			{
				if (docno_ != ai->itr->skipDoc( docno_))
				{
					break;
				}
			}
		}
		if (ai != ae)
		{
			docno_ = nextDocno+1;
			goto AGAIN;
		}
		ai = m_argumentList.begin();
		for (; ai != ae; ++ai,++idx)
		{
			weight_ += ai->function->call( *ai->itr) * ai->factor;
		}
	}
	else
	{
		for (; ai != ae; ++ai,++idx)
		{
			if (docno_ == ai->itr->skipDoc( docno_))
			{
				weight_ += ai->function->call( *ai->itr) * ai->factor;
			}
		}
	}
	return true;
}

