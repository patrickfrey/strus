/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_TERM_STATISTICS_HPP_INCLUDED
#define _STRUS_TERM_STATISTICS_HPP_INCLUDED
#include "strus/storage/index.hpp"

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


