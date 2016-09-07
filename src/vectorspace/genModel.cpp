/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for breeding good representants of similarity classes with help of genetic algorithms
#include "genModel.hpp"
#include "random.hpp"
#include "private/internationalization.hpp"
#include <ctime>
#include <cmath>
#include <iostream>

using namespace strus;
#define STRUS_LOWLEVEL_DEBUG

static Random g_random;

static void selectCandidates(
		std::set<SampleIndex>& visited,
		std::set<SampleIndex>& selected,
		SampleIndex rootidx,
		unsigned int depth,
		const SimRelationMap& simrelmap)
{
	visited.insert( rootidx);
	selected.insert( rootidx);

	SimRelationMap::Row row = simrelmap.row( rootidx);
	SimRelationMap::Row::const_iterator ri = row.begin(), re = row.end();
	for (; ri != re; ++ri)
	{
		if (visited.find( *ri) == visited.end())
		{
			if (depth > 0)
			{
				selectCandidates( visited, selected, *ri, depth-1, simrelmap);
			}
		}
	}
}


std::vector<SimHash> GenModel::run( const std::vector<SimHash>& samplear) const
{
	typedef std::list<SimGroup> GroupList;
	GroupList groupList;			// list of similarity group representants
	typedef std::map<strus::Index,GroupList::iterator> GroupMap;
	GroupMap groupMap;			// map of group indices to group
	Index groupcnt=0;			// counter for creating new group handles
	SimRelationMap simrelmap;		// similarity relation map of the list of samples
	typedef std::multimap<std::size_t,strus::Index> SampleIndexToGroupMap;
	SampleIndexToGroupMap sampleidx2groupmap;

	// Calculate similarity map:
	std::vector<SimHash>::const_iterator si = samplear.begin(), se = samplear.end();
	for (std::size_t sidx=0; si != se; ++si,++sidx)
	{
		std::vector<SimHash>::const_iterator pi = samplear.begin();
		for (std::size_t pidx=0; pi != si; ++pi,++pidx)
		{
			if (si->near( *pi, m_simdist))
			{
				simrelmap.defineRelation( sidx, pidx);
			}
		}
	}

	// Build the groups:
	si = samplear.begin(), se = samplear.end();
	SampleIndex sidx = 0;
	for (; si != se; ++si,++sidx)
	{
		std::set<SampleIndex> visited;
		std::set<SampleIndex> selected;
		selectCandidates( visited, selected, sidx, m_depth, simrelmap);
		std::vector<SampleIndex> local_samplear;
		local_samplear.insert( local_samplear.end(), selected.begin(), selected.end());

		// Get all groups that are somehow referenced by the selected local indices as base
		std::vector<SampleIndex>::const_iterator
			li = local_samplear.begin(), le = local_samplear.end();
		for (; li != le; ++li)
		{
			SampleIndexToGroupMap::const_iterator gi = sampleidx2groupmap.find( *li);
			if (gi != sampleidx2groupmap.end())
			{
				
			}
		}
	}

	// Build the result:
	std::vector<SimHash> rt;
	return rt;
}



