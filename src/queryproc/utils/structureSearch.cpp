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

void strus::collectFieldHeaders( std::vector<strus::IndexRange>& res, StructIteratorInterface* structIterator, strus::Index structno, const strus::IndexRange& contentField)
{
	int li=0, le=structIterator->levels();
	for (; li != le; ++li)
	{
		strus::IndexRange field = structIterator->skipPos( li, contentField.start());
		if (field.cover( contentField))
		{
			StructureLinkArray lar = structIterator->links( li);
			int ki=0, ke=lar.nofLinks();
			for (; ki!=ke;++ki)
			{
				const StructureLink& lnk = lar[ ki];
				if (!structno || structno == lnk.structno())
				{
					if (lnk.header())
					{
						res.push_back( field);
					}
					else
					{
						strus::IndexRange headerField = structIterator->headerField( lnk.index());
						if (headerField.defined())
						{
							res.push_back( headerField);
						}
					}
				}
			}
		}
	}
}

