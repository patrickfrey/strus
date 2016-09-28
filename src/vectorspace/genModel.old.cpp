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
#include <sstream>
#include <limits>
#include <algorithm>

using namespace strus;
#undef STRUS_LOWLEVEL_DEBUG

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

	unsigned int nofGroupsAllocated() const
	{
		return m_cnt - m_freeList.size();
	}

private:
	Index m_cnt;
	std::vector<strus::Index> m_freeList;
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
				if (pidx != sidx && si->near( *pi, simdist))
				{
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "declare similarity " << sidx << " ~ " << pidx << std::endl;
#endif
					simrellist.push_back( SimRelationMap::Element( sidx, pidx, si->dist( *pi)));
				}
			}
		}
	}
	return SimRelationMap( simrellist, samplear.size());
}

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
	if (group_slot == groupInstanceMap.end()) throw strus::runtime_error(_TXT("illegal reference in group map (removeGroup): %u"), group_id);
	GroupInstanceList::iterator group_iter = group_slot->second;
	if (group_iter->id() != group_id) throw strus::runtime_error(_TXT("illegal reference in group list"));
	groupInstanceMap.erase( group_slot);

	SimGroup::const_iterator
		mi = group_iter->begin(), me = group_iter->end();
	for (; mi != me; ++mi)
	{
		simGroupMap.remove( *mi, group_id);
	}
	groupIdAllocator.free( group_id);
	groupInstanceList.erase( group_iter);
}

static unsigned int age_mutations( const SimGroup& group, unsigned int maxage, unsigned int conf_mutations)
{
	return (conf_mutations * (maxage - std::min( maxage, group.age()))) / maxage;
}

static unsigned int age_mutation_votes( const SimGroup& group, unsigned int maxage, unsigned int conf_votes)
{
	return conf_votes * (std::min( group.age(), maxage) / maxage) + 1;
}

static bool tryAddGroupMember( const Index& group_id, std::size_t newmember,
				GroupInstanceMap& groupInstanceMap,
				SimGroupMap& simGroupMap, const std::vector<SimHash>& samplear,
				unsigned int descendants, unsigned int mutations, unsigned int votes,
				unsigned int maxage)
{
	GroupInstanceMap::iterator group_slot = groupInstanceMap.find( group_id);
	if (group_slot == groupInstanceMap.end()) throw strus::runtime_error(_TXT("illegal reference in group map (tryAddGroupMember): %u"), group_id);
	GroupInstanceList::iterator group_inst = group_slot->second;

	SimGroup newgroup( *group_inst);
	newgroup.addMember( newmember);
	newgroup.mutate( samplear, descendants, age_mutations( newgroup, maxage, mutations), age_mutation_votes( newgroup, maxage, votes));
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

///\brief Eval the group closest to a sample that is closer than a min dist
static Index getSampleClosestSimGroup(
				std::size_t main_sampleidx, std::size_t search_sampleidx, 
				unsigned short min_dist,
				const GroupInstanceList& groupInstanceList,
				const GroupInstanceMap& groupInstanceMap,
				const SimGroupMap& simGroupMap,
				const std::vector<SimHash>& samplear)
{
	Index rt = 0;
	SimGroupMap::const_node_iterator gi = simGroupMap.node_begin( main_sampleidx), ge = simGroupMap.node_end( main_sampleidx);
	for (; gi != ge; ++gi)
	{
		GroupInstanceMap::const_iterator group_slot = groupInstanceMap.find( *gi);
		if (group_slot == groupInstanceMap.end())
		{
			throw strus::runtime_error(_TXT("illegal reference in group map (getSampleClosestSimGroup): %u"), *gi);
		}
		GroupInstanceList::const_iterator group_inst = group_slot->second;

		// The new element is closer to a group already existing, then add it there
		if (samplear[ search_sampleidx].near( group_inst->gencode(), min_dist))
		{
			rt = group_inst->id();
			min_dist = samplear[ search_sampleidx].dist( group_inst->gencode());
			break;
		}
	}
	return rt;
}

static bool findClosestFreeSample( std::size_t& res_sampleidx, unsigned short& res_dist, std::size_t sampleidx, const SimGroupMap& simGroupMap, const SimRelationMap& simrelmap)
{
	res_sampleidx = 0;
	res_dist = std::numeric_limits<unsigned short>::max();

	SimRelationMap::Row row = simrelmap.row( sampleidx);
	SimRelationMap::Row::const_iterator ri = row.begin(), re = row.end();
	for (; ri != re; ++ri)
	{
		if (simGroupMap.hasSpace( ri.col()) && *ri < res_dist && !simGroupMap.shares( sampleidx, ri.col()))
		{
			res_dist = *ri;
			res_sampleidx = ri.col();
		}
	}
	return (res_dist < std::numeric_limits<unsigned short>::max());
}

#ifdef STRUS_LOWLEVEL_DEBUG
static void checkSimGroupStructures(
				const GroupInstanceList& groupInstanceList,
				const GroupInstanceMap& groupInstanceMap,
				const SimGroupMap& simGroupMap,
				std::size_t nofSamples)
{
	std::cerr << "check structures:" << std::endl;
	simGroupMap.check();
	std::ostringstream errbuf;
	bool haserr = false;

	GroupInstanceList::const_iterator gi = groupInstanceList.begin(), ge = groupInstanceList.end();
	for (; gi != ge; ++gi)
	{
		std::cerr << "group " << gi->id() << " members = ";
		SimGroup::const_iterator mi = gi->begin(), me = gi->end();
		for (unsigned int midx=0; mi != me; ++mi,++midx)
		{
			if (midx) std::cerr << ", ";
			std::cerr << *mi;
			if (!simGroupMap.contains( *mi, gi->id()))
			{
				errbuf << "missing element group relation in simGroupMap: " << *mi << " IN " << gi->id() << std::endl;
				haserr = true;
			}
		}
		std::cerr << std::endl;
		gi->check();

		GroupInstanceMap::const_iterator gi_slot = groupInstanceMap.find( gi->id());
		if (gi_slot == groupInstanceMap.end())
		{
			errbuf << "missing entry in sim group map (check structures): " << gi->id() << std::endl;
			haserr = true;
		}
	}
	std::size_t si=0, se=nofSamples;
	for (; si != se; ++si)
	{
		SimGroupMap::const_node_iterator ni = simGroupMap.node_begin( si), ne = simGroupMap.node_end( si);
		unsigned int nidx=0;
		if (ni != ne) std::cerr << "sample " << si << " groups = ";
		for (; ni != ne; ++ni,++nidx)
		{
			if (nidx) std::cerr << ", ";
			std::cerr << *ni;

			GroupInstanceMap::const_iterator gi_slot = groupInstanceMap.find( *ni);
			if (gi_slot == groupInstanceMap.end())
			{
				errbuf << "entry not found in group instance map (check structures): " << *ni << std::endl;
				haserr = true;
			}
			GroupInstanceList::const_iterator gi = gi_slot->second;
			if (!gi->isMember( si))
			{
				errbuf << "illegal entry in sim group map (check structures): expected " << si << " to be member of group " << gi->id() << std::endl;
				haserr = true;
			}
		}
		if (nidx) std::cerr << std::endl;
	}
	if (haserr)
	{
		throw std::runtime_error( errbuf.str());
	}
}
#endif

std::vector<SimHash> GenModel::run( const std::vector<SimHash>& samplear) const
{
	GroupIdAllocator groupIdAllocator;					// Allocator of group ids
	GroupInstanceList groupInstanceList;					// list of similarity group representants
	GroupInstanceMap groupInstanceMap;					// map indices to group representant list iterators
	SimGroupMap simGroupMap( samplear.size());				// map of sample idx to group idx
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "build similarity relation map" << std::endl;
#endif
	SimRelationMap simrelmap( getSimRelationMap( samplear, m_simdist));	// similarity relation map of the list of samples
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "got similarity relation map:" << std::endl << simrelmap.tostring() << std::endl;
#endif
	// Do the iterations of creating new individuals
	unsigned int iteration=0;
	for (; iteration != m_iterations; ++iteration)
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "GenModel::run iteration " << iteration << std::endl;
		checkSimGroupStructures( groupInstanceList, groupInstanceMap, simGroupMap, samplear.size());

		std::cerr << "existing groups:";
		GroupInstanceList::iterator li = groupInstanceList.begin(), le = groupInstanceList.end();
		for (; li != le; ++li)
		{
			std::cerr << " " << li->id();
		}
		std::cerr << std::endl;
		std::cerr << "create new groups" << std::endl;
#endif
		// Go through all elements and try to create new groups with the closest free neighbours:
		std::vector<SimHash>::const_iterator si = samplear.begin(), se = samplear.end();
		for (std::size_t sidx=0; si != se; ++si,++sidx)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "visit sample " << sidx << std::endl;
#endif
			if (!simGroupMap.hasSpace( sidx)) continue;

			// Find the closest neighbour, that is not yet in a group with this sample:
			std::size_t neighbour_sampleidx;
			unsigned short neighbour_dist;
			if (findClosestFreeSample( neighbour_sampleidx, neighbour_dist, sidx, simGroupMap, simrelmap))
			{
				// Try to find a group the visited sample belongs to that is closer to the
				// found candidate than the visited sample:
				Index bestmatch_simgroup = getSampleClosestSimGroup( sidx, neighbour_sampleidx, neighbour_dist, groupInstanceList, groupInstanceMap, simGroupMap, samplear);
				if (bestmatch_simgroup)
				{
					// ...if we found such a group, we try to add the candidate there instead:
					if (tryAddGroupMember( bestmatch_simgroup, neighbour_sampleidx, groupInstanceMap, simGroupMap, samplear, m_descendants, m_mutations, m_votes, m_maxage))
					{
#ifdef STRUS_LOWLEVEL_DEBUG
						std::cerr << "add new member " << neighbour_sampleidx << " to closest group " << bestmatch_simgroup << std::endl;
#endif
						bestmatch_simgroup = 0;
					}
				}
				if (!bestmatch_simgroup)
				{
					// ...if we did not find such a group we found a new one with the two
					// elements as members:
					SimGroup newgroup( samplear, sidx, neighbour_sampleidx, groupIdAllocator.alloc());
					newgroup.mutate( samplear, m_descendants, age_mutations( newgroup, m_maxage, m_mutations), age_mutation_votes( newgroup, m_maxage, m_votes));
					simGroupMap.insert( neighbour_sampleidx, newgroup.id());
					simGroupMap.insert( sidx, newgroup.id());
					groupInstanceList.push_back( newgroup);
					GroupInstanceList::iterator enditr = groupInstanceList.end();
					groupInstanceMap[ newgroup.id()] = --enditr;
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cerr << "create new group " << newgroup.id() << " with members " << sidx << " and " << neighbour_sampleidx << std::endl;
#endif
				}
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "find neighbour groups out of " << groupIdAllocator.nofGroupsAllocated() << std::endl;
#endif
		// Go through all groups and try to make elements jump to neighbour groups and try
		// to unify groups:
		GroupInstanceList::iterator gi = groupInstanceList.begin(), ge = groupInstanceList.end();
		for (; gi != ge; ++gi)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "visit group " << gi->id() << std::endl;
#endif
			// Build the set of neighbour groups:
			std::set<Index> neighbour_groups;
			SimGroup::const_iterator mi = gi->begin(), me = gi->end();
			for (; mi != me; ++mi)
			{
				SimGroupMap::const_node_iterator si = simGroupMap.node_begin(*mi), se = simGroupMap.node_end(*mi);
				for (; si != se; ++si)
				{
					if (*si != gi->id())
					{
#ifdef STRUS_LOWLEVEL_DEBUG
						std::cerr << "found neighbour group " << *si << " member sample " << *mi << std::endl;
#endif
						neighbour_groups.insert( *si);
					}
				}
			}
			// Go through all neighbour groups that are in a distance closer to eqdist
			// and delete them if the members are all within the group:
			std::set<Index>::const_iterator ni = neighbour_groups.begin(), ne = neighbour_groups.end();
			for (; ni != ne; ++ni)
			{
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "found neighbour group " << *ni << std::endl;
#endif
				GroupInstanceMap::iterator sim_gi_slot = groupInstanceMap.find( *ni);
				if (sim_gi_slot == groupInstanceMap.end()) throw strus::runtime_error(_TXT("illegal reference in group map (join eqdist groups): %u"), *ni);
				GroupInstanceList::iterator sim_gi = sim_gi_slot->second;

				if (sim_gi->gencode().near( gi->gencode(), m_eqdist))
				{
					// Try to swallow members in sim_gi as long as it is in eqdist:
					SimGroup::const_iterator 
						sim_mi = sim_gi->begin(), sim_me = sim_gi->end();
					for (; sim_mi != sim_me; ++sim_mi)
					{
						if (!gi->isMember( *sim_mi))
						{
							if (!simGroupMap.hasSpace( *sim_mi)) break;
							// Add member of sim_gi to gi:
							gi->addMember( *sim_mi);
							simGroupMap.insert( *sim_mi, gi->id());
							gi->mutate( samplear, m_descendants, age_mutations( *gi, m_maxage, m_mutations), age_mutation_votes( *gi, m_maxage, m_votes));
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
#ifdef STRUS_LOWLEVEL_DEBUG
						std::cerr << "remove group (swallowed by another group) " << sim_gi->id() << std::endl;
#endif
						if (sim_gi->fitness( samplear) < gi->fitness( samplear))
						{
							removeGroup( sim_gi->id(), groupIdAllocator, groupInstanceList, groupInstanceMap, simGroupMap);
						}
						sim_gi = groupInstanceList.end();
					}
				}
				if (sim_gi != groupInstanceList.end() && gi->gencode().near( sim_gi->gencode(), m_simdist))
				{
					// Try add one member of sim_gi to gi that is similar to sim_gi:
					SimGroup::const_iterator mi = sim_gi->begin(), me = sim_gi->end();
					for (; mi != me; ++mi)
					{
						if (gi->gencode().near( samplear[*mi], m_simdist) && !gi->isMember( *mi))
						{
							if (simGroupMap.hasSpace( *mi))
							{
								if (tryAddGroupMember( gi->id(), *mi, groupInstanceMap, simGroupMap, samplear, m_descendants, m_mutations, m_votes, m_maxage))
								{
									break;
								}
							}
						}
					}
				}
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "start mutation step" << std::endl;
#endif
		// Mutation step for all groups and dropping of elements that got too far away from the
		// representants genom:
		gi = groupInstanceList.begin(), ge = groupInstanceList.end();
		for (; gi != ge; ++gi)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "visit group " << gi->id() << std::endl;
#endif
			gi->mutate( samplear, m_descendants, age_mutations( *gi, m_maxage, m_mutations), age_mutation_votes( *gi, m_maxage, m_votes));

			SimGroup::const_iterator mi = gi->begin(), me = gi->end();
			for (std::size_t midx=0; mi != me; ++mi,++midx)
			{
				if (!gi->gencode().near( samplear[ *mi], m_simdist))
				{
					// Dropped members that got too far out of the group:
					std::size_t member = *mi;
					mi = gi->removeMemberItr( mi);
					--mi;
					--midx;
					simGroupMap.remove( member, gi->id());
				}
			}
			if (gi->size() < 2)
			{
				// Delete group that lost too many members:
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "remove group (too few members left) " << gi->id() << std::endl;
#endif
				Index gid = gi->id();
				++gi;
				removeGroup( gid, groupIdAllocator, groupInstanceList, groupInstanceMap, simGroupMap);
				--gi;
			}
		}
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "build the result" << std::endl;
#endif
	// Build the result:
	std::vector<SimHash> rt;
	GroupInstanceList::const_iterator gi = groupInstanceList.begin(), ge = groupInstanceList.end();
	for (; gi != ge; ++gi)
	{
		rt.push_back( gi->gencode());
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "got " << rt.size() << " categories" << std::endl;
	gi = groupInstanceList.begin(), ge = groupInstanceList.end();
	for (; gi != ge; ++gi)
	{
		std::cerr << "category " << gi->id() << ": ";
		SimGroup::const_iterator mi = gi->begin(), me = gi->end();
		for (unsigned int midx=0; mi != me; ++mi,++midx)
		{
			if (midx) std::cerr << ", ";
			std::cerr << *mi;
		}
		std::cerr << std::endl;
	}
#endif
	return rt;
}

std::string GenModel::tostring() const
{
	std::ostringstream rt;
	rt << "simdist=" << m_simdist << std::endl
		<< ", eqdist=" << m_eqdist << std::endl
		<< ", mutations=" << m_mutations << std::endl
		<< ", votes=" << m_votes << std::endl
		<< ", descendants=" << m_descendants << std::endl
		<< ", maxage=" << m_maxage << std::endl
		<< ", iterations=" << m_iterations << std::endl;
	return rt.str();
}

