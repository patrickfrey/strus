/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "postingIteratorSucc.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <cstdlib>

using namespace strus;

PostingIteratorInterface* PostingJoinSucc::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "succ");
		return 0;
	}
	if (range_ != 0)
	{
		m_errorhnd->report( _TXT( "no range argument expected for '%s'"), "succ");
		return 0;
	}
	if (argitr.size() < 1)
	{
		m_errorhnd->report( _TXT( "too few arguments for '%s'"), "succ");
		return 0;
	}
	if (argitr.size() > 1)
	{
		m_errorhnd->report( _TXT( "too many arguments for '%s'"), "succ");
		return 0;
	}
	try
	{
		return new IteratorSucc( argitr[0], m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "succ", *m_errorhnd, 0);
}


PostingJoinOperatorInterface::Description PostingJoinSucc::getDescription() const
{
	try
	{
		return Description( _TXT("Get the set of postings (d,p+1) for all (d,p) in the argument set"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "succ", *m_errorhnd, Description());
}



