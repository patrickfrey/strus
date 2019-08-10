/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Standard implementation of the map of global statistics (in case of a distributed index)
/// \file statisticsMap.cpp
#include "statisticsMap.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/termStatisticsChange.hpp"
#include "strus/statisticsProcessorInterface.hpp"
#include "strus/statisticsViewerInterface.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/string_format.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

StatisticsMap::StatisticsMap( int nofBlocks_, const StatisticsProcessorInterface* proc_, ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_),m_proc(proc_),m_map(nofBlocks_),m_nofDocuments(0){}

StatisticsMap::~StatisticsMap(){}

void StatisticsMap::addNofDocumentsInsertedChange( int increment)
{
	m_nofDocuments.increment( increment);
}

class MapIncrement
{
public:
	void operator()( GlobalCounter& dest, GlobalCounter source) const
	{
		dest += source;
	}
};

static std::string termKey( const char* termtype, const char* termvalue)
{
	return strus::string_format( "%s\1%s", termtype, termvalue);
}

void StatisticsMap::addDfChange( const char* termtype, const char* termvalue, int increment)
{
	try
	{
		std::string key( termKey( termtype, termvalue));
		m_map.set( key.c_str(), increment, MapIncrement());
		addType( termtype);
	}
	CATCH_ERROR_MAP( _TXT("error updating df in statistics map: %s"), *m_errorhnd);
}

bool StatisticsMap::processStatisticsMessage( const void* msgptr, std::size_t msgsize)
{
	try
	{
		typedef std::pair<std::string,GlobalCounter> KeyValuePair;
		strus::local_ptr<StatisticsViewerInterface> viewer( m_proc->createViewer( msgptr, msgsize));
		if (!viewer.get()) throw std::runtime_error( m_errorhnd->fetchError());
		std::set<std::string> typelist;

		m_nofDocuments.increment( viewer->nofDocumentsInsertedChange());

		std::vector<KeyValuePair> elements;
		TermStatisticsChange rec;
		while (viewer->nextDfChange( rec))
		{
			std::string key( termKey( rec.type(), rec.value()));
			elements.push_back( KeyValuePair( key, rec.increment()));
			typelist.insert( rec.type());
		}
		m_map.set( elements, MapIncrement());
		mergeTypes( typelist);
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error processing a statistics message: %s"), *m_errorhnd, false);
}

GlobalCounter StatisticsMap::nofDocuments() const
{
	try
	{
		return m_nofDocuments.value();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error getting total number of documents in statistics map: %s"), *m_errorhnd, 0);
}

GlobalCounter StatisticsMap::df( const std::string& termtype, const std::string& termvalue) const
{
	try
	{
		GlobalCounter rt = 0;
		std::string key( termtype);
		key.push_back( '\1');
		key.append( termvalue);
		if (!m_map.get( key.c_str(), rt)) return 0;
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error getting df in statistics map: %s"), *m_errorhnd, 0);
}

std::vector<std::string> StatisticsMap::types() const
{
	strus::shared_ptr<std::set<std::string> > types_ownership( m_types);
	if (!types_ownership.get()) return std::vector<std::string>();
	return std::vector<std::string>( types_ownership->begin(), types_ownership->end());
}

void StatisticsMap::mergeTypes( const std::set<std::string>& types_)
{
	strus::unique_lock lock( m_mutex_types);
	strus::shared_ptr<std::set<std::string> > types_ownership( m_types);
	if (!types_ownership.get())
	{
		m_types.reset( new std::set<std::string>( types_));
	}
	else
	{
		strus::shared_ptr<std::set<std::string> > types_new( new std::set<std::string>( *types_ownership));
		types_new->insert( types_.begin(), types_.end());
		m_types = types_new;
	}
}

void StatisticsMap::addType( const std::string& type)
{
	strus::unique_lock lock( m_mutex_types);
	strus::shared_ptr<std::set<std::string> > types_ownership( m_types);
	if (!types_ownership.get())
	{
		strus::shared_ptr<std::set<std::string> > types_new( new std::set<std::string>());
		types_new->insert( type);
		m_types = types_new;
	}
	else
	{
		strus::shared_ptr<std::set<std::string> > types_new( new std::set<std::string>( *types_ownership));
		types_new->insert( type);
		m_types = types_new;
	}
}


