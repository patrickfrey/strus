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
	StatisticsStorageClient(
			const DatabaseInterface* database_,
			const StatisticsProcessorInterface* statisticsProc_,
			const std::string& databaseConfig,
			ErrorBufferInterface* errorhnd_);
	virtual ~StatisticsStorageClient();

	virtual bool reload( const std::string& config);

	virtual long diskUsage() const;

	virtual std::string config() const;

	virtual GlobalCounter nofDocuments() const;

	virtual GlobalCounter documentFrequency(
			const std::string& type,
			const std::string& term) const;

	virtual TimeStamp storageTimeStamp( const std::string& storagename) const;

	virtual bool putStatisticsMessage( const StatisticsMessage& msg, const std::string& storagename, const TimeStamp timestamp) const;

	virtual const StatisticsProcessorInterface* getStatisticsProcessor() const;

	virtual void close();

	virtual void compaction();

private:
	void getVariablesWriteBatch(
			DatabaseTransactionInterface* transaction,
			int nof_documents_incr);

	void init( const std::string& databaseConfig);
	void loadVariables( DatabaseClientInterface* database_);
	void storeVariables();
	Index getTermType( const std::string& name) const;

private:
	const DatabaseInterface* m_dbtype;			///< type of key value store database interface
	Reference<DatabaseClientInterface> m_database;		///< reference to key value store database
	char const** m_cfgparam;				///< list of configuration parameters
	strus::AtomicCounter<Index> m_next_typeno;		///< next index to assign to a new term type
	strus::AtomicCounter<GlobalCounter> m_nof_documents;	///< number of documents in the collection

	strus::mutex m_transaction_mutex;			///< mutual exclusion in the critical part of a transaction
	strus::mutex m_immalloc_typeno_mutex;			///< mutual exclusion in the critical part of immediate allocation of typeno

	bool m_close_called;					///< true if close was already called
	const StatisticsProcessorInterface* m_statisticsProc;	///< statistics message processor
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}//namespace
#endif

