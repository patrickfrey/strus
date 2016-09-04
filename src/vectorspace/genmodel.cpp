/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for breeding good representants of similarity classes with help of genetic algorithms
#include "genmodel.hpp"
#include "random.hpp"
#include "private/internationalization.hpp"
#include <ctime>

using namespace strus;

static Random g_random;

void GenModel::addSample( const SimHash& sample)
{
	if (m_samplear.empty())
	{
		m_samplesize = sample.size();
	}
	else if (m_samplesize != sample.size())
	{
		throw strus::runtime_error(_TXT("samples added have different sizes (%u,%u)"), m_samplesize, sample.size());
	}
	m_samplear.push_back( sample);
	SampleIndex sampleidx = m_samplear.size();

	std::list<Group>::iterator gi = m_grouplist.begin(), ge = m_grouplist.end();
	std::vector<std::list<Group>::iterator> nbgroups;
	for (; gi != ge; ++gi)
	{
		if (sample.near( gi->gencode(), m_nbdist))
		{
			if (sample.near( gi->gencode(), m_simdist))
			{
				gi->addMember( sampleidx, ++m_timestamp);
			}
			else
			{
				m_neighbourmap.insert( NeighbourMap ::value_type( gi->id(), sampleidx));
				addGroup( sampleidx);
			}
		}
		else
		{
			addGroup( sampleidx);
		}
	}
}

SimHash GenModel::groupKernel( Group& group)
{
	std::set<SampleIndex>::const_iterator si = group.members().begin(), se = group.members().end();
	if (si == se) return SimHash();
	SimHash first( m_samplear[ *si]);
	SimHash rt( first.size(), true);

	for (++si; si != se; ++si)
	{
		rt &= ~(first ^ m_samplear[ *si]);
	}
	return rt;
}

void GenModel::addGroup( const SampleIndex& sampleidx)
{
	Group group( ++m_groupcnt, ++m_timestamp, m_samplear[ sampleidx]);
	group.addMember( sampleidx, m_timestamp);
	m_grouplist.push_back( group);
}

SimHash GenModel::mutation( Group& group)
{
	std::vector<SampleIndex> members;
	members.insert( members.end(), group.members().begin(), group.members().end());
	if (members.size() < 2 || m_samplesize == 0) return SimHash();

	// Calculate 'kernel' = the set of all elements equal for all members. These cannot be mutated:
	SimHash kernel = groupKernel( group);

	SimHash rt( group.gencode());

	unsigned int mi=0, me=m_mutations * (m_maxage - group.age() + 1) / (m_maxage + 1);
	for (; mi != me; ++mi)
	{
		unsigned int mutidx = g_random.get( 0, m_samplesize);
		if (kernel[ mutidx]) continue; //.... do not mutate kernel elements

		// The majority of randomly selected members decide the direction of the mutation.
		// With growing age spins with a higher vote are preferred:
		unsigned int true_cnt=0, false_cnt=0;
		std::size_t ci=0, ce=members.size();
		if (ce > group.age()) ce = group.age();
		for (; ci != ce; ++ci)
		{
			SampleIndex memberidx = g_random.get( 0, members.size());
			if (m_samplear[ members[ memberidx]][ mutidx])
			{
				++true_cnt;
			}
			else
			{
				++false_cnt;
			}
		}
		if (true_cnt > false_cnt)
		{
			rt.set( mutidx, true);
		}
		else if (true_cnt < false_cnt)
		{
			rt.set( mutidx, false);
		}
		else
		{
			rt.set( mutidx, group.gencode()[ mutidx]);
		}
	}
	return rt;
}





