/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for a map of sample indices to similarity groups they are members of
#ifndef _STRUS_VECTOR_SPACE_MODEL_SIMILAITY_GROUP_MAP_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_SIMILAITY_GROUP_MAP_HPP_INCLUDED
#include "simGroup.hpp"
#include "strus/index.hpp"
#include <map>
#include <cstring>

namespace strus {

/// \brief Structure for storing sample to assigned group relations
class SimGroupMap
{
public:
	explicit SimGroupMap( std::size_t nofNodes)
		:m_nodear( nofNodes, Node()){}
	SimGroupMap( const SimGroupMap& o)
		:m_nodear(o.m_nodear){}

	bool insert( const Index& idx, const Index& groupidx)
		{return m_nodear[idx].insert( groupidx);}
	/// \brief Evaluate if a sample references a group
	bool contains( const Index& idx, const Index& groupidx) const;
	/// \brief Evaluate if the two samples share a group reference
	bool shares( const Index& idx1, const Index& idx2);
	/// \brief Remove sample reference relationship to a group
	bool remove( const Index& idx, const Index& groupidx);
	/// \brief Get the list of elements
	std::vector<Index> getElements( const Index& idx) const;
	/// \brief Evaluate, if there is space left for adding a new relation
	bool hasSpace( const Index& idx) const
		{return m_nodear[idx].groupidx[ NofNodeBranches-1] == 0;}

private:
	enum {NofNodeBranches=8};
	struct Node
	{
		Index groupidx[ NofNodeBranches];

		Node( const Node& o);
		Node();

		bool insert( const Index& gix);
	};
	std::vector<Node> m_nodear;
};

}//namespace
#endif
