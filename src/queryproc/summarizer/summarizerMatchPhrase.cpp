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
#include "strus/forwardIndexViewerInterface.hpp"
#include "strus/storageInterface.hpp"

using namespace strus;

SummarizerMatchPhrase::SummarizerMatchPhrase( StorageInterface* storage_, const std::string& termtype_, int maxlen_)
	:m_storage(storage_)
	,m_forwardindex( storage_->createForwardIndexViewer( termtype_))
	,m_termtype(termtype_)
	,m_maxlen(maxlen_)
{}

SummarizerMatchPhrase::~SummarizerMatchPhrase()
{
	if (m_forwardindex) delete m_forwardindex;
}


static void getSummary_(
		std::vector<SummarizerInterface::SummaryElement>& res,
		const Index& docno,
		IteratorInterface& itr,
		IteratorInterface& markitr,
		ForwardIndexViewerInterface& forwardindex,
		int maxlen)
{
	if (itr.skipDoc( docno) == docno)
	{
		forwardindex.initDoc( docno);

		Index pos = 0;
		while (0!=(pos=itr.skipPos( pos+1)))
		{
			if (maxlen >= 0)
			{
				unsigned int length = 0;
				Index endpos = markitr.skipPos( pos);
				const char* trailer = "";
				if (endpos - pos > maxlen)
				{
					trailer = " ...";
					endpos = pos + maxlen;
				}
				std::string phrase;
				for (;pos <= endpos; ++pos)
				{
					pos = forwardindex.skipPos(pos);
					if (pos)
					{
						if (!phrase.empty()) phrase.push_back(' ');
						phrase.append( forwardindex.fetch());
						++length;
					}
					else
					{
						break;
					}
				}
				phrase.append( trailer);
				res.push_back( SummarizerInterface::SummaryElement( phrase, pos, length));
			}
			else
			{
				int absolute_maxlen = -maxlen;
				unsigned int length = 0;
				Index rangepos = (pos > absolute_maxlen) ? (pos-absolute_maxlen):1;
				Index prevpos = markitr.skipPos( rangepos);
				std::string phrase;
				if (!prevpos || prevpos > pos)
				{
					prevpos = rangepos;
					if (rangepos > 1)
					{
						phrase.append( "...");
					}
				}
				else for (;;)
				{
					Index midpos = markitr.skipPos( prevpos+1);
					if (!midpos || midpos > pos) break;
					prevpos = midpos;
				}
				for (;prevpos <= pos; ++prevpos)
				{
					prevpos = forwardindex.skipPos(prevpos);
					if (prevpos)
					{
						if (!phrase.empty()) phrase.push_back(' ');
						phrase.append( forwardindex.fetch());
						++length;
					}
					else
					{
						break;
					}
				}
				res.push_back( SummarizerInterface::SummaryElement( phrase, pos, length));
			}
		}
	}
}


std::vector<SummarizerInterface::SummaryElement>
	SummarizerMatchPhrase::getSummary(
		const Index& docno,
		IteratorInterface& itr,
		IteratorInterface& markitr)
{
	std::vector<SummaryElement> rt;
	if (docno)
	{
		getSummary_( rt, docno, itr, markitr, *m_forwardindex, m_maxlen);
	}
	else
	{
		Index dn = 0;
		while (0!=(dn=itr.skipDoc( dn+1)))
		{
			getSummary_( rt, dn, itr, markitr, *m_forwardindex, m_maxlen);
		}
	}
	return rt;
}


