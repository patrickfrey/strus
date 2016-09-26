/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for a map of sample indices to similarity groups they are members of
#include "simGroupMap.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <cstring>

using namespace strus;

bool SimGroupMap::contains( const Index& idx, const Index& groupidx) const
{
	const Node& nd = m_nodear[ idx];
	unsigned int ii=0;
	for (; ii<NofNodeBranches && nd.groupidx[ii] && nd.groupidx[ii] < groupidx; ++ii){}
	if (ii==NofNodeBranches) return false;
	return (nd.groupidx[ii] == groupidx);
}

bool SimGroupMap::shares( const Index& idx1, const Index& idx2) const
{
	unsigned int i1=0, i2=0;
	const Node& nd1 = m_nodear[ idx1];
	const Node& nd2 = m_nodear[ idx2];
	while (i1<NofNodeBranches && i2<NofNodeBranches)
	{
		if (nd1.groupidx[i1] < nd2.groupidx[i2])
		{
			++i1;
		}
		else if (nd1.groupidx[i1] > nd2.groupidx[i2])
		{
			++i2;
		}
		else
		{
			return (nd1.groupidx[i1] != 0);
		}
	}
	return false;
}

bool SimGroupMap::remove( const Index& idx, const Index& groupidx)
{
	Node& nd = m_nodear[ idx];
	unsigned int ii=0;
	for (; ii<NofNodeBranches && nd.groupidx[ii] && nd.groupidx[ii] < groupidx; ++ii){}
	if (ii==NofNodeBranches) return false;
	if (nd.groupidx[ii] == groupidx)
	{
		for (; ii<NofNodeBranches-1 && nd.groupidx[ii]; ++ii)
		{
			nd.groupidx[ii] = nd.groupidx[ii+1];
		}
		nd.groupidx[ii] = 0;
		return true;
	}
	return false;
}

SimGroupMap::const_node_iterator SimGroupMap::node_end( std::size_t nd) const
{
	const Node& ndrec = m_nodear[ nd];
	std::size_t ii = NofNodeBranches;
	for (; ii>0 && ndrec.groupidx[ii-1]==0; --ii){}
	return const_node_iterator( ndrec.groupidx + ii);
}

SimGroupMap::Node::Node( const Node& o)
{
	std::memcpy( groupidx, o.groupidx, sizeof(groupidx));
}

SimGroupMap::Node::Node()
{
	std::memset( groupidx, 0, sizeof(groupidx));
}

bool SimGroupMap::Node::insert( const Index& gidx)
{
	for (unsigned int ii=0; ii<NofNodeBranches; ++ii)
	{
		if (groupidx[ii] == 0)
		{
			groupidx[ii] = gidx;
			return true;
		}
		else if (groupidx[ii] >= gidx)
		{
			if (groupidx[ii] == gidx) return true;
			unsigned int kk = NofNodeBranches;
			for (; kk>ii && !groupidx[kk-1]; --kk){}
			if (kk == NofNodeBranches)
			{
				throw strus::runtime_error(_TXT("try to insert in full simgroupmap node"));
			}
			for (; kk>ii; --kk)
			{
				groupidx[kk] = groupidx[kk-1];
			}
			groupidx[ii] = gidx;
			return true;
		}
	}
	return false;
}

void SimGroupMap::Node::check() const
{
	unsigned int ii = 1;
	for (; ii<NofNodeBranches; ++ii)
	{
		if (groupidx[ii] == 0) break;
		if (groupidx[ii] < groupidx[ii-1])
		{
			throw strus::runtime_error(_TXT("illegal SimGroupMap::Node (order)"));
		}
	}
	for (; ii<NofNodeBranches; ++ii)
	{
		if (groupidx[ii] != 0)
		{
			throw strus::runtime_error(_TXT("illegal SimGroupMap::Node (eof)"));
		}
	}
}

void SimGroupMap::check() const
{
	std::vector<Node>::const_iterator ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne; ++ni)
	{
		ni->check();
	}
}

