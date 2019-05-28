/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Datatype used for communicating statistics as a blob, interpretation dependent on statistics processor implementation
#ifndef _STRUS_STORAGE_STATISTICS_MESSAGE_BLOB_HPP_INCLUDED
#define _STRUS_STORAGE_STATISTICS_MESSAGE_BLOB_HPP_INCLUDED
#include "strus/timeStamp.hpp"
#include <string>
#include <utility>

namespace strus {

/// \brief Datatype used for communicating statistics as a blob, interpretation dependent on statistics processor implementation
class StatisticsMessage
{
public:
	/// \brief Timestamp used to order messages
	TimeStamp timestamp() const	{return m_timestamp;}
	/// \brief Get a pointer to content blob
	/// \return the pointer to the start of the message blob
	const void* ptr() const	{return m_blob.c_str();}
	/// \brief Get the size of the content blob in bytes
	/// \return the size of the message blob in bytes
	std::size_t size() const	{return m_blob.size();}
	/// \brief Find out if a message is valid or invalid (resp. empty meaning no message left or an error occurred)
	/// \return true if the message is empty
	bool empty() const		{return 0==m_blob.size();}
	/// \brief Get the content blob as std string
	/// \return the blob
	const std::string& blob() const	{return m_blob;}

	/// \brief Constructor
	StatisticsMessage()
		:m_timestamp(),m_blob(){}
	StatisticsMessage( const void* blob_, std::size_t blobsize_, const TimeStamp& timestamp_)
		:m_timestamp(timestamp_),m_blob( (const char*)blob_,blobsize_){}
	/// \brief Copy constructor
	StatisticsMessage( const StatisticsMessage& o)
		:m_timestamp(o.m_timestamp),m_blob(o.m_blob){}
	/// \brief Assignment
	StatisticsMessage& operator=( const StatisticsMessage& o)
		{m_timestamp=o.m_timestamp; m_blob=o.m_blob; return *this;}
#if __cplusplus >= 201103L
	StatisticsMessage( StatisticsMessage&& o)
		:m_timestamp(std::move(o.m_timestamp)),m_blob(std::move(o.m_blob)){}
	StatisticsMessage& operator=( StatisticsMessage&& o)
		{m_timestamp=std::move(o.m_timestamp); m_blob=std::move(o.m_blob); return *this;}
#endif
private:
	TimeStamp m_timestamp;
	std::string m_blob;
};

}//namespace
#endif


