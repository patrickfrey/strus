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
#include "summarizerMatchPhrase.hpp"
#include "strus/iteratorInterface.hpp"
#include "strus/storageInterface.hpp"

using namespace strus;


std::vector<SummarizerInterface::SummaryElement>
	SummarizerMatchPhrase::getSummary(
		const Index& docno,
		IteratorInterface& itr,
		IteratorInterface& markitr)
{
	Index pos = 0;
	while (0!=(pos=itr.skipPos( pos+1)))
	{
		if (m_maxlen >= 0)
		{
			Index endpos = markitr.skipPos( pos);
			if (endpos - pos > m_maxlen)
			{
				endpos = pos + m_maxlen;
			}
			while (pos <= endpos)
			{
				
			}
			std::string phrase;
	}
	std::vector<SummarizerInterface::SummaryElement> rt;
	std::string attr = m_storage->documentAttributeString( docno, m_name);
	if (!attr.empty())
	{
		rt.push_back( attr);
	}
}


