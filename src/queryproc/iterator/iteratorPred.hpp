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
#ifndef _STRUS_ITERATOR_PREDECCESSOR_HPP_INCLUDED
#define _STRUS_ITERATOR_PREDECCESSOR_HPP_INCLUDED
#include "iteratorJoin.hpp"
#include "iteratorReference.hpp"

namespace strus
{

class IteratorPred
	:public IteratorJoin
{
public:
	IteratorPred( const IteratorPred& o)
		:m_origin( o.m_origin->copy())
		,m_featureid(o.m_featureid){}

	IteratorPred( const IteratorInterface* origin_)
		:m_origin( origin_?origin_->copy():0)
		,m_featureid(origin_->featureid())
	{
		m_featureid.push_back('<');
	}

	virtual const std::string& featureid() const
	{
		return m_featureid;
	}

	virtual ~IteratorPred(){}

	virtual Index skipDoc( const Index& docno_)
	{
		return m_origin->skipDoc( docno_);
	}

	virtual Index skipPos( const Index& pos_)
	{
		Index rt = m_origin->skipPos( pos_+1);
		return rt?(rt - 1):0;
	}

	virtual std::vector<const IteratorInterface*> subExpressions( bool positive)
	{
		return m_origin->subExpressions( positive);
	}

	virtual IteratorInterface* copy() const
	{
		return new IteratorPred( *this);
	}

private:
	IteratorReference m_origin;		///< base feature expression this is the successor of
	std::string m_featureid;		///< unique id of the feature expression
};

}//namespace
#endif
