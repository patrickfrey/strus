/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_KEY_ALLOCATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_KEY_ALLOCATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>

namespace strus {

class KeyAllocatorInterface
{
public:
	KeyAllocatorInterface( bool immediate_)
		:m_immediate(immediate_){}

	virtual ~KeyAllocatorInterface(){}
	virtual Index alloc()=0;
	virtual Index getOrCreate( const std::string& name)=0;

	/// \brief Defines what interface is provided true->getOrCreate, false->alloc
	bool immediate() const {return m_immediate;}
private:
	bool m_immediate;
};

}//namespace
#endif

