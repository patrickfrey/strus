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
#include "summarizer_standard.hpp"
#include "summarizerMetaData.hpp"
#include "summarizerAttribute.hpp"
#include "summarizerMatchPhrase.hpp"
#include "summarizerListMatches.hpp"
#include "summarizerMatchVariables.hpp"

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



