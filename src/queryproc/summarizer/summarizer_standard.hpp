/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Library providing some standard summarizers
#ifndef _STRUS_SUMMARIZER_STANDARD_LIB_HPP_INCLUDED
#define _STRUS_SUMMARIZER_STANDARD_LIB_HPP_INCLUDED

namespace strus {

/// \brief Forward declaration
class SummarizerFunctionInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Create a summarizer for getting all positions of the matches in document
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerListMatches( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the value of a document attribute
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerAttribute( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the value of a document meta data element
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerMetaData( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the best phrases of matches
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerMatchPhrase( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the values (in the forward index) of referenced variables of all matches
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerMatchVariables( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for accumulating weights of contents (taken from the forward index) referenced by variable 
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerAccumulateVariable( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for weighting links near matches
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerAccumulateNear( ErrorBufferInterface* errorhnd_);

}//namespace
#endif

