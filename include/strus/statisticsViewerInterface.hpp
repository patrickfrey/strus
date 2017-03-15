/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
	class DocumentFrequencyChange
	{
	public:
		DocumentFrequencyChange()
			:m_type(0),m_value(0),m_increment(0){}
		DocumentFrequencyChange( const DocumentFrequencyChange& o)
			:m_type(o.m_type),m_value(o.m_value),m_increment(o.m_increment){}
		DocumentFrequencyChange( const char* type_, const char* value_, int increment_)
			:m_type(type_),m_value(value_),m_increment(increment_){}

		const char* type() const	{return m_type;}
		const char* value() const	{return m_value;}
		int increment() const		{return m_increment;}
	private:
		const char* m_type;	///< type of the term
		const char* m_value;	///< value of the term
		int m_increment;	///< document frequency increment/decrement
	};

	/// \brief Fetch the next message propagating a change in the df (document frequency)
	/// \param[out] the record describing the document frequency change
	/// \return false, if there is no record left and wqe are at the end of the message
	virtual bool nextDfChange( DocumentFrequencyChange& rec)=0;
};
}//namespace
#endif

