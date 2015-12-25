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


