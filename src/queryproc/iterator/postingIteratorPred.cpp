/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "postingIteratorPred.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <cstdlib>

using namespace strus;

PostingIteratorInterface* PostingJoinPred::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range,
		unsigned int cardinality) const
{
	if (cardinality != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "pred");
		return 0;
	}
	if (range != 0)
	{
		m_errorhnd->report( _TXT( "no range argument expected for '%s'"), "pred");
		return 0;
	}
	if (argitr.size() < 1)
	{
		m_errorhnd->report( _TXT( "too few arguments for '%s'"), "pred");
		return 0;
	}
	if (argitr.size() > 1)
	{
		m_errorhnd->report( _TXT( "too many arguments for '%s'"), "pred");
		return 0;
	}
	try
	{
		return new IteratorPred( argitr[0], m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "pred", *m_errorhnd, 0);
}

PostingJoinOperatorInterface::Description PostingJoinPred::getDescription() const
{
	try
	{
		return Description( _TXT("Get the set of postings (d,p-1) for all (d,p) with p>1 in the argument set"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "pred", *m_errorhnd, Description());
}


