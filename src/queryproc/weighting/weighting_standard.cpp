/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Library providing some standard weighting functions
#include "weighting_standard.hpp"
#include "weightingSmart.hpp"
#include "weightingScalar.hpp"
#include "weightingBM25.hpp"
#include "weightingBM25pff.hpp"
#include "weightingConstant.hpp"
#include "weightingMetadata.hpp"
#include "weightingFrequency.hpp"

using namespace strus;

WeightingFunctionInterface* strus::createWeightingFunctionSmart( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionSmart( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionScalar( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionScalar( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionBm25( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionBM25( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionBm25pff( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionBM25pff( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionConstant( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionConstant( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionMetadata( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionMetadata( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionTermFrequency( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionTermFrequency( errorhnd);
}




