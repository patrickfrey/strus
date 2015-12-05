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
#include "iterator_standard.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "postingIteratorContains.hpp"
#include "postingIteratorIntersect.hpp"
#include "postingIteratorStructWithin.hpp"
#include "postingIteratorStructSequence.hpp"
#include "postingIteratorDifference.hpp"
#include "postingIteratorSucc.hpp"
#include "postingIteratorPred.hpp"
#include "postingIteratorUnion.hpp"

using namespace strus;

PostingJoinOperatorInterface* strus::createPostingJoinIntersect( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinIntersect( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinUnion( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinUnion( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingPred( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinPred( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingSucc( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinSucc( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinWithin( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinWithin( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinStructWithin( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinStructWithin( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinInRange( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinInRange( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinStructInRange( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinStructInRange( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinSequence( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinSequence( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinStructSequence( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinStructSequence( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinChain( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinChain( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinStructChain( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinStructChain( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinContains( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinContains( errorhnd);
}

PostingJoinOperatorInterface* strus::createPostingJoinDifference( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinDifference( errorhnd);
}


