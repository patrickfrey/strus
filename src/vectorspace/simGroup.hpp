/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for storing similarity group representants (individuals in the genetic algorithm for breeding similarity group representants)
#ifndef _STRUS_VECTOR_SPACE_MODEL_SIMILAITY_GROUP_REPRESENTANT_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_SIMILAITY_GROUP_REPRESENTANT_HPP_INCLUDED
#include "simHash.hpp"
#include "strus/index.hpp"
#include <vector>
#include <algorithm>
#include <armadillo>

namespace strus {

typedef std::size_t SampleIndex;

/// \brief Structure for storing a representant of a similarity group
class SimGroup
{
public:
	SimGroup( strus::Index id_, const SimHash& gencode_)
		:m_id(id_),m_gencode(gencode_),m_age(0),m_members(){}
	SimGroup( const SimGroup& o)
		:m_id(o.m_id),m_gencode(o.m_gencode),m_age(o.m_age),m_members(o.m_members){}

	const strus::Index& id() const					{return m_id;}
	const SimHash& gencode() const					{return m_gencode;}
	unsigned int age() const					{return m_age;}
	const std::vector<SampleIndex>& members() const			{return m_members;}

	/// \brief Change the genetic code of this individual to a new value
	void setGencode( const SimHash& gc);
	/// \brief Add a new member, if not member yet
	void addMember( const SampleIndex& idx);
	/// \brief Remove a member
	void removeMember( const SampleIndex& idx);

	/// \brief Calculate the fitness of this individual
	/// \param[in] samplear global array of samples the reference system of this individual is based on
	double fitness( const std::vector<SimHash>& samplear) const;

	/// \brief Do a mutation step
	/// \param[in] samplear global array of samples the reference system of this individual is based on
	void mutate( const std::vector<SimHash>& samplear,
			unsigned int selectionCandidates,
			unsigned int maxNofMutations);

private:
	/// \brief Evaluate the fitness of a proposed genom change
	/// \param[in] samplear global array of samples the reference system of this individual is based on
	double fitness( const std::vector<SimHash>& samplear, const SimHash& candidate) const;
	/// \param[in] samplear global array of samples the reference system of this individual is based on
	SimHash kernel( const std::vector<SimHash>& samplear) const;
	/// \param[in] samplear global array of samples the reference system of this individual is based on
	SimHash mutation( const std::vector<SimHash>& samplear, unsigned int maxNofMutations) const;

private:
	strus::Index m_id;			///< feature identifier given to the group
	SimHash m_gencode;			///< genetic code of the group
	unsigned int m_age;			///< virtual value for age of the genom
	std::vector<SampleIndex> m_members;	///< members of the group
};

}//namespace
#endif
