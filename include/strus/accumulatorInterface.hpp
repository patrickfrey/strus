/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_ACCUMULATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_ACCUMULATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>
#include <vector>

namespace strus
{
class WeightingFunctionInterface;
class IteratorInterface;

class AccumulatorInterface
{
public:
	/// \brief Destructor
	virtual ~AccumulatorInterface(){}

	/// \brief Add another source to accumulate result from
	/// \remark Should not be called anymore after the first call of 'nextRank(Index&,double&)'
	virtual void addRanker(
			double factor,
			const std::string& function,
			const std::vector<float>& parameter,
			const IteratorInterface& iterator)=0;

	/// \brief Add another set for document selection for weighting
	/// \remark Not called anymore after the first call of 'nextRank(Index&,double&)'
	virtual void addSelector(
			const IteratorInterface& iterator)=0;

	/// \brief Get the next matching document with its weight
	/// \param[out] docno document number
	/// \param[out] weight calculated document weight that reflects its relevance for the search
	/// \param[out] selectorState returned index of the current selector chosen
	/// \remark The function returns the documents in ascending order per selector set, not weighted by relevance
	virtual bool nextRank( Index& docno, unsigned int& selectorState, double& weight)=0;
};

}//namespace
#endif

