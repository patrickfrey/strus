/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Result element of the query evaluation
/// \file "resultDocument.hpp"
#ifndef _STRUS_RESULT_DOCUMENT_HPP_INCLUDED
#define _STRUS_RESULT_DOCUMENT_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "strus/storage/weightedDocument.hpp"
#include "strus/storage/summaryElement.hpp"
#include <vector>
#include <utility>

namespace strus {

/// \class ResultDocument
/// \brief Structure defining one result element of a strus query
class ResultDocument
	:public WeightedDocument
{
public:
	/// \brief Default constructor
	ResultDocument(){}
	/// \brief Copy constructor
	ResultDocument( const ResultDocument& o)
		:WeightedDocument(o),m_summaryElements(o.m_summaryElements){}
	/// \brief Constructor from a composition of the pure query evaluation result and the summary elements
	ResultDocument( const WeightedDocument& o, const std::vector<SummaryElement>& summaryElements_)
		:WeightedDocument(o),m_summaryElements(summaryElements_){}
	/// \brief Constructor from a composition of the pure query evaluation result without summary elements
	ResultDocument( const WeightedDocument& o)
		:WeightedDocument(o){}
	/// \brief Constructor from a composition its basic parts
	ResultDocument( strus::Index docno_, const strus::IndexRange& field_, double weight_, const std::vector<SummaryElement>& summaryElements_)
		:WeightedDocument(docno_,field_,weight_),m_summaryElements(summaryElements_){}
	ResultDocument& operator=( const ResultDocument& o)
		{WeightedDocument::operator=(o); m_summaryElements=o.m_summaryElements; return *this;}

#if __cplusplus >= 201103L
	ResultDocument( ResultDocument&& o)
		:WeightedDocument(o),m_summaryElements(std::move(o.m_summaryElements)){}
	ResultDocument& operator=( ResultDocument&& o)
		{WeightedDocument::operator=(o); m_summaryElements=std::move(o.m_summaryElements); return *this;}
#endif
	/// \brief Get the list of summary elements of this result
	const std::vector<SummaryElement>& summaryElements() const	{return m_summaryElements;}

private:
	std::vector<SummaryElement> m_summaryElements;	///< summary elements of this result
};

}//namespace
#endif

