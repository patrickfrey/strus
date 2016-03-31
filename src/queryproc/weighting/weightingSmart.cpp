/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\file weightingSmart.cpp
///\brief Implementation of a weighting function defined as function on tf,df,N and some metadata references in a string
#include "weightingSmart.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/utils.hpp"
#include "private/internationalization.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace strus;

WeightingFunctionContextSmart::WeightingFunctionContextSmart(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		const std::vector<std::string>& m_metadataelemar,
		const ScalarFunctionInstanceInterface* func_,
		ErrorBufferInterface* errorhnd_)
{
}

void WeightingFunctionContextSmart::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		float weight_,
		const TermStatistics& stats_)
{
}

double WeightingFunctionContextSmart::call( const Index& docno)
{
}


void WeightingFunctionInstanceSmart::addStringParameter( const std::string& name, const std::string& value)
{
}

void WeightingFunctionInstanceSmart::addNumericParameter( const std::string& name, const NumericVariant& value)
{
}

WeightingFunctionContextInterface* WeightingFunctionInstanceSmart::createFunctionContext(
		const StorageClientInterface* storage_,
		MetaDataReaderInterface* metadata,
		const GlobalStatistics& stats) const
{
}

std::string WeightingFunctionInstanceSmart::tostring() const
{
}

WeightingFunctionInstanceInterface* WeightingFunctionSmart::createInstance(
		const QueryProcessorInterface* processor) const
{
}

Description WeightingFunctionSmart::getDescription() const
{
}


