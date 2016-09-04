/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for breeding good representants of similarity classes with help of genetic algorithms
#ifndef _STRUS_VECTOR_SPACE_MODEL_GENMODEL_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_GENMODEL_HPP_INCLUDED
#include "simhash.hpp"
#include "strus/index.hpp"
#include <vector>
#include <list>
#include <set>
#include <map>

namespace strus {

/// \brief Structure for implementing unsupervised learning of SimHash group representants with help of genetic algorithms
class GenModel
{
public:
	/// \brief Constructor
	GenModel( unsigned int simdist_, unsigned int nbdist_, unsigned int mutations_, unsigned int maxage_)
		:m_simdist(simdist_),m_nbdist(nbdist_),m_mutations(mutations_),m_samplesize(0),m_maxage(maxage_)
		,m_timestamp(0),m_grouplist(),m_groupcnt(0),m_samplear()
		,m_neighbourmap(){}
	/// \brief Copy constructor
	GenModel( const GenModel& o)
		:m_simdist(o.m_simdist),m_mutations(o.m_mutations),m_samplesize(o.m_samplesize),m_maxage(o.m_maxage)
		,m_timestamp(o.m_timestamp),m_grouplist(o.m_grouplist),m_groupcnt(o.m_groupcnt),m_samplear(o.m_samplear)
		,m_neighbourmap(o.m_neighbourmap){}

	/// \brief Add sample vector
	void addSample( const SimHash& hash);

private:
	typedef std::size_t SampleIndex;

	class Group
	{
	public:
		Group( strus::Index id_, uint64_t lastModified_, SimHash gencode_)
			:m_id(id_),m_gencode(gencode_),m_lastModified(lastModified_),m_age(0),m_members(){}
		Group( const Group& o)
			:m_id(o.m_id),m_gencode(o.m_gencode),m_lastModified(o.m_lastModified),m_age(o.m_age),m_members(o.m_members){}

		const strus::Index& id() const					{return m_id;}
		const SimHash& gencode() const					{return m_gencode;}
		uint64_t lastModified() const					{return m_lastModified;}
		unsigned int age() const					{return m_age;}
		const std::set<SampleIndex>& members() const			{return m_members;}

		void setGencode( const SimHash& gc, uint64_t timestamp_)	{m_gencode = gc; m_lastModified = timestamp_; ++m_age;}
		void addMember( const SampleIndex& idx, uint64_t timestamp_)	{m_members.insert( idx); m_lastModified = timestamp_; m_age -= m_age/3;}
		void removeMember( const SampleIndex& idx)			{m_members.erase( idx);}

	private:
		strus::Index m_id;			///< feature identifier given to the group
		SimHash m_gencode;			///< genetic code of the group
		uint64_t m_lastModified;		///< last modification timestamp
		unsigned int m_age;			///< virtual value for age of the genom
		std::set<SampleIndex> m_members;	///< members of the group
	};

	void addGroup( const SampleIndex& sampleidx);
	void iteration( Group& group);

private:
	SimHash mutation( Group& group);
	SimHash groupKernel( Group& group);

private:
	unsigned int m_simdist;			///< maximal distance to be considered similar
	unsigned int m_nbdist;			///< maximal distance to be considered neighbours (potentially similar in the future)
	unsigned int m_mutations;		///< number of random selected mutation candidates of the non kernel elements of a group
	unsigned int m_samplesize;		///< number of bits in a sample SimHash
	unsigned int m_maxage;			///< upper bound value used for calculate number of mutations (an older individuum mutates less)
	uint64_t m_timestamp;			///< current timestamp to calculate a last modified value (not used yet)
	std::list<Group> m_grouplist;		///< list of groups
	Index m_groupcnt;			///< Counter for creating new group handles
	std::vector<SimHash> m_samplear;	///< list of samples
	typedef std::multimap<strus::Index,SampleIndex> NeighbourMap;
	NeighbourMap m_neighbourmap;		///< Map of group identifiers to neighbour samples
};

}//namespace
#endif


