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

/// \brief Create a summarizer for getting all positions of the matches in document field
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerListMatches( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for accumulating weights of contents (taken from the forward index) referenced by variable 
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerAccumulateMatches( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the value of a document attribute
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerAttribute( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the value of a document meta data element
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerMetaData( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the whole forward index of a given type for a document field
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerContent( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the best weighted passage of a document field
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerMatchPhrase( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for weighting neighbour features of the matches
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerAccumulateNear( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the headers of the content fields covering a document field
/// \return the summarizer reference (with ownership)
SummarizerFunctionInterface* createSummarizerStructureHeader( ErrorBufferInterface* errorhnd_);

}//namespace
#endif

