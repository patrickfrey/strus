/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Interface for an iterator on term occurrencies in documents (intended for feature selection)
/// \file "DocumentTermIteratorInterface.hpp"
#ifndef _STRUS_DOCUMENT_TERM_OCCURRENCE_ITERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_DOCUMENT_TERM_OCCURRENCE_ITERATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <vector>
#include <string>

namespace strus
{

/// \class DocumentTermIteratorInterface
/// \brief Structure that allows to iterate on document term occurrencies.
class DocumentTermIteratorInterface
{
public:
	struct Term
	{
		unsigned int tf;		//< term frequency in the document
		unsigned int firstpos;		//< first occurrence term position
		Index termno;			//< internal term number (only valid in local server context)

		Term( unsigned int tf_, unsigned int firstpos_, Index termno_)
			:tf(tf_),firstpos(firstpos_),termno(termno_){}
		Term( const Term& o)
			:tf(o.tf),firstpos(o.firstpos),termno(o.termno){}
		void init( unsigned int tf_, unsigned int firstpos_, Index termno_)
			{tf=tf_; firstpos=firstpos_; termno=termno_;}
	};

	virtual ~DocumentTermIteratorInterface(){}

	/// \brief Define the document of the items inspected
	/// \param[in] docno document number to seek
	virtual void skipDoc( const Index& docno)=0;

	/// \brief Get the next term
	/// \param[out] value the next term fetched, if exists
	/// \return true, if exists
	virtual bool nextTerm( Term& value)=0;

	/// \brief Get the local document frequency of a term
	/// \param[in] termno internal local term number
	/// \return the local document frequency (aka 'df')
	virtual unsigned int termDocumentFrequency( const Index& termno) const=0;

	/// \brief Get string representation of a term
	/// \param[in] termno internal local term number
	/// \return the term string
	virtual std::string termValue( const Index& termno) const=0;

};

}//namespace
#endif


