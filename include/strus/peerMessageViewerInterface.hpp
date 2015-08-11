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
/// \brief Interface for a viewer of a message received from a peer with some statistics (distributed index)
/// \file peerMessageViewerInterface.hpp
#ifndef _STRUS_PEER_MESSAGE_VIEWER_INTERFACE_HPP_INCLUDED
#define _STRUS_PEER_MESSAGE_VIEWER_INTERFACE_HPP_INCLUDED
#include <cstdlib>

namespace strus
{

/// \brief Interface for a viewer of a message received from a peer with some statistics (distributed index)
class PeerMessageViewerInterface
{
public:
	/// \brief Destructor
	virtual ~PeerMessageViewerInterface(){}

	/// \brief Define the change of the number of document inserted
	/// \param[in] increment positive or negative (decrement) value of the local change of the collection size
	virtual int nofDocumentsInsertedChange()=0;

	struct DocumentFrequencyChange
	{
		DocumentFrequencyChange()
			:type(0),typesize(0),value(0),valuesize(0),increment(0),isnew(false){}
		DocumentFrequencyChange( const DocumentFrequencyChange& o)
			:type(o.type),typesize(o.typesize),value(o.value),valuesize(o.valuesize),increment(o.increment),isnew(o.isnew){}

		const char* type;
		std::size_t typesize;
		const char* value;
		std::size_t valuesize;
		int increment;
		bool isnew;
	};

	/// \brief Fetch the next message propagating a change in the df (document frequency)
	/// \param[out] the record describing the document frequency change
	/// \return false, if there is no record left and wqe are at the end of the message
	virtual bool fetchDfChange( DocumentFrequencyChange& rec)=0;
};
}//namespace
#endif

