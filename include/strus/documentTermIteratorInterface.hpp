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
		Index tf;		///< term frequency in the document
		Index firstpos;		///< first occurrence term position in the document
		Index termno;		///< internal term number (only valid in local server context, use the method DocumentTermIteratorInterface::termValue(const Index& termno)const to get the term value string)

		/// \brief Default contructor
		Term()
			:tf(0),firstpos(0),termno(0){}
		/// \brief Contructor
		/// \param[in] tf_ term frequency in the document
		/// \param[in] firstpos_ first occurrence term position in the document
		/// \param[in] termno_ local storage internal term number
		Term( Index tf_, Index firstpos_, Index termno_)
			:tf(tf_),firstpos(firstpos_),termno(termno_){}
		/// \brief Copy contructor
		Term( const Term& o)
			:tf(o.tf),firstpos(o.firstpos),termno(o.termno){}
		/// \brief Initialization
		void init( Index tf_, Index firstpos_, Index termno_)
			{tf=tf_; firstpos=firstpos_; termno=termno_;}
	};
	/// \brief Destructor
	virtual ~DocumentTermIteratorInterface(){}

	/// \brief Define the document of the items inspected
	/// \param[in] docno document number to seek
	virtual bool skipDoc( const Index& docno)=0;

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


