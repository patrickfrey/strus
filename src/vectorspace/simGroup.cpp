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
	:m_id(id_),m_gencode(),m_age(0),m_members(),m_nofmembers(2)
{
	if (m1 == m2) throw strus::runtime_error(_TXT("illegal group creations (two init members are duplicates)"));
	if (m1 > m2) std::swap( m1, m2);
	m_members.insert( m1);
	m_members.insert( m2);
	m_gencode = inithash( samplear);
}

void SimGroup::setGencode( const SimHash& gc)
{
	m_gencode = gc;
	++m_age;
}

bool SimGroup::addMember( const SampleIndex& idx)
{
	if (m_members.insert( idx).second)
	{
		++m_nofmembers;
		m_age -= m_age/3;
		return true;
	}
	return false;
}

bool SimGroup::removeMember( const SampleIndex& idx)
{
	if (m_members.erase( idx) > 0)
	{
		--m_nofmembers;
		return true;
	}
	return false;
}

SimGroup::const_iterator SimGroup::removeMemberItr( const_iterator itr)
{
	const_iterator rt = itr;
	++rt;
	m_members.erase( itr);
	return rt;
}

bool SimGroup::isMember( const SampleIndex& idx) const
{
	return m_members.find( idx) != m_members.end();
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
	const_iterator mi = m_members.begin(), me = m_members.end();
	for (; mi != me; ++mi)
	{
		double dist = genom.dist( samplear[ *mi]);
		sqrsum += dist * dist;
	}
	return pow_uint( 1.0 + 1.0 / std::sqrt( sqrsum / m_nofmembers), m_nofmembers) - 1.0;
}

SimHash SimGroup::kernel( const std::vector<SimHash>& samplear) const
{
	const_iterator si = m_members.begin(), se = m_members.end();
	if (si == se) return gencode();

	SimHash first( samplear[ *si]);
	SimHash rt( first.size(), true);

	for (++si; si != se; ++si)
	{
		rt &= ~(first ^ samplear[ *si]);
	}
	return rt;
}

bool SimGroup::mutation_vote( const std::vector<SimHash>& samplear, unsigned int mutidx, unsigned int nofqueries) const
{
	unsigned int true_cnt=0, false_cnt=0;
	unsigned int ci = 0, ce = nofqueries;
	for (; ci != ce; ++ci)
	{
		SampleIndex rnd = g_random.get( 0, samplear.size());
		SampleIndex chosenSampleIdx;
		if (rnd == 0)
		{
			chosenSampleIdx = *m_members.begin();
		}
		else
		{
			std::set<SampleIndex>::const_iterator mi = m_members.upper_bound( rnd);
			if (mi == m_members.end())
			{
				chosenSampleIdx = *m_members.begin();
			}
			else
			{
				chosenSampleIdx = *mi;
			}
		}
		if (samplear[ chosenSampleIdx][ mutidx])
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
		return true;
	}
	else if (true_cnt < false_cnt)
	{
		return false;
	}
	else
	{
		return m_gencode[ mutidx];
	}
}

SimHash SimGroup::mutation( const std::vector<SimHash>& samplear, unsigned int maxNofMutations, unsigned int maxNofVotes) const
{
	if (m_nofmembers < 2) return gencode();

	SimHash rt( m_gencode);
	// Calculate 'kernel' = the set of all elements equal for all members. These cannot be mutated:
	SimHash kn = kernel( samplear);

	unsigned int ki=0, ke=maxNofMutations;
	for (; ki != ke; ++ki)
	{
		unsigned int mutidx = g_random.get( 0, gencode().size());
		if (!kn[ mutidx])
		{
			//.... only mutate non kernel elements
			// The majority of randomly selected members decide the direction of the mutation:
			bool mutval = mutation_vote( samplear, mutidx, maxNofVotes);
			rt.set( mutidx, mutval);
		}
	}
	return rt;
}

SimHash SimGroup::inithash( const std::vector<SimHash>& samplear) const
{
	if (m_nofmembers == 0) return gencode();

	// Calculate 'kernel' = the set of all elements equal for all members. These cannot be mutated:
	SimHash kn = kernel( samplear);
	SimHash rnd( SimHash::randomHash( kn.size(), g_random.get( 0, std::numeric_limits<unsigned int>::max())));
	// All elements belonging to the kernel are taken from the first element the others chosen randomly:
	SimHash rt( (~kn & rnd) |(kn & samplear[ *m_members.begin()]) );
	return rt;
}

void SimGroup::mutate( const std::vector<SimHash>& samplear, unsigned int descendants, unsigned int maxNofMutations, unsigned int maxNofVotes)
{
	std::vector<SimHash> descendantlist;
	descendantlist.reserve( descendants);

	double max_fitness = fitness( samplear);
	int selected = -1;
	std::size_t di=0, de=descendants;
	for (; di != de; ++di)
	{
		descendantlist.push_back( mutation( samplear, maxNofMutations, maxNofVotes));
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

