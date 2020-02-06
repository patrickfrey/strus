/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structureSearch.hpp"
#include "strus/structIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include <limits>

using namespace strus;

typedef StructIteratorInterface::HeaderField HeaderField;

void strus::collectHeaderFields( std::vector<HeaderField>& res, StructIteratorInterface* structIterator, strus::Index structno, strus::Index docno, const strus::IndexRange& contentField)
{
	structIterator->skipDoc( docno);

	int li=0, le=structIterator->levels();
	for (; li != le; ++li)
	{
		strus::IndexRange field = structIterator->skipPos( li, contentField.start());
		if (field.cover( contentField))
		{
			bool mapsToItself = false;
			bool isHeader = false;
			int nofContentLinks = 0;
			StructureLinkArray lar = structIterator->links( li);
			int ki=0, ke=lar.nofLinks();
			for (; ki!=ke;++ki)
			{
				const StructureLink& lnk = lar[ ki];
				if (!structno || structno == lnk.structno())
				{
					if (!lnk.header())
					{
						HeaderField hh = structIterator->headerField( lnk.index());
						if (hh.defined())
						{
							mapsToItself |= (hh.field() == field);
							int hlevel = field.cover( hh.field()) ? li+1 : li;
							res.push_back( HeaderField( hh.field(), hlevel));
							++nofContentLinks;
						}
					}
					else
					{
						isHeader = true;
					}
				}
			}
			if (mapsToItself)
			{
				if (nofContentLinks == 1)
				{
					res.back() = HeaderField( res.back().field(), res.back().level()-1);
				}
			}
			else
			{
				if (isHeader)
				{
					res.push_back( HeaderField( field, li));
				}
			}
		}
	}
}

