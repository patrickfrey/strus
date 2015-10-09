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
/// \brief Interface for packing/unpacking messages with statistics used for query evaluation to other peer storages.
/// \file peerMessageProcessor.cpp
#include "peerMessageProcessor.hpp"
#include "peerMessageBuilder.hpp"
#include "peerMessageViewer.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

PeerMessageProcessor::PeerMessageProcessor( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_){}

PeerMessageProcessor::~PeerMessageProcessor(){}

PeerMessageViewerInterface* PeerMessageProcessor::createViewer(
			const char* peermsgptr, std::size_t peermsgsize) const
{
	try
	{
		return new PeerMessageViewer( peermsgptr, peermsgsize, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error create peer message viewer: %s"), *m_errorhnd, 0);
}

PeerMessageBuilderInterface* PeerMessageProcessor::createBuilder( const BuilderOptions& options_) const
{
	try
	{
		return new PeerMessageBuilder( (options_.set & BuilderOptions::InsertInLexicalOrder) != 0, options_.maxBlockSize, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error create peer message viewer: %s"), *m_errorhnd, 0);
}


