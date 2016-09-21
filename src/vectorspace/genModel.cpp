/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for breeding good representants of similarity classes with help of genetic algorithms
#include "genModel.hpp"
#include "simGroupMap.hpp"
#include "random.hpp"
#include "private/internationalization.hpp"
#include <ctime>
#include <cmath>
#include <iostream>

using namespace strus;
#define STRUS_LOWLEVEL_DEBUG

static Random g_random;

class GroupIdAllocator
{
public:
	GroupIdAllocator()
		:m_cnt(0){}
	GroupIdAllocator( const GroupIdAllocator& o)
		:m_cnt(o.m_cnt),m_freeList(o.m_freeList){}

	Index alloc()
	{
		strus::Index rt;
		if (!m_freeList.empty())
		{
			rt = m_freeList.back();
			m_freeList.pop_back();
		}
		else
		{
			rt = ++m_cnt;
		}
		return rt;
	}

	void free( const Index& idx)
	{
		m_freeList.push_back( idx);
	}

private:
	Index m_cnt;
	std::vector<strus::Index> m_freeList;
};

template <class Element, std::size_t SIZE>
class FixedSizePrioQueue
{
public:
	FixedSizePrioQueue(){}

	void push( const Element& elem)
	{
		if (m_ar.size() < SIZE)
		{
			m_ar.push_back( elem);
		}
		else if (m_ar.back() < elem)
		{
			return;
		}
		Element tmp = elem;
		typename std::vector<Element>::iterator pi = m_ar.begin(), pe = m_ar.end();
		for (; pi != pe && *pi < elem; ++pi){}
		for (; pi != pe; ++pi)
		{
			std::swap( tmp, *pi);
		}
	}

	typedef typename std::vector<Element>::const_iterator const_iterator;
	const_iterator begin() const	{return m_ar.begin();}
	const_iterator end() const	{return m_ar.begin();}

private:
	std::vector<Element> m_ar;
};

static SimRelationMap getSimRelationMap( const std::vector<SimHash>& samplear, unsigned int simdist)
{
	std::vector<SimRelationMap::Element> simrellist;
	{
		std::vector<SimHash>::const_iterator si = samplear.begin(), se = samplear.end();
		for (std::size_t sidx=0; si != se; ++si,++sidx)
		{
			std::vector<SimHash>::const_iterator pi = samplear.begin();
			for (std::size_t pidx=0; pi != si; ++pi,++pidx)
			{
				if (si->near( *pi, simdist))
				{
					simrellist.push_back( SimRelationMap::Element( sidx, pidx, si->dist( *pi)));
				}
			}
		}
	}
	return SimRelationMap( simrellist);
}

struct ProxElem
{
	ProxElem( unsigned short dist_, std::size_t sampleidx_)	:dist(dist_),sampleidx(sampleidx_){}
	ProxElem( const ProxElem& o)				:dist(o.dist),sampleidx(o.sampleidx){}

	unsigned short dist;
	std::size_t sampleidx;

	bool operator<( const ProxElem& o) const	{return dist < o.dist;}
	bool operator>( const ProxElem& o) const	{return dist > o.dist;}
	bool operator==( const ProxElem& o) const	{return dist == o.dist;}
};

typedef std::list<SimGroup> GroupInstanceList;
typedef std::map<strus::Index,GroupInstanceList::iterator> GroupInstanceMap;

static void removeGroup(
		const Index& group_id,
		GroupIdAllocator& groupIdAllocator,
		GroupInstanceList& groupInstanceList,
		GroupInstanceMap& groupInstanceMap,
		SimGroupMap& simGroupMap)
{
	GroupInstanceMap::iterator group_slot = groupInstanceMap.find( group_id);
	if (group_slot == groupInstanceMap.end()) throw strus::runtime_error(_TXT("illegal reference in group map"));
	GroupInstanceList::iterator group_iter = group_slot->second;
	if (group_iter->id() != group_id) throw strus::runtime_error(_TXT("illegal reference in group list"));
	groupInstanceMap.erase( group_slot);

	std::vector<SampleIndex>::const_iterator
		mi = group_iter->members().begin(), me = group_iter->members().end();
	for (; mi != me; ++mi)
	{
		simGroupMap.remove( *mi, group_id);
	}
	groupIdAllocator.free( group_id);
	groupInstanceList.erase( group_iter);
}

static bool tryAddGroupMember( const Index& group_id, std::size_t newmember,
				GroupInstanceMap& groupInstanceMap,
				SimGroupMap& simGroupMap, const std::vector<SimHash>& samplear,
				unsigned int descendants, unsigned int mutations)
{
	GroupInstanceMap::iterator group_slot = groupInstanceMap.find( group_id);
	if (group_slot == groupInstanceMap.end()) throw strus::runtime_error(_TXT("illegal reference in group map"));
	GroupInstanceList::iterator group_inst = group_slot->second;

	SimGroup newgroup( *group_inst);
	newgroup.addMember( newmember);
	newgroup.mutate( samplear, descendants, mutations);
	if (newgroup.fitness( samplear) >= group_inst->fitness( samplear))
	{
		*group_inst = newgroup;
		simGroupMap.insert( newmember, group_inst->id());
		return true;
	}
	else
	{
		return false;
	}
}

std::vector<SimHash> GenModel::run( const std::vector<SimHash>& samplear) const
{
	GroupIdAllocator groupIdAllocator;					// Allocator of group ids
	GroupInstanceList groupInstanceList;					// list of similarity group representants
	GroupInstanceMap groupInstanceMap;					// map indices to group representant list iterators
	SimGroupMap simGroupMap( samplear.size());				// map of sample idx to group idx
	SimRelationMap simrelmap( getSimRelationMap( samplear, m_simdist));	// similarity relation map of the list of samples

	// Do the iterations of creating new individuals
	unsigned int iteration=0;
	for (; iteration != m_iterations; ++iteration)
	{
		// Go through all elements and try to create new groups:
		std::vector<SimHash>::const_iterator si = samplear.begin(), se = samplear.end();
		for (std::size_t sidx=0; si != se; ++si,++sidx)
		{
			if (!simGroupMap.hasSpace( sidx)) continue;

			// For each element calculate the 8 closest neighbours, that
			// are not yet assigned to a group of this sample:
			FixedSizePrioQueue<ProxElem,8> proxlist;

			SimRelationMap::Row row = simrelmap.row( sidx);
			SimRelationMap::Row::const_iterator ri = row.begin(), re = row.end();
			for (; ri != re; ++ri)
			{
				if (simGroupMap.hasSpace( ri.col()) && !simGroupMap.shares( sidx, ri.col()))
				{
					proxlist.push( ProxElem( *ri, ri.col()));
				}
			}
			// For each of the max 8 closest neighbours, check, if we can join it to an 
			// existing group, or create a new group:
			std::vector<ProxElem>::const_iterator pi = proxlist.begin(), pe = proxlist.end();
			for (; pi != pe && simGroupMap.hasSpace( sidx); ++pi)
			{
				if (!simGroupMap.hasSpace( pi->sampleidx)) continue;

				// Try to find a group the visited sample belongs to that is closer to the
				// found candidate than the visited sample:
				Index bestmatch_simgroup = 0;
				unsigned short bestmatch_dist = pi->dist;
				std::vector<Index> simgroups = simGroupMap.getElements( sidx);
				std::vector<Index>::const_iterator gi = simgroups.begin(), ge = simgroups.end();
				for (; gi != ge; ++gi)
				{
					GroupInstanceMap::const_iterator groupinst_slot = groupInstanceMap.find( *gi);
					if (groupinst_slot == groupInstanceMap.end()) throw strus::runtime_error(_TXT("illegal reference in group map"));
					GroupInstanceList::const_iterator groupinst = groupinst_slot->second;

					// The new element is closer to a group already existing, then add it there
					if (samplear[ pi->sampleidx].near( groupinst->gencode(), bestmatch_dist))
					{
						bestmatch_simgroup = groupinst->id();
						bestmatch_dist = samplear[ pi->sampleidx].dist( groupinst->gencode());
						break;
					}
				}
				if (bestmatch_simgroup)
				{
					// ...if we found such a group, we try to add the candidate there instead:
					if (!tryAddGroupMember( bestmatch_simgroup, pi->sampleidx, groupInstanceMap, simGroupMap, samplear, m_descendants, m_mutations))
					{
						bestmatch_simgroup = 0;
					}
				}
				if (!bestmatch_simgroup)
				{
					// ...if we did not find such a group we found a new one with the two
					// elements as members:
					
					SimGroup newgroup( samplear, sidx, pi->sampleidx, groupIdAllocator.alloc());
					newgroup.mutate( samplear, m_descendants, m_mutations);
					simGroupMap.insert( pi->sampleidx, newgroup.id());
					simGroupMap.insert( sidx, newgroup.id());
					groupInstanceList.push_back( newgroup);
					GroupInstanceList::iterator enditr = groupInstanceList.end();
					groupInstanceMap[ newgroup.id()] = --enditr;
				}
			}
		}
		// Go through all groups and try to make elements jump to neighbour groups and try
		// to unify groups:
		GroupInstanceList::iterator gi = groupInstanceList.begin(), ge = groupInstanceList.end();
		for (; gi != ge; ++gi)
		{
			// Build the set of neighbour groups:
			std::set<Index> neighbour_groups;
			std::vector<SampleIndex>::const_iterator
				mi = gi->members().begin(), me = gi->members().end();
			for (; mi != me; ++mi)
			{
				std::vector<Index> simgroups = simGroupMap.getElements( *mi);
				std::vector<Index>::const_iterator si = simgroups.begin(), se = simgroups.end();
				for (; si != se; ++si)
				{
					neighbour_groups.insert( *si);
				}
			}
			// Go through all neighbour groups that are in a distance closer to eqdist
			// and delete them if the members are all within the group:
			std::set<Index>::const_iterator ni = neighbour_groups.begin(), ne = neighbour_groups.end();
			for (; ni != ne; ++ni)
			{
				GroupInstanceMap::iterator sim_gi_slot = groupInstanceMap.find( *ni);
				if (sim_gi_slot == groupInstanceMap.end()) throw strus::runtime_error(_TXT("illegal reference in group map"));
				GroupInstanceList::iterator sim_gi = sim_gi_slot->second;

				if (sim_gi->gencode().near( gi->gencode(), m_eqdist))
				{
					// Try to swallow members in sim_gi as long as it is in eqdist:
					std::vector<SampleIndex>::const_iterator
						sim_mi = sim_gi->members().begin(), sim_me = sim_gi->members().end();
					for (; sim_mi != sim_me; ++sim_mi)
					{
						if (!gi->isMember( *sim_mi))
						{
							// Add member of sim_gi to gi:
							gi->addMember( *sim_mi);
							simGroupMap.insert( *sim_mi, gi->id());
							gi->mutate( samplear, m_descendants, m_mutations);
							if (!sim_gi->gencode().near( gi->gencode(), m_eqdist))
							{
								break;
							}
						}
					}
					if (sim_mi == sim_me)
					{
						// ... delete the neighbour group sim_gi where all elements 
						// can be added to gi and it is still in eqdist:
						removeGroup( sim_gi->id(), groupIdAllocator, groupInstanceList, groupInstanceMap, simGroupMap);
						sim_gi = groupInstanceList.end();
					}
				}
				if (sim_gi != groupInstanceList.end() && sim_gi->gencode().near( gi->gencode(), m_simdist))
				{
					// Try add members of gi to sim_gi that are similar to sim_gi:
					std::vector<SampleIndex>::const_iterator
						mi = gi->members().begin(), me = gi->members().end();
					for (; mi != me; ++mi)
					{
						if (sim_gi->gencode().near( samplear[*mi], m_simdist)
						&&  !sim_gi->isMember( *mi))
						{
							tryAddGroupMember( sim_gi->id(), *mi, groupInstanceMap, simGroupMap, samplear, m_descendants, m_mutations);
						}
					}
				}
			}
		}
		// Mutation step for all groups and dropping of elements that got too far away from the
		// representants genom:
		gi = groupInstanceList.begin(), ge = groupInstanceList.end();
		for (; gi != ge; ++gi)
		{
			gi->mutate( samplear, m_descendants, m_mutations);

			std::vector<SampleIndex>::const_iterator
				mi = gi->members().begin(), me = gi->members().end();
			for (; mi != me; ++mi)
			{
				if (!gi->gencode().near( samplear[ *mi], m_simdist))
				{
					// Dropped members that got too far out of the group:
					gi->removeMember( *mi);
					simGroupMap.remove( *mi, gi->id());
					if (gi->members().size() < 2)
					{
						// Delete group that lost too many members:
						removeGroup( gi->id(), groupIdAllocator, groupInstanceList, groupInstanceMap, simGroupMap);
					}
				}
			}
		}
	}

	// Build the result:
	std::vector<SimHash> rt;
	GroupInstanceList::const_iterator gi = groupInstanceList.begin(), ge = groupInstanceList.end();
	for (; gi != ge; ++gi)
	{
		rt.push_back( gi->gencode());
	}
	return rt;
}



