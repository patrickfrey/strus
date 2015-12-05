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
/// \brief Library providing some standard summarizers
#ifndef _STRUS_SUMMARIZER_STANDARD_LIB_HPP_INCLUDED
#define _STRUS_SUMMARIZER_STANDARD_LIB_HPP_INCLUDED

namespace strus {

/// \brief Forward declaration
class SummarizerFunctionInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Create a summarizer for getting all positions of the matches in document
/// \return the summarizer reference (to dispose with delete)
SummarizerFunctionInterface* createSummarizerListMatches( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the value of a document attribute
/// \return the summarizer reference (to dispose with delete)
SummarizerFunctionInterface* createSummarizerAttribute( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the value of a document meta data element
/// \return the summarizer reference (to dispose with delete)
SummarizerFunctionInterface* createSummarizerMetaData( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the best phrases of matches
/// \return the summarizer reference (to dispose with delete)
SummarizerFunctionInterface* createSummarizerMatchPhrase( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for getting the values (in the forward index) of referenced variables of all matches
/// \return the summarizer reference (to dispose with delete)
SummarizerFunctionInterface* createSummarizerMatchVariables( ErrorBufferInterface* errorhnd_);

/// \brief Create a summarizer for accumulating weights of contents (taken from the forward index) referenced by variable 
/// \return the summarizer reference (to dispose with delete)
SummarizerFunctionInterface* createSummarizerAccumulateVariable( ErrorBufferInterface* errorhnd_);

}//namespace
#endif

