/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_GLOBAL_STATISTICS_HPP_INCLUDED
#define _STRUS_GLOBAL_STATISTICS_HPP_INCLUDED
#include "strus/storage/index.hpp"

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
	bool defined() const					{return m_nofDocumentsInserted >= 0;}

private:
	GlobalCounter m_nofDocumentsInserted;	///< global number of documents inserted (-1 for undefined, if undefined then the storage value of the global number of documents is used)
};

}//namespace
#endif


