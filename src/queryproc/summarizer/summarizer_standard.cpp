/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Library providing some standard summarizers
#include "summarizer_standard.hpp"
#include "summarizerMetaData.hpp"
#include "summarizerAttribute.hpp"
#include "summarizerMatchPhrase.hpp"
#include "summarizerListMatches.hpp"
#include "summarizerMatchVariables.hpp"
#include "summarizerAccumulateVariable.hpp"
#include "summarizerWeightNeighbours.hpp"

using namespace strus;

SummarizerFunctionInterface* strus::createSummarizerListMatches( ErrorBufferInterface* errorhnd_)
{
	return new SummarizerFunctionListMatches( errorhnd_);
}

SummarizerFunctionInterface* strus::createSummarizerAttribute( ErrorBufferInterface* errorhnd_)
{
	return new SummarizerFunctionAttribute( errorhnd_);
}

SummarizerFunctionInterface* strus::createSummarizerMetaData( ErrorBufferInterface* errorhnd_)
{
	return new SummarizerFunctionMetaData( errorhnd_);
}

SummarizerFunctionInterface* strus::createSummarizerMatchPhrase( ErrorBufferInterface* errorhnd_)
{
	return new SummarizerFunctionMatchPhrase( errorhnd_);
}

SummarizerFunctionInterface* strus::createSummarizerMatchVariables( ErrorBufferInterface* errorhnd_)
{
	return new SummarizerFunctionMatchVariables( errorhnd_);
}

SummarizerFunctionInterface* strus::createSummarizerAccumulateVariable( ErrorBufferInterface* errorhnd_)
{
	return new SummarizerFunctionAccumulateVariable( errorhnd_);
}

SummarizerFunctionInterface* strus::createSummarizerWeightNeighbours( ErrorBufferInterface* errorhnd_)
{
	return new SummarizerFunctionWeightNeighbours( errorhnd_);
}

