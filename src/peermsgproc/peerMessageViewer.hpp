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
/// \brief Implementation of the interface for a viewer of a message received from a peer with some statistics (distributed index)
/// \file peerMessageViewerInterface.hpp
#ifndef _STRUS_PEER_MESSAGE_VIEWER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_PEER_MESSAGE_VIEWER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/peerMessageViewerInterface.hpp"
#include "peerMessageHeader.hpp"
#include <stdint.h>
#include <string>

namespace strus
{
///\brief Forward declaration
class ErrorBufferInterface;

class PeerMessageViewer
	:public PeerMessageViewerInterface
{
public:
	PeerMessageViewer( const char* peermsgptr, std::size_t peermsgsize, ErrorBufferInterface* errorhnd_);
	virtual ~PeerMessageViewer();

	virtual int nofDocumentsInsertedChange();

	virtual bool nextDfChange( DocumentFrequencyChange& rec);

private:
	const PeerMessageHeader* m_hdr;
	const char* m_peermsgptr;
	char const* m_peermsgitr;
	char const* m_peermsgend;
	std::size_t m_peermsgsize;
	std::string m_msg;
	ErrorBufferInterface* m_errorhnd;
};
}//namespace
#endif

