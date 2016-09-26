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
#include "simHash.hpp"
#include "simGroup.hpp"
#include "simRelationMap.hpp"
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
	GenModel( unsigned int simdist_, unsigned int eqdist_, unsigned int mutations_, unsigned int descendants_, unsigned int maxage_, unsigned int iterations_)
		:m_simdist(simdist_),m_eqdist(eqdist_),m_mutations(mutations_),m_descendants(descendants_)
		,m_maxage(maxage_),m_iterations(iterations_){}
	/// \brief Copy constructor
	GenModel( const GenModel& o)
		:m_simdist(o.m_simdist),m_eqdist(o.m_eqdist),m_mutations(o.m_mutations),m_descendants(o.m_descendants)
		,m_maxage(o.m_maxage),m_iterations(o.m_iterations){}

	/// \brief map contents to string in readable form
	std::string tostring() const;

	/// \brief Unsupervised learning of a good group representantion of the sample set passed as argument
	std::vector<SimHash> run( const std::vector<SimHash>& samples) const;

private:
	unsigned int m_simdist;			///< maximal distance to be considered similar
	unsigned int m_eqdist;			///< maximum distance to be considered equal
	unsigned int m_mutations;		///< number of random selected mutation candidates of the non kernel elements of a group
	unsigned int m_descendants;		///< number of descendants of which the fittest is selected
	unsigned int m_maxage;			///< upper bound value used for calculate number of mutations (an older individuum mutates less)
	unsigned int m_iterations;		///< number of iterations
};

}//namespace
#endif


