/*
 * Copyright (c) 2021 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Client interface for accessing a storage of term and document occurrence statistics (read/write)
/// \file "statisticsStorageClientInterface.hpp"
#ifndef _STRUS_STATISTICS_STORAGE_CLIENT_INTERFACE_HPP_INCLUDED
#define _STRUS_STATISTICS_STORAGE_CLIENT_INTERFACE_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "strus/timeStamp.hpp"
#include <string>
#include <vector>

namespace strus
{
/// \brief Forward declaration
class StatisticsStorageTransactionInterface;

/// \brief Interface of a storage of term and document occurrence statistics
class StatisticsStorageClientInterface
{
public:
	/// \brief Destructor
	/// \remark Calls 'close()' but ignore errors returned silently
	virtual ~StatisticsStorageClientInterface(){}

	/// \brief Reload storage client with altered configuration
	/// \param[in] config configuration string
	virtual bool reload( const std::string& config)=0;

	/// \brief Get the disk usage in kilo byte units (approximately) of the storage
	virtual long diskUsage() const=0;

	/// \brief Get the interpreted configuration this storage client was created with
	/// \return the configuration as string
	virtual std::string config() const=0;

	/// \brief Get the global number of documents
	/// \return the number of documents
	virtual GlobalCounter nofDocuments() const=0;

	/// \brief Get the global document frequency of a feature
	/// \param[in] type the term type addressed
	/// \param[in] term the term value addressed
	/// \return the global document frequency
	virtual GlobalCounter documentFrequency(
			const std::string& type,
			const std::string& term) const=0;

	/// \brief Create an insert/update transaction object
	/// \return the created transaction interface (with ownership)
	/// \note this function is thread safe, multiple concurrent transactions are allowed
	virtual StatisticsStorageTransactionInterface* createTransaction()=0;

	/// \brief Get the list of all timestamps identifying when a change of statistics occurred
	/// \note Returns an empty list with error if no statistics processor instance defined for this storage client
	/// \note Depending on the higher level configuration of the system, older files with statistic changes are deleted and not accessible anymore
	/// \return the list of timestamps
	virtual std::vector<TimeStamp> getChangeStatisticTimeStamps() const=0;

	/// \brief Load the one incremental statistics change message associated with a timestamp
	/// \note Returns an empty message with error if no statistics processor instance defined for this storage client
	/// \param[in] timestamp timestamp associated with the statistics change message
	/// \return the statistics change message structure
	virtual StatisticsMessage loadChangeStatisticsMessage( const TimeStamp& timestamp) const=0;

	/// \brief Get the processing message interface for introspecting and packing messages outside the queue context
	/// \return the message processor interface
	virtual const StatisticsProcessorInterface* getStatisticsProcessor() const=0;

	/// \brief Close client connection and eventually do some cleanup.
	/// \remark This method is not implicitely called with the destructor because it might be a complicated operation that cannot be afforded in panic shutdown.
	/// \note the method does not have to be called necessarily
	/// \note it calls compactDatabase of the underlying database and can therefore last some time (some minutes in case of leveldb after large inserts).
	virtual void close()=0;

	/// \brief Do compaction of data.
	/// \remark This method is also called as side effect close
	/// \note the method does not have to be called necessarily
	/// \note it calls compactDatabase of the underlying database and can therefore last some time (some minutes in case of leveldb after large inserts).
	virtual void compaction()=0;
};

}//namespace
#endif


