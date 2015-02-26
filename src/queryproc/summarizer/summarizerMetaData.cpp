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
#include "summarizerMetaData.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include "strus/arithmeticVariant.hpp"

using namespace strus;

SummarizerClosureMetaData::SummarizerClosureMetaData( 
		MetaDataReaderInterface* metadata_, const std::string& name_)
	:m_metadata(metadata_)
	,m_attrib(metadata_->elementHandle( name_.c_str()))
{}

std::vector<SummarizerClosureInterface::SummaryElement>
	SummarizerClosureMetaData::getSummary( const Index& docno)
{
	std::vector<SummarizerClosureInterface::SummaryElement> rt;
	m_metadata->skipDoc( docno);
	ArithmeticVariant value = m_metadata->getValue( m_attrib);
	if (value.defined()) 
	{
		rt.push_back( SummarizerClosureInterface::SummaryElement( arithmeticVariantToString( value), 1.0));
	}
	return rt;
}


