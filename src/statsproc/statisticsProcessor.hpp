/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for packing/unpacking messages with statistics used for query evaluation.
/// \file statisticsProcessorInterface.hpp
#ifndef _STRUS_STATISTICS_PROCESSOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STATISTICS_PROCESSOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/statisticsProcessorInterface.hpp"

namespace strus
{
///\brief Forward declaration
class ErrorBufferInterface;

class StatisticsProcessor
	:public StatisticsProcessorInterface
{
private:
	enum {DefaultMaxBlockSize=32000};

public:
	explicit StatisticsProcessor( ErrorBufferInterface* errorhnd_);
	virtual ~StatisticsProcessor();

	virtual StatisticsViewerInterface* createViewer(
			const void* msgptr, std::size_t msgsize) const;

	virtual StatisticsBuilderInterface* createBuilder() const;

private:
	ErrorBufferInterface* m_errorhnd;
};

}//namespace
#endif

