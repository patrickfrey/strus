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

bool SimGroupMap::shares( const std::size_t& ndidx1, const std::size_t& ndidx2) const
{
	Index i1=0, i2=0;
	const Node& nd1 = m_nodear[ ndidx1];
	const Node& nd2 = m_nodear[ ndidx2];
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

void SimGroupMap::check() const
{
	std::vector<Node>::const_iterator ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne; ++ni)
	{
		ni->check();
	}
}

SimGroupMap::Node::Node( const Node& o)
{
	std::memcpy( this, &o, sizeof(*this));
}

SimGroupMap::Node::Node()
{
	std::memset( this, 0, sizeof(*this));
}

bool SimGroupMap::Node::remove( const Index& gidx)
{
	Index ii=0;
	for (; ii<size && groupidx[ii] < gidx; ++ii){}
	if (ii==size) return false;
	if (groupidx[ii] == gidx)
	{
		--size;
		for (; ii<size; ++ii)
		{
			groupidx[ii] = groupidx[ii+1];
		}
		groupidx[ii] = 0;
		return true;
	}
	return false;
}

bool SimGroupMap::Node::insert( const Index& gidx)
{
	if (size == NofNodeBranches)
	{
		throw strus::runtime_error(_TXT("try to insert in full simgroupmap node"));
	}
	for (Index ii=0; ii<size; ++ii)
	{
		if (groupidx[ii] >= gidx)
		{
			if (groupidx[ii] == gidx) return false;
			Index kk = ++size;
			for (; kk>ii; --kk)
			{
				groupidx[kk] = groupidx[kk-1];
			}
			groupidx[ii] = gidx;
			return true;
		}
	}
	groupidx[ size++] = gidx;
	return true;
}

bool SimGroupMap::Node::contains( const Index& gidx) const
{
	Index ii=0;
	for (; ii<size && groupidx[ii] < gidx; ++ii){}
	if (ii==size) return false;
	return (groupidx[ii] == gidx);
}

void SimGroupMap::Node::check() const
{
	Index ii = 1;
	for (; ii<size; ++ii)
	{
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


