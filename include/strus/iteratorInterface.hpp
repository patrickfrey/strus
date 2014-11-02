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
#ifndef _STRUS_ITERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_ITERATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>
#include <vector>

namespace strus
{

class IteratorInterface
{
public:
	virtual ~IteratorInterface(){}

	/// \brief Unique id in the system for a feature expression
	virtual const std::string& featureid() const=0;

	/// \brief Return the next match with a document number higher than or equal to docno
	virtual std::vector<IteratorInterface*> subExpressions( bool positive)=0;

	/// \brief Return the next match with a document number higher than or equal to docno
	virtual Index skipDoc( const Index& docno)=0;

	/// \brief Return the next matching position higher than or equal to firstpos in the current document. The current document is the one returned with the last 'skipDoc( const Index&)' call.
	virtual Index skipPos( const Index& firstpos)=0;

	/// \brief Get the number of documents in the collection where the feature occurrs
	/// \remark May not be defined for composed features
	virtual Index documentFrequency()=0;

	/// \brief Get the frequency of the current document reached with 'skipDoc(const Index&)'
	virtual unsigned int frequency()=0;

	/// \brief Return a copy of this iterator
	virtual IteratorInterface* copy() const=0;
};

}//namespace
#endif


