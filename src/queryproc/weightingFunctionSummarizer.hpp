/*
 * Copyright (c) 2017 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_FROM_WEIGHTING_HPP_INCLUDED
#define _STRUS_SUMMARIZER_FROM_WEIGHTING_HPP_INCLUDED
/// \brief Interface to create a summarizer from a weighting function
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/errorBufferInterface.hpp"

namespace strus
{

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class WeightingFunctionInterface;
/// \brief Forward declaration
class SummarizerFunctionInterface;

SummarizerFunctionInterface*
	createSummarizerFromWeightingFunction(
		const std::string& name_,
		ErrorBufferInterface* errorhnd_,
		const WeightingFunctionInterface* func_);

} //namespace
#endif


