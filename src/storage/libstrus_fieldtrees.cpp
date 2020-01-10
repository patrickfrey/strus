/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions for building field inclusion dependency trees
/// \file libstrus_fieldtrees.cpp
#include "strus/lib/fieldtrees.hpp"
#include "strus/base/dll_tags.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

/// \brief strus toplevel namespace
using namespace strus;

static bool addFieldTree( FieldTree& tree, const strus::IndexRange& elem)
{
	if (elem.overlap( tree.range))
	{
		if (elem.cover( tree.range))
		{
			return false;
		}
		else if (tree.range.cover( elem))
		{
			std::list<FieldTree>::iterator ci = tree.chld.begin(), ce = tree.chld.end();
			for (; ci != ce; ++ci)
			{
				if (elem.cover( ci->range))
				{
					if (elem == ci->range) return true;

					FieldTree covernode( elem);
					covernode.add( *ci);
					ci = tree.chld.erase( ci);
					ce = tree.chld.end();
					while (ci != ce)
					{
						if (covernode.range.cover( ci->range))
						{
							covernode.add( *ci);
							ci = tree.chld.erase( ci);
							ce = tree.chld.end();
						}
						else
						{
							++ci;
						}
					}
					tree.add( covernode);
					return true;
				}
				else if (addFieldTree( *ci, elem))
				{
					return true;
				}
			}
			tree.add( elem);
			return true;
		}
	}
	return false;
}

static std::vector<FieldTree> buildFieldTrees_( std::vector<strus::IndexRange>& rest, const std::vector<strus::IndexRange>& llist)
{
	std::vector<FieldTree> rt;
	if (llist.empty()) return rt;

	std::vector<strus::IndexRange>::const_iterator
		li = llist.begin(), le = llist.end();
	for (; li != le; ++li)
	{
		bool added = false;
		std::vector<FieldTree>::iterator
			ri = rt.begin(), re = rt.end();
		while (ri != re && !added)
		{
			if (addFieldTree( *ri, *li))
			{
				added = true;
			}
			else if (*li == ri->range)
			{
				added = true;
			}
			else if (li->cover( ri->range))
			{
				FieldTree covernode( *li);
				covernode.add( *ri);
				ri = rt.erase( ri);
				re = rt.end();
				while (ri != re)
				{
					if (covernode.range.cover( ri->range))
					{
						covernode.add( *ri);
						ri = rt.erase( ri);
						re = rt.end();
					}
					else
					{
						++ri;
					}
				}
				rt.push_back( covernode);
				added = true;
			}
			else if (li->overlap( ri->range))
			{
				rest.push_back( *li);
				added = true;
			}
			else
			{
				++ri;
			}
		}
		if (!added)
		{
			rt.push_back( *li);
		}
	}
	return rt;
}

DLL_PUBLIC std::vector<FieldTree> strus::buildFieldTrees( std::vector<strus::IndexRange>& rest, const std::vector<strus::IndexRange>& fieldlist, ErrorBufferInterface* errorhnd)
{
	try
	{
		return buildFieldTrees_( rest, fieldlist);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error building tree of fields: %s"), *errorhnd, std::vector<FieldTree>());
}


