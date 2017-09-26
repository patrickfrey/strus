/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "iterator_standard.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "postingIteratorContains.hpp"
#include "postingIteratorIntersect.hpp"
#include "postingIteratorStructWithin.hpp"
#include "postingIteratorStructSequence.hpp"
#include "postingIteratorSequenceImm.hpp"
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

PostingJoinOperatorInterface* strus::createPostingJoinSequenceImm( ErrorBufferInterface* errorhnd)
{
	return new PostingJoinSequenceImm( errorhnd);
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


