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
#include "summarizerAttribute.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/storageInterface.hpp"

using namespace strus;

SummarizerClosureAttribute::SummarizerClosureAttribute( AttributeReaderInterface* attribreader_, const char* name_)
	:m_attribreader(attribreader_)
	,m_attrib(attribreader_->elementHandle( name_))
{}

SummarizerClosureAttribute::~SummarizerClosureAttribute()
{
	delete m_attribreader;
}


std::vector<std::string>
	SummarizerClosureAttribute::getSummary( const Index& docno)
{
	std::vector<std::string> rt;
	m_attribreader->skipDoc( docno);
	std::string attr = m_attribreader->getValue( m_attrib);
	if (!attr.empty()) 
	{
		rt.push_back( attr);
	}
	return rt;
}


SummarizerClosureInterface* SummarizerFunctionAttribute::createClosure(
		const StorageInterface* storage_,
		const char* elementname_,
		PostingIteratorInterface* structitr_,
		std::size_t nofitrs_,
		PostingIteratorInterface**,
		MetaDataReaderInterface*,
		const std::vector<ArithmeticVariant>&) const
{
	if (nofitrs_ || structitr_) throw std::runtime_error( "no feature sets as arguments expected for summarizer 'attribute'");
	return new SummarizerClosureAttribute( storage_->createAttributeReader(), elementname_);
}


