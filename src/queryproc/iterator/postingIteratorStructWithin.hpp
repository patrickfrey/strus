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
#ifndef _STRUS_ITERATOR_STRUCT_WITHIN_HPP_INCLUDED
#define _STRUS_ITERATOR_STRUCT_WITHIN_HPP_INCLUDED
#include "iterator/postingIteratorJoin.hpp"
#include "postingIteratorReference.hpp"
#include <vector>

namespace strus
{

/// \class IteratorStructWithin
/// \brief Selects all elements that are appearing inside a defined range without overlapping with a structure delimiter element.
class IteratorStructWithin
	:public IteratorJoin
{
public:
	/// \param[in] range_ the maximum position difference between the start element and the end element of the group
	/// \param[in] nofargs number of elements in args
	/// \param[in] args the positive elements of this join
	/// \param[in] cut (optional) the cut element
	IteratorStructWithin( int range_, std::size_t nofargs, const PostingIteratorInterface** args, const PostingIteratorInterface* cut=0);

	IteratorStructWithin( const IteratorStructWithin& o);

	virtual const std::string& featureid() const
	{
		return m_featureid;
	}
	virtual ~IteratorStructWithin(){}

	virtual std::vector<PostingIteratorInterface*>
			subExpressions( bool positive);

	virtual Index skipDoc( const Index& docno);
	virtual Index skipPos( const Index& pos);

	virtual Index documentFrequency();

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

	virtual PostingIteratorInterface* copy() const
	{
		return new IteratorStructWithin( *this);
	}

private:
	Index m_docno;					///< current document number
	Index m_docno_cut;				///< next document number after m_docno that contains a cut element
	Index m_posno;					///< current position
	PostingIteratorReferenceArray m_group;		///< the elements of the group
	PostingIteratorReference m_cut;			///< the set of elements then must not appear inside the group
	int m_range;					///< the maximum position difference between the start element and the end element of the group
	std::string m_featureid;			///< unique id of the feature expression
	Index m_documentFrequency;			///< document frequency (of the rarest subexpression)
};

}//namespace
#endif


