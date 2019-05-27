/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Datatype used to mark time for ordering events and specifying snapshots
#ifndef _STRUS_STORAGE_TIMESTAMP_HPP_INCLUDED
#define _STRUS_STORAGE_TIMESTAMP_HPP_INCLUDED
#include <ctime>

namespace strus {

/// \brief Datatype used to mark time for ordering events and specifying snapshots
class TimeStamp
{
public:
	/// \brief Seconds since 1.1.1970
	time_t unixtime() const	{return m_unixtime;}
	/// \brief Counter for ordering events appearing in the same second
	int counter() const	{return m_counter;}

	/// \brief Constructor
	explicit TimeStamp( time_t unixtime_, int counter_=0)
		:m_unixtime(unixtime_),m_counter(counter_){}
	/// \brief Copy constructor
	TimeStamp( const TimeStamp& o)
		:m_unixtime(o.m_unixtime),m_counter(o.m_counter){}
	/// \brief Assignment
	TimeStamp& operator=( const TimeStamp& o)
		{m_unixtime=o.m_unixtime; m_counter=o.m_counter; return *this;}

	/// \brief Comparison
	bool operator == (const TimeStamp& o) const
		{return m_unixtime == o.m_unixtime && m_counter == o.m_counter;}
	bool operator != (const TimeStamp& o) const
		{return m_unixtime != o.m_unixtime || m_counter != o.m_counter;}
	bool operator >= (const TimeStamp& o) const
		{return m_unixtime == o.m_unixtime ? m_counter >= o.m_counter : m_unixtime >= o.m_unixtime;}
	bool operator > (const TimeStamp& o) const
		{return m_unixtime == o.m_unixtime ? m_counter > o.m_counter : m_unixtime > o.m_unixtime;}
	bool operator <= (const TimeStamp& o) const
		{return m_unixtime == o.m_unixtime ? m_counter <= o.m_counter : m_unixtime <= o.m_unixtime;}
	bool operator < (const TimeStamp& o) const
		{return m_unixtime == o.m_unixtime ? m_counter < o.m_counter : m_unixtime < o.m_unixtime;}

private:
	time_t m_unixtime;		//< seconds since 1.1.1970
	int m_counter;			//< counter for ordering events appearing in the same second
};

}//namespace
#endif


