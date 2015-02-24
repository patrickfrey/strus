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
#include "summarizerMatchVariables.hpp"
#include "postingIteratorLink.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/constants.hpp"
#include <cstdlib>

using namespace strus;

SummarizerClosureMatchVariables::SummarizerClosureMatchVariables(
		const StorageInterface* storage_,
		const QueryProcessorInterface* processor_,
		const std::string& termtype_,
		const std::string& delimiter_,
		const std::string& assign_,
		const std::vector<SummarizerFunctionInterface::FeatureParameter>& features_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( termtype_))
	,m_termtype(termtype_)
	,m_delimiter(delimiter_.empty()?std::string(","):delimiter_)
	,m_assign(assign_.empty()?std::string("="):assign_)
	,m_features()
{
	std::vector<Reference<PostingIteratorInterface> > structelem;
	std::vector<SummarizerFunctionInterface::FeatureParameter>::const_iterator fi = features_.begin(), fe = features_.end();
	for (; fi != fe; ++fi)
	{
		m_features.push_back( fi->feature());
	}
}

SummarizerClosureMatchVariables::~SummarizerClosureMatchVariables()
{}


std::vector<SummarizerClosureInterface::SummaryElement>
	SummarizerClosureMatchVariables::getSummary( const Index& docno)
{
	std::vector<SummarizerClosureInterface::SummaryElement> rt;
	m_forwardindex->skipDoc( docno);
	Index curpos = 0;

	std::vector<SummarizationFeature>::const_iterator
		fi = m_features.begin(), fe = m_features.end();

	for (; fi != fe; ++fi)
	{
		if (docno==fi->postingIterator()->skipDoc( docno))
		{
			for (curpos = 0; curpos; curpos=fi->postingIterator()->skipPos( curpos+1))
			{
				SummarizationFeature::variable_const_iterator
					vi = fi->variables_begin(),
					ve = fi->variables_end();

				std::string line;
				for (int vidx=0; vi != ve; ++vi,++vidx)
				{
					if (vidx) line.append( m_delimiter);
					line.append( vi->name());
					line.append( m_assign);
					m_forwardindex->skipPos( vi->position());
					line.append( m_forwardindex->fetch());
				}
			}
		}
	}
	return rt;
}


