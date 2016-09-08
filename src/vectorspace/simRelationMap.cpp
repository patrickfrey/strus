/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for storing similarity relations
#include "simRelationMap.hpp"
#include <limits>
#include <algorithm>
#include <iostream>
#include <set>

#define STRUS_LOWLEVEL_DEBUG

using namespace strus;

struct HotSpotElement
{
	double weight;
	std::size_t index;

	HotSpotElement( double weight_, std::size_t index_)
		:weight(weight_),index(index_){}
	HotSpotElement( const HotSpotElement& o)
		:weight(o.weight),index(o.index){}

	bool operator <( const HotSpotElement& o) const
	{
		if (weight < o.weight) return true;
		if (weight > o.weight + std::numeric_limits<double>::epsilon()) return false;
		return index < o.index;
	}
	bool operator >( const HotSpotElement& o) const
	{
		if (weight > o.weight) return true;
		if (weight + std::numeric_limits<double>::epsilon() < o.weight) return false;
		return index > o.index;
	}
};

std::vector<std::size_t> SimRelationMap::getHotspotList() const
{
	// Build the connection matrix A:
	std::size_t ri=0, re=m_mat.n_rows;
	arma::umat locations;
	arma::vec values;

	for (; ri != re; ++ri)
	{
		arma::SpMat<unsigned short>::const_row_iterator
			ci = m_mat.begin_row( ri), ce = m_mat.end_row( ri);
		std::size_t nofcols = 0;
		for (; ci != ce; ++ci,++nofcols)
		{
			locations << ri << *ci << arma::endr;
		}
		if (nofcols) continue;
		double weight = 1.0 / nofcols;

		std::size_t ni=0, ne=nofcols;
		for (; ni != ne; ++ni)
		{
			values << weight;
		}
	}
	arma::sp_mat A_( locations, values);

	// Calculate convergence vector with page rank:
	const double d_ = 0.85;
	const unsigned int N_ = m_mat.n_rows;

	enum {NofIterations=15};
	unsigned int ti=0, te=NofIterations;

	arma::vec v_( N_, 1.0 / N_);		// initial vector
	arma::vec e_( N_, (1.0 - d_) / N_);	// vector representing probability to select a new element randomly
	for (; ti != te; ++ti)
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "page rank vector:" << v_ << std::endl;
#endif
		v_ = A_ * v_ * d_ + e_;
	}

	// Build a vector of weighted element references:
	std::vector<HotSpotElement> elemlist;
	arma::vec::const_iterator vi = v_.begin(), ve = v_.end();
	for (std::size_t vidx=0; vi != ve; ++vi,++vidx)
	{
		elemlist.push_back( HotSpotElement( *vi, vidx));
	}
	// Get the element references sorted by descending weight:
	std::sort( elemlist.begin(), elemlist.end(), std::greater<HotSpotElement>());

	// Return the indices of the best selected elements:
	std::vector<std::size_t> rt;
	std::vector<HotSpotElement>::const_iterator
		ei = elemlist.begin(), ee = elemlist.end();
	std::set<std::size_t> visited;
	for (; ei != ee; ++ei)
	{
		if (visited.find( ei->index) == visited.end())
		{
			rt.push_back( ei->index);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "hotspot result weight=" << ei->weight << " " << ei->index << std::endl;
#endif
			visited.insert( ei->index);
			arma::SpMat<unsigned short>::const_row_iterator
				ci = m_mat.begin_row( ri), ce = m_mat.end_row( ri);
			for (; ci != ce; ++ci)
			{
				visited.insert( *ci);
			}
		}
	}
	return rt;
}

