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
/// \file peerMessageProcessorInterface.hpp
#ifndef _STRUS_PEER_MESSAGE_PROCESSOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_PEER_MESSAGE_PROCESSOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/peerMessageProcessorInterface.hpp"

namespace strus
{

class PeerMessageProcessor
	:public PeerMessageProcessorInterface
{
public:
	PeerMessageProcessor();
	virtual ~PeerMessageProcessor();

	virtual PeerMessageViewerInterface* createViewer(
			const char* peermsgptr, std::size_t peermsgsize) const;

	virtual PeerMessageBuilderInterface* createBuilder() const;
};

}//namespace
#endif
