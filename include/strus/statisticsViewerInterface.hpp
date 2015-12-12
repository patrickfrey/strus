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
/// \brief Interface for a viewer of a message with statistics (distributed index)
/// \file statisticsViewerInterface.hpp
#ifndef _STRUS_STATISTICS_VIEWER_INTERFACE_HPP_INCLUDED
#define _STRUS_STATISTICS_VIEWER_INTERFACE_HPP_INCLUDED
#include <cstdlib>

namespace strus
{

/// \brief Interface for a viewer of a statistics message (distributed index)
class StatisticsViewerInterface
{
public:
	/// \brief Destructor
	virtual ~StatisticsViewerInterface(){}

	/// \brief Fetch the change of the number of document inserted
	/// \return the increment positive or negative (decrement) value of the local change of the collection size
	virtual int nofDocumentsInsertedChange()=0;

	/// \brief Structure describing the document frequency change of one term in the collection
	struct DocumentFrequencyChange
	{
		DocumentFrequencyChange()
			:type(0),value(0),increment(0){}
		DocumentFrequencyChange( const DocumentFrequencyChange& o)
			:type(o.type),value(o.value),increment(o.increment){}

		const char* type;	///< type of the term
		const char* value;	///< value of the term
		int increment;		///< document frequency increment/decrement
	};

	/// \brief Fetch the next message propagating a change in the df (document frequency)
	/// \param[out] the record describing the document frequency change
	/// \return false, if there is no record left and wqe are at the end of the message
	virtual bool nextDfChange( DocumentFrequencyChange& rec)=0;
};
}//namespace
#endif

