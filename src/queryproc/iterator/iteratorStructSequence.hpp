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
#ifndef _STRUS_ITERATOR_STRUCT_SEQUENCE_HPP_INCLUDED
#define _STRUS_ITERATOR_STRUCT_SEQUENCE_HPP_INCLUDED
#include "iteratorJoin.hpp"
#include "iteratorReference.hpp"
#include <vector>

namespace strus
{

/// \class IteratorStructSequence
/// \brief Selects all elements that are appearing in a strict ascending position order inside a defined range without overlapping with a delimiter element.
class IteratorStructSequence
	:public IteratorJoin
{
public:
	/// \param[in] range_ the maximum position difference between the start element and the end element of the group
	/// \param[in] nofargs number of elements in args
	/// \param[in] args the elements of this join 
	/// \param[in] cut (optional) the cut element
	IteratorStructSequence( int range_, std::size_t nofargs, const IteratorInterface** args, const IteratorInterface* cut=0);

	IteratorStructSequence( const IteratorStructSequence& o);
	virtual ~IteratorStructSequence(){}

	virtual const std::string& featureid() const
	{
		return m_featureid;
	}
	virtual Index skipDoc( const Index& docno);
	virtual Index skipPos( const Index& pos);

	virtual std::vector<const IteratorInterface*> subExpressions( bool positive);

	virtual IteratorInterface* copy() const
	{
		return new IteratorStructSequence( *this);
	}

private:
	Index m_docno;				///< current document number
	Index m_docno_cut;			///< next document number after m_docno that contains a cut element
	std::vector<IteratorReference> m_seq;	///< the elements of the sequence
	IteratorReference m_cut;		///< the set of elements then must not appear inside the sequence
	int m_range;				///< the maximum position difference between the start element and the end element of the sequence
	std::string m_featureid;		///< unique id of the feature expression
};

}//namespace
#endif


