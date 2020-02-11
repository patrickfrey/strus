/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structureSearch.hpp"
#include "strus/structureIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include <limits>
#include <algorithm>

using namespace strus;

void strus::collectHeaderFields( std::vector<StructureHeaderField>& res, StructureIteratorInterface* structIterator, strus::Index structno, strus::Index docno, const strus::IndexRange& contentField)
{
	structIterator->skipDoc( docno);
	strus::IndexRange coverField = contentField;

	int li=structIterator->levels()-1;
	for (; li >= 0; --li)
	{
		strus::IndexRange field = structIterator->skipPos( li, contentField.start());
		if (field.cover( coverField))
		{
			bool isHeader = false;
			bool mapsToItself = false;
			StructureLinkArray lar = structIterator->links( li);
			int ki=0, ke=lar.nofLinks();
			for (; ki!=ke;++ki)
			{
				const StructureLink& lnk = lar[ ki];
				if (!structno || structno == lnk.structno())
				{
					if (!lnk.header())
					{
						StructureHeaderField hh = structIterator->headerField( lnk.index());
						if (hh.defined())
						{
							mapsToItself |= (hh.field() == field);
							if (hh.field().start() <= coverField.start())
							{
								coverField.setStart( hh.field().start());
							}
							if (hh.field().end() >= coverField.end())
							{
								coverField.setEnd( hh.field().end());
							}
							res.push_back( StructureHeaderField( hh.field(), li + (mapsToItself?1:0)));
						}
					}
					else
					{
						isHeader = true;
					}
				}
			}
			if (!mapsToItself && isHeader)
			{
				res.push_back( StructureHeaderField( field, li));
			}
		}
	}
	// Ensure top to down order and assign correct hierarchy level:
	std::sort( res.begin(), res.end());
	std::vector<StructureHeaderField>::iterator ri = res.begin(), re = res.end();
	int hcnt = 0;
	while (ri != re)
	{
		int lv = ri->hierarchy();
		for (; ri != re && lv == ri->hierarchy(); ++ri)
		{
			*ri = StructureHeaderField( ri->field(), hcnt);
		}
		++hcnt;
	}
}

