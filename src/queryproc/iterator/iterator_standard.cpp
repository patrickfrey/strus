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

PostingJoinOperatorInterface* strus::createPostingJoinIntersect()
{
	return new PostingJoinIntersect();
}

PostingJoinOperatorInterface* strus::createPostingJoinUnion()
{
	return new PostingJoinUnion();
}

PostingJoinOperatorInterface* strus::createPostingPred()
{
	return new PostingJoinPred();
}

PostingJoinOperatorInterface* strus::createPostingSucc()
{
	return new PostingJoinSucc();
}

PostingJoinOperatorInterface* strus::createPostingJoinWithin()
{
	return new PostingJoinWithin();
}

PostingJoinOperatorInterface* strus::createPostingJoinStructWithin()
{
	return new PostingJoinStructWithin();
}

PostingJoinOperatorInterface* strus::createPostingJoinSequence()
{
	return new PostingJoinSequence();
}

PostingJoinOperatorInterface* strus::createPostingJoinStructSequence()
{
	return new PostingJoinStructSequence();
}

PostingJoinOperatorInterface* strus::createPostingJoinContains()
{
	return new PostingJoinContains();
}

PostingJoinOperatorInterface* strus::createPostingJoinDifference()
{
	return new PostingJoinDifference();
}


