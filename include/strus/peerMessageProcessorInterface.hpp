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
#ifndef _STRUS_PEER_MESSAGE_PROCESSOR_INTERFACE_HPP_INCLUDED
#define _STRUS_PEER_MESSAGE_PROCESSOR_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Forward declaration
class PeerMessageViewerInterface;
/// \brief Forward declaration
class PeerMessageBuilderInterface;


/// \brief Interface for packing/unpacking messages with statistics used for query evaluation to other peer storages.
/// \note this interface is used for distributing a search index
class PeerMessageProcessorInterface
{
public:
	/// \brief Destructor
	virtual ~PeerMessageProcessorInterface(){}

	/// \brief Creates a viewer for the contents of a peer message
	/// \param[in] peermsgptr pointer to the packed peer message blob (not necessarily copied by the viewer, lifetime assumed longer than that of viewer)
	/// \param[in] peermsgsize size of the packed peer message blob in bytes
	/// \return the viewer object (with ownership returned)
	virtual PeerMessageViewerInterface* createViewer(
			const char* peermsgptr, std::size_t peermsgsize) const=0;

	struct BuilderFlags
	{
		enum Set {
			None=0x0,			///< No flags
			InsertInLexicalOrder=0x1	///< insertion happens in lexial order
		};
		Set set;

		BuilderFlags( const BuilderFlags& o)
			:set(o.set){}
		BuilderFlags( const Set& set_)
			:set(set_){}
		BuilderFlags()
			:set(None){}
	};

	/// \brief Creates a builder for a peer message
	/// \return the builder object (with ownership returned)
	virtual PeerMessageBuilderInterface* createBuilder( const BuilderFlags& flags_) const=0;
};

}//namespace
#endif

