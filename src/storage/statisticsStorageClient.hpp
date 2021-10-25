/*
 * Copyright (c) 2021 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STATISTICS_STORAGE_HPP_INCLUDED
#define _STRUS_STATISTICS_STORAGE_HPP_INCLUDED
#include "strus/statisticsStorageClientInterface.hpp"
#include "strus/storage/index.hpp"
#include "strus/numericVariant.hpp"
#include "strus/reference.hpp"
#include "strus/base/thread.hpp"
#include "strus/base/shared_ptr.hpp"
#include "strus/base/atomic.hpp"
#include "strus/storage/termStatistics.hpp"
#include "strus/statisticsProcessorInterface.hpp"

namespace strus {

/// \brief Forward declaration
class StatisticsStorageTransactionInterface;
/// \brief Forward declaration
class KeyAllocatorInterface;
/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Implementation of the StatisticsStorageClientInterface
class StatisticsStorageClient
	:public StatisticsStorageClientInterface
{
public:
	/// \param[in] database key value store database type used by this storage
	/// \param[in] statisticsProc_ statistics message processor interface
	/// \param[in] databaseConfig configuration string (not a filename!) of the database interface to create for this storage
	/// \param[in] errorhnd_ error buffering interface for error handling
	StatisticsStorageClient(
			const DatabaseInterface* database_,
			const StatisticsProcessorInterface* statisticsProc_,
			const std::string& databaseConfig,
			ErrorBufferInterface* errorhnd_);
	virtual ~StatisticsStorageClient();

	/// \brief Reload storage client with altered configuration
	/// \param[in] config configuration string
	virtual bool reload( const std::string& config);

	/// \brief Get the disk usage in kilo byte units (approximately) of the storage
	virtual long diskUsage() const;

	/// \brief Get the interpreted configuration this storage client was created with
	/// \return the configuration as string
	virtual std::string config() const;

	/// \brief Get the global number of documents
	/// \return the number of documents
	virtual GlobalCounter nofDocuments() const;

	/// \brief Get the global document frequency of a feature
	/// \param[in] type the term type addressed
	/// \param[in] term the term value addressed
	/// \return the global document frequency
	virtual GlobalCounter documentFrequency(
			const std::string& type,
			const std::string& term) const;

	/// \brief Create an insert/update transaction object
	/// \return the created transaction interface (with ownership)
	/// \note this function is thread safe, multiple concurrent transactions are allowed
	virtual StatisticsStorageTransactionInterface* createTransaction();

	/// \brief Get the list of all timestamps identifying when a change of statistics occurred
	/// \note Returns an empty list with error if no statistics processor instance defined for this storage client
	/// \note Depending on the higher level configuration of the system, older files with statistic changes are deleted and not accessible anymore
	/// \return the list of timestamps
	virtual std::vector<TimeStamp> getChangeStatisticTimeStamps() const;

	/// \brief Load the one incremental statistics change message associated with a timestamp
	/// \note Returns an empty message with error if no statistics processor instance defined for this storage client
	/// \param[in] timestamp timestamp associated with the statistics change message
	/// \return the statistics change message structure
	virtual StatisticsMessage loadChangeStatisticsMessage( const TimeStamp& timestamp) const;

	/// \brief Get the processing message interface for introspecting and packing messages outside the queue context
	/// \return the message processor interface
	virtual const StatisticsProcessorInterface* getStatisticsProcessor() const;

	/// \brief Close client connection and eventually do some cleanup.
	/// \remark This method is not implicitely called with the destructor because it might be a complicated operation that cannot be afforded in panic shutdown.
	/// \note the method does not have to be called necessarily
	/// \note it calls compactDatabase of the underlying database and can therefore last some time (some minutes in case of leveldb after large inserts).
	virtual void close();

	/// \brief Do compaction of data.
	/// \remark This method is also called as side effect close
	/// \note the method does not have to be called necessarily
	/// \note it calls compactDatabase of the underlying database and can therefore last some time (some minutes in case of leveldb after large inserts).
	virtual void compaction();
};

}//namespace
#endif

