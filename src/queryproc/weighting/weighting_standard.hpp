/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Library providing some standard weighting functions
#ifndef _STRUS_WEIGHTING_STANDARD_LIB_HPP_INCLUDED
#define _STRUS_WEIGHTING_STANDARD_LIB_HPP_INCLUDED

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class WeightingFunctionInterface;

/// \brief Create a weighting function for a schema defined as formula in a string
/// \return the weighting function reference (to dispose with delete)
/// \remark This type of weighting function may not have the ultimate preformance, but it might be used for prototyping
WeightingFunctionInterface* createWeightingFunctionFormula( ErrorBufferInterface* errorhnd);

/// \brief Create a weighting function for the weighting schema BM25
/// \return the weighting function reference (to dispose with delete)
WeightingFunctionInterface* createWeightingFunctionBm25( ErrorBufferInterface* errorhnd);

/// \brief Create a weighting function for the weighting schema BM25pff (BM25 with proximity based ff calculation)
/// \return the weighting function reference (to dispose with delete)
WeightingFunctionInterface* createWeightingFunctionBm25pff( ErrorBufferInterface* errorhnd);

/// \brief Create a weighting function that accumulates a constant for each matching feature in a document
/// \return the weighting function reference (to dispose with delete)
WeightingFunctionInterface* createWeightingFunctionConstant( ErrorBufferInterface* errorhnd);

/// \brief Create a weighting function that returns an element of the metadata as weight
/// \return the weighting function reference (to dispose with delete)
WeightingFunctionInterface* createWeightingFunctionMetadata( ErrorBufferInterface* errorhnd);

/// \brief Create a weighting function that accumulates the feature frequency for each matching feature in a document
/// \return the weighting function reference (to dispose with delete)
WeightingFunctionInterface* createWeightingFunctionTermFrequency( ErrorBufferInterface* errorhnd);

}//namespace
#endif


