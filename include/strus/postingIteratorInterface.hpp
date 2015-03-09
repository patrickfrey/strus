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
#ifndef _STRUS_POSTING_ITERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_POSTING_ITERATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <vector>

namespace strus
{

class PostingIteratorInterface
{
public:
	virtual ~PostingIteratorInterface(){}

	/// \brief Return the next match candidate with a document number higher than or equal to docno
	/// \note Candidate means that the document may not match in case of an expression. You have to call ff() or skipPos( const Index&) to decide if the candidate document contains a match. But if 0 is returned you can be sure that there is no match.
	virtual Index skipDoc( const Index& docno)=0;

	/// \brief Return the next matching position higher than or equal to firstpos in the current document. The current document is the one returned with the last 'skipDoc( const Index&)' call.
	virtual Index skipPos( const Index& firstpos)=0;

	/// \brief Unique id in the system for a feature expression
	virtual const char* featureid() const=0;

	/// \brief Return the list of positive or negative subexpressions of the iterator
	/// \param[in] positive true, if the subexpressions should belong to the ones that specify the elements of the set / false, if the subexpressions should belong to the ones that exclude elements from the set.
	/// \return a list of const references to subexpression posting iterators without ownership
	virtual std::vector<const PostingIteratorInterface*> subExpressions( bool positive) const=0;

	/// \brief Get the number of documents where the feature occurrs
	/// \remark May not be defined exactly for composed features. In this case a substitute value should be returned, estimated from the df's of the sub expressions
	/// \return the document frequency (aka 'df')
	virtual GlobalCounter documentFrequency() const=0;

	/// \brief Get the frequency of the feature in the current document
	/// \return the feature frequency (aka 'ff' of 'tf')
	virtual unsigned int frequency()=0;

	/// \brief Get the current document number
	/// \return the document number
	virtual Index docno() const=0;

	/// \brief Get the current position number
	/// \return the position number
	virtual Index posno() const=0;
};

}//namespace
#endif


