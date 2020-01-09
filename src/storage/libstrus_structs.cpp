/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions for handling structures in the storage
/// \file libstrus_structs.cpp
#include "strus/lib/structs.hpp"
#include "strus/base/dll_tags.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/structIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

/// \brief strus toplevel namespace
using namespace strus;

DLL_PUBLIC StorageStructMap::StorageStructMap( StructIteratorInterface* stitr, strus::Index docno, ErrorBufferInterface* errorhnd)
{
	try
	{
		stitr->skipDoc( docno);
		int li = 0, le = stitr->levels();
		for (; li != le; ++li)
		{
			IndexRange field = stitr->skipPos( li, 0);
			for (; field.defined(); field = stitr->skipPos( li, field.end()))
			{
				StructureLinkArray lnka = stitr->links( li);
				int ai = 0, ae = lnka.nofLinks();
				for (; ai != ae; ++ai)
				{
					const StructureLink& link = lnka[ ai];
					Key key( link.structno(), link.index());
					std::pair<Map::iterator,bool> ins = m_map.insert( Map::value_type( key, FieldRelation()));
					FieldRelation& rel= ins.first->second;
					if (link.header())
					{
						if (rel.first.defined())
						{
							throw strus::runtime_error(_TXT("corrupt index: structure with more than one header"));
						}
						rel.first = field;
					}
					else
					{
						rel.second.push_back( field);
					}
				}
			}
		}
	}
	CATCH_ERROR_MAP( _TXT("error creating map of structures of a document in the storage: %s"), *errorhnd);
}



