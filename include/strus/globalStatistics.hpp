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
#ifndef _STRUS_GLOBAL_STATISTICS_HPP_INCLUDED
#define _STRUS_GLOBAL_STATISTICS_HPP_INCLUDED
#include "strus/index.hpp"

namespace strus {

/// \brief Global document statistics, if passed down with the query
/// \note If values of this structure is undefined, then the storage values are used
struct GlobalStatistics
{
	/// \brief Default constructor
	GlobalStatistics()
		:m_nofDocumentsInserted(-1){}
	/// \brief Constructor
	explicit GlobalStatistics( const GlobalCounter& nofDocumentsInserted_)
		:m_nofDocumentsInserted(nofDocumentsInserted_){}
	/// \brief Copy constructor
	GlobalStatistics( const GlobalStatistics& o)
		:m_nofDocumentsInserted(o.m_nofDocumentsInserted){}

	GlobalCounter nofDocumentsInserted() const		{return m_nofDocumentsInserted;}
	void setNofDocumentsInserted( const GlobalCounter& n)	{m_nofDocumentsInserted = n;}

private:
	GlobalCounter m_nofDocumentsInserted;	///< global number of documents inserted (-1 for undefined, if undefined then the storage value of the global number of documents is used)
};

}//namespace
#endif


