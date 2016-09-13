/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for storing similarity group representants (individuals in the genetic algorithm for breeding similarity group representants)
#include "simGroup.hpp"
#include "random.hpp"
#include <algorithm>

using namespace strus;

static Random g_random;

void SimGroup::setGencode( const SimHash& gc)
{
	m_gencode = gc;
	++m_age;
}

void SimGroup::addMember( const SampleIndex& idx)
{
	std::vector<SampleIndex>::const_iterator
		mi = std::find( m_members.begin(), m_members.end(), idx);
	if (mi == m_members.end())
	{
		m_members.push_back( idx);
		m_age -= m_age/3;
	}
}

void SimGroup::removeMember( const SampleIndex& idx)
{
	std::remove( m_members.begin(), m_members.end(), idx);
}

bool SimGroup::isMember( const SampleIndex& idx) const
{
	return (std::find( m_members.begin(), m_members.end(), idx) != m_members.end());
}

double SimGroup::fitness( const std::vector<SimHash>& samplear) const
{
	return fitness( samplear, gencode());
}

double pow_uint( double value, unsigned int exp)
{
	double rt = ((exp & 1) == 1) ? value : 1.0;
	if (exp >= 2)
	{
		rt *= pow_uint( value * value, exp >> 1);
	}
	return rt;
}

double SimGroup::fitness( const std::vector<SimHash>& samplear, const SimHash& genom) const
{
	double sqrsum = 0.0;
	std::vector<SampleIndex>::const_iterator mi = m_members.begin(), me = m_members.end();
	for (; mi != me; ++mi)
	{
		double dist = genom.dist( samplear[ *mi -1]);
		sqrsum += dist * dist;
	}
	return pow_uint( 1.0 + 1.0 / std::sqrt( sqrsum / m_members.size()), m_members.size());
}

SimHash SimGroup::kernel( const std::vector<SimHash>& samplear) const
{
	std::vector<SampleIndex>::const_iterator si = members().begin(), se = members().end();
	if (si == se) return gencode();

	SimHash first( samplear[ *si -1]);
	SimHash rt( first.size(), true);

	for (++si; si != se; ++si)
	{
		rt &= ~(first ^ samplear[ *si -1]);
	}
	return rt;
}

SimHash SimGroup::mutation( const std::vector<SimHash>& samplear, unsigned int mutations) const
{
	if (m_members.size() < 2) return gencode();

	// Calculate 'kernel' = the set of all elements equal for all members. These cannot be mutated:
	SimHash kn = kernel( samplear);

	SimHash rt( gencode());

	unsigned int mi=0, me=mutations;
	for (; mi != me; ++mi)
	{
		unsigned int mutidx = g_random.get( 0, gencode().size());
		if (kn[ mutidx]) continue; //.... do not mutate kernel elements

		// The majority of randomly selected members decide the direction of the mutation
		// With growing age spins with a higher vote are preferred:
		unsigned int true_cnt=0, false_cnt=0;
		std::size_t ci=0, ce=m_members.size();
		if (ce > age()+1) ce = age()+1;
		for (; ci != ce; ++ci)
		{
			SampleIndex memberidx = g_random.get( 0, m_members.size());
			if (samplear[ m_members[ memberidx]-1][ mutidx])
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
			rt.set( mutidx, gencode()[ mutidx]);
		}
	}
	return rt;
}


void SimGroup::mutate( const std::vector<SimHash>& samplear, unsigned int descendants, unsigned int mutations)
{
	std::vector<SimHash> descendantlist;
	descendantlist.reserve( descendants);

	double max_fitness = fitness( samplear);
	int selected = -1;
	std::size_t di=0, de=descendants;
	for (; di != de; ++di)
	{
		descendantlist.push_back( mutation( samplear, mutations));
		double desc_fitness = fitness( samplear, descendantlist.back());
		if (desc_fitness > max_fitness)
		{
			selected = (int)di;
			max_fitness = desc_fitness;
		}
	}
	if (selected)
	{
		setGencode( descendantlist[ di]);
	}
}

