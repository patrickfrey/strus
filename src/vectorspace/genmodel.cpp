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
#include <cmath>
#include <iostream>

using namespace strus;
#define STRUS_LOWLEVEL_DEBUG

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
	m_sampleGroupCntMap.push_back( 0);
	SampleIndex sampleidx = m_samplear.size();

	std::list<Group>::iterator gi = m_groupList.begin(), ge = m_groupList.end();
	std::vector<std::list<Group>::iterator> nbgroups;
	for (; gi != ge; ++gi)
	{
		if (sample.near( gi->gencode(), m_nbdist))
		{
			if (sample.near( gi->gencode(), m_simdist))
			{
				++m_sampleGroupCntMap.back();
				gi->addMember( sampleidx, ++m_timestamp);
			}
			else
			{
				m_neighbourSampleSet.insert( NeighbourSampleSet::value_type( gi->id(), sampleidx));
			}
		}
	}
	if (m_sampleGroupCntMap.back() == 0)
	{
		// no group to assign, then found own group:
		addGroup( sampleidx);
	}
}

SimHash GenModel::groupKernel( const Group& group)
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
	++m_sampleGroupCntMap[ sampleidx];
	Group group( ++m_groupcnt, ++m_timestamp, m_samplear[ sampleidx]);
	std::list<Group>::const_iterator gi = m_groupList.begin(), ge = m_groupList.end();
	const SimHash& sample = m_samplear[ sampleidx];
	for (; gi != ge; ++gi)
	{
		if (sample.near( gi->gencode(), m_nbdist))
		{
			m_neighbourGroupSet.insert( NeighbourGroupSet::value_type( gi->id(), group.id()));
			m_neighbourGroupSet.insert( NeighbourGroupSet::value_type( group.id(), gi->id()));
		}
	}
	group.addMember( sampleidx, m_timestamp);
	m_groupList.push_back( group);
	std::list<Group>::iterator lastitr = m_groupList.end();
	--lastitr;
	m_groupMap.insert( GroupMap::value_type( group.id(), lastitr));
}

SimHash GenModel::mutation( const Group& group)
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

double GenModel::fitness( const SimHash& candidate, const Group& group)
{
	double sqrsum = 0.0;
	std::set<SampleIndex>::const_iterator mi = group.members().begin(), me = group.members().end();
	for (; mi != me; ++mi)
	{
		double dist = candidate.dist( m_samplear[ *mi]);
		sqrsum += dist * dist;
	}
	double costs = std::sqrt( sqrsum);

	NeighbourSampleSet::const_iterator
		si = m_neighbourSampleSet.upper_bound( NeighbourSampleSet::value_type(group.id(),0)),
		se = m_neighbourSampleSet.end();
	for (; si != se && si->first == group.id(); ++si)
	{
		if (candidate.near( m_samplear[ si->second], m_simdist))
		{
			costs -= costs / 20;
		}
	}
	return 1.0 / costs;
}

void GenModel::mutate( Group& group)
{
	std::vector<SimHash> descendantlist;
	descendantlist.reserve( m_descendants);

	double max_fitness = fitness( group.gencode(), group);
	int selected = -1;
	std::size_t di=0, de=m_descendants;
	for (; di != de; ++di)
	{
		descendantlist.push_back( mutation( group));
		double desc_fitness = fitness( descendantlist.back(), group);
		if (desc_fitness > max_fitness)
		{
			selected = (int)di;
			max_fitness = desc_fitness;
		}
	}
	if (selected)
	{
		group.setGencode( descendantlist[ di], ++m_timestamp);
	}
}


void GenModel::reorganizeMembers( Group& group)
{
	std::vector<SampleIndex> moves;
	// Move members that moved to far away to neighbours:
	std::set<SampleIndex>::iterator mi = group.members().begin(), me = group.members().end();
	for (; mi != me; ++mi)
	{
		if (!group.gencode().near( m_samplear[ *mi], m_simdist))
		{
			moves.push_back( *mi);
		}
	}
	std::vector<SampleIndex>::const_iterator vi = moves.begin(), ve = moves.end();
	for (; vi != ve; ++vi)
	{
		group.removeMember( *vi);
		if (m_sampleGroupCntMap[*vi] > 0)
		{
			--m_sampleGroupCntMap[*vi];
			if (m_sampleGroupCntMap[*vi] == 0)
			{
				// no group assigned anymore, then found own group:
				addGroup( *vi);
			}
		}
		else
		{
			throw strus::runtime_error(_TXT( "corrupt data in gen model"));
		}
		m_neighbourSampleSet.insert( NeighbourSampleSet::value_type( group.id(), *vi));
	}
	moves.resize(0);

	// Move neighbours that are so close to become members to such:
	NeighbourSampleSet::const_iterator
		si = m_neighbourSampleSet.upper_bound( NeighbourSampleSet::value_type( group.id(), 0)),
		se = m_neighbourSampleSet.end();
	for (; si != se && si->first == group.id(); ++si)
	{
		if (group.gencode().near( m_samplear[ si->second], m_simdist))
		{
			moves.push_back( si->second);
		}
	}
	vi = moves.begin(), ve = moves.end();
	for (; vi != ve; ++vi)
	{
		++m_sampleGroupCntMap[*vi];
		group.addMember( *vi, ++m_timestamp);
		m_neighbourSampleSet.erase( NeighbourSampleSet::value_type( group.id(), *vi));
	}
}

void GenModel::iteration()
{
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "[genmodel] do iteration" << std::endl;
#endif
	// Iterate through all groups and try to find a fitter representant:
	std::list<Group>::iterator gi = m_groupList.begin(), ge = m_groupList.end();
	for (; gi != ge; ++gi)
	{
		mutate( *gi);
		reorganizeMembers( *gi);
	}

	// Check neighbour groups for unification:
	std::vector<NeighbourGroupSet::value_type> neighbourGroupMapDeletes;
	std::vector<Index> groupDeletes;
	NeighbourGroupSet::iterator
		ri = m_neighbourGroupSet.begin(),
		re = m_neighbourGroupSet.end();
	for (; ri != re; ++ri)
	{
		GroupMap::iterator first_pi = m_groupMap.find( ri->first);
		GroupMap::iterator second_pi = m_groupMap.find( ri->second);
		if (first_pi == m_groupMap.end() || second_pi == m_groupMap.end())
		{
			neighbourGroupMapDeletes.push_back( *ri);
		}
		else if (first_pi->second->gencode().dist( second_pi->second->gencode()) <= m_simdist)
		{
			bool moveAllMembers = true;
			std::set<SampleIndex>::const_iterator
				si = second_pi->second->members().begin(), 
				se = second_pi->second->members().end();
			for (; si != se; ++si)
			{
				if (first_pi->second->members().find( *si) == first_pi->second->members().end())
				{
					if (first_pi->second->gencode().dist( m_samplear[ *si]) <= m_simdist)
					{
						first_pi->second->addMember( *si, ++m_timestamp);
					}
					else
					{
						moveAllMembers = false;
					}
				}
			}
			if (moveAllMembers)
			{
				// Mark group 1 and its neighbour relations to delete:
				neighbourGroupMapDeletes.push_back( *ri);
				groupDeletes.push_back( ri->second);
				// Move all neighbour elements from group 2 to group 1:
				NeighbourSampleSet::const_iterator
					si = m_neighbourSampleSet.upper_bound( NeighbourSampleSet::value_type( ri->second, 0)),
					se = m_neighbourSampleSet.end();
				for (; si != se && si->first == ri->second; ++si)
				{
					m_neighbourSampleSet.insert( NeighbourSampleSet::value_type( ri->first, si->second));
				}
			}
		}
	}

	// Do the deletes after unification of similar groups:
	std::vector<NeighbourGroupSet::value_type>::const_iterator
		bi = neighbourGroupMapDeletes.begin(), be = neighbourGroupMapDeletes.end();
	for (; bi != be; ++bi)
	{
		m_neighbourGroupSet.erase( *bi);
	}
	std::vector<Index>::const_iterator di = groupDeletes.begin(), de = groupDeletes.end();
	for (; di != de; ++di)
	{
		GroupMap::const_iterator mi = m_groupMap.find( *di);
		if (mi != m_groupMap.end())
		{
			m_groupList.erase( mi->second);
		}
		m_groupMap.erase( *di);
	}
}



