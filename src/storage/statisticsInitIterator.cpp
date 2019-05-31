/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the iterator statistics messages of the storage initialization to populate
/// \file statisticsInitIterator.cpp
#include "statisticsInitIterator.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/statisticsBuilderInterface.hpp"
#include "strus/statisticsProcessorInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/reference.hpp"
#include "databaseAdapter.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <map>

using namespace strus;

StatisticsIteratorInterface*
	strus::createStatisticsInitIterator(
		const StorageClientInterface* storage,
		const DatabaseClientInterface* database,
		ErrorBufferInterface* errorhnd)
{
	const StatisticsProcessorInterface* proc = storage->getStatisticsProcessor();
	if (!proc)
	{
		throw strus::runtime_error(_TXT("error no statistics processor defined"));
	}
	strus::Reference<StatisticsBuilderInterface> builder( proc->createBuilder( ""/*path*/));
	if (!builder.get())
	{
		throw strus::runtime_error(_TXT("failed to create statistics builder: %s"), errorhnd->fetchError());
	}
	int nofdocs = storage->nofDocumentsInserted();
	builder->addNofDocumentsInsertedChange( nofdocs);

	std::map<Index,std::size_t> typenomap;
	std::map<Index,std::size_t> termnomap;
	std::string strings;

	// Fill a map with the strings of all types in the collection:
	{
		DatabaseAdapter_TermType::Cursor typecursor( database);
		Index typeno;
		std::string typestr;
		for (bool more=typecursor.loadFirst( typestr, typeno); more; more=typecursor.loadNext( typestr, typeno))
		{
			typenomap[ typeno] = strings.size();
			strings.append( typestr);
			strings.push_back( '\0');
		}
	}
	// Fill a map with the strings of all terms in the collection:
	{
		DatabaseAdapter_TermValue::Cursor termcursor( database);
		Index termno;
		std::string termstr;
		for (bool more=termcursor.loadFirst( termstr, termno); more; more=termcursor.loadNext( termstr, termno))
		{
			termnomap[ termno] = strings.size();
			strings.append( termstr);
			strings.push_back( '\0');
		}
	}
	// Feed all df changes to the statistics message builder:
	{
		DatabaseAdapter_DocFrequency::Cursor dfcursor( database);
		Index typeno;
		Index termno;
		Index df;

		for (bool more=dfcursor.loadFirst( typeno, termno, df); more; more=dfcursor.loadNext( typeno, termno, df))
		{
			std::map<Index,std::size_t>::const_iterator ti;
			ti = typenomap.find( typeno);
			if (ti == typenomap.end()) throw strus::runtime_error( _TXT( "encountered undefined type (no %d) when populating df's"), (int)typeno);
			const char* typenam = strings.c_str() + ti->second;
	
			ti = termnomap.find( termno);
			if (ti == termnomap.end()) throw strus::runtime_error( "%s",  _TXT( "encountered undefined term when populating df's"));
			const char* termnam = strings.c_str() + ti->second;
	
			builder->addDfChange( typenam, termnam, df);
		}
	}
	return builder->createIteratorAndRollback();
}



