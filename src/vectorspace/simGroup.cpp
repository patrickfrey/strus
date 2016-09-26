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
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <algorithm>

using namespace strus;

static Random g_random;

SimGroup::SimGroup( const std::vector<SimHash>& samplear, std::size_t m1, std::size_t m2, const Index& id_)
	:m_id(id_),m_gencode(),m_age(0),m_members()
{
	if (m1 == m2) throw strus::runtime_error(_TXT("illegal group creations (two init members are duplicates)"));
	if (m1 > m2) std::swap( m1, m2);
	m_members.push_back( m1);
	m_members.push_back( m2);
	m_gencode = inithash( samplear);
}

void SimGroup::setGencode( const SimHash& gc)
{
	m_gencode = gc;
	++m_age;
}

std::size_t SimGroup::upperBound( const SampleIndex& idx) const
{
	std::vector<SampleIndex>::const_iterator mi = m_members.begin(), me = m_members.end();
	std::size_t midx = 0;
	for (; mi != me; ++mi,++midx)
	{
		if (*mi >= idx) break;
	}
	return midx;
}

bool SimGroup::addMember( const SampleIndex& idx)
{
	std::size_t midx = upperBound( idx);
	if (midx == m_members.size())
	{
		m_members.push_back( idx);
	}
	else
	{
		if (m_members[ midx] == idx) return false;
		m_members.push_back( idx);
		std::vector<SampleIndex>::reverse_iterator ri = m_members.rbegin(), re = m_members.rbegin() + m_members.size() - midx - 1;
		for (; ri != re; ++ri)
		{
			*ri = *(ri+1);
		}
		*ri = idx;
	}
	m_age -= m_age/3;
	return true;
}

bool SimGroup::removeMember( const SampleIndex& idx)
{
	std::size_t midx = upperBound( idx);
	if (midx == m_members.size() || m_members[ midx] != idx) return false;
	std::vector<SampleIndex>::iterator mi = m_members.begin()+midx, me = m_members.end()-1;
	for (; mi != me; ++mi)
	{
		*mi = *(mi+1);
	}
	m_members.pop_back();
	return true;
}

void SimGroup::check() const
{
	std::vector<SampleIndex>::const_iterator mi = m_members.begin(), me = m_members.end()-1;
	for (; mi < me; ++mi)
	{
		if (*mi >= *(mi+1)) throw strus::runtime_error(_TXT("consistency check failed for SimGroup"));
	}
}

bool SimGroup::isMember( const SampleIndex& idx) const
{
	std::size_t midx = upperBound( idx);
	return midx < m_members.size() && m_members[ midx] == idx;
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
		double dist = genom.dist( samplear[ *mi]);
		sqrsum += dist * dist;
	}
	return pow_uint( 1.0 + 1.0 / std::sqrt( sqrsum / m_members.size()), m_members.size()) - 1.0;
}

SimHash SimGroup::kernel( const std::vector<SimHash>& samplear) const
{
	std::vector<SampleIndex>::const_iterator si = members().begin(), se = members().end();
	if (si == se) return gencode();

	SimHash first( samplear[ *si]);
	SimHash rt( first.size(), true);

	for (++si; si != se; ++si)
	{
		rt &= ~(first ^ samplear[ *si]);
	}
	return rt;
}

SimHash SimGroup::mutation( const std::vector<SimHash>& samplear, unsigned int mutations) const
{
	if (m_members.size() < 2) return gencode();

	// Calculate 'kernel' = the set of all elements equal for all members. These cannot be mutated:
	SimHash kn = kernel( samplear);

	SimHash rt( m_gencode);
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
			if (samplear[ m_members[ memberidx]][ mutidx])
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

SimHash SimGroup::inithash( const std::vector<SimHash>& samplear) const
{
	if (m_members.size() == 0) return gencode();

	// Calculate 'kernel' = the set of all elements equal for all members. These cannot be mutated:
	SimHash kn = kernel( samplear);
	SimHash rnd( SimHash::randomHash( kn.size(), g_random.get( 0, std::numeric_limits<unsigned int>::max())));
	// All elements belonging to the kernel are taken from the first element the others chosen randomly:
	SimHash rt( (~kn & rnd) |(kn & samplear[ m_members[0]]) );
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
	if (selected >= 0)
	{
		setGencode( descendantlist[ selected]);
	}
}

