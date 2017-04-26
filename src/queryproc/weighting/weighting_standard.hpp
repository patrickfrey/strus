/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

/// \brief Create a weighting function for a SMART schema defined as scalar function on ff,df,N,qf and metadata as string
/// \return the weighting function reference (to dispose with delete)
WeightingFunctionInterface* createWeightingFunctionSmart( ErrorBufferInterface* errorhnd);

/// \brief Create a weighting function as scalar function on the collection size and constants, variables and document metadata as string
/// \return the weighting function reference (to dispose with delete)
WeightingFunctionInterface* createWeightingFunctionScalar( ErrorBufferInterface* errorhnd);

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


