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
#ifndef _STRUS_TERM_STATISTICS_HPP_INCLUDED
#define _STRUS_TERM_STATISTICS_HPP_INCLUDED
#include "strus/index.hpp"

namespace strus {

/// \brief Global term statistics, if passed down with the query
/// \note If values of this structure is undefined, then the value stored is used
class TermStatistics
{
public:
	/// \brief Constructor
	TermStatistics()
		:m_df(-1){}
	/// \brief Constructor
	TermStatistics( const GlobalCounter& df_)
		:m_df(df_){}
	/// \brief Copy constructor
	TermStatistics( const TermStatistics& o)
		:m_df(o.m_df){}

	/// \brief Get the global document frequency
	/// \return the global document frequency or -1 for undefined, if undefined then the value cache for the global dfs in the document frequency or what is stored in the local storage)
	GlobalCounter documentFrequency() const			{return m_df;}

	/// \brief Set the global document frequency to use for the associated term
	/// \param[in] df_ the global document frequency value
	void setDocumentFrequency( const GlobalCounter& df_)	{m_df = df_;}

private:
	GlobalCounter m_df;		///< global document frequency (-1 for undefined, if undefined then the value cache for the global dfs in the document frequency or what is stored in the local storage)
};

}//namespace
#endif


