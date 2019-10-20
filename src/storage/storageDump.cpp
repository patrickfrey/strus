/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storageDump.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/databaseOptions.hpp"
#include "strus/errorBufferInterface.hpp"
#include "databaseKey.hpp"
#include "extractKeyValueData.hpp"
#include <iostream>
#include <sstream>

using namespace strus;

StorageDump::StorageDump( const Reference<DatabaseClientInterface>& database_, const std::string& keyprefix, ErrorBufferInterface* errorhnd_)
	:m_database(database_)
	,m_cursor()
	,m_errorhnd(errorhnd_)
{
	if (!m_database.get()) throw std::runtime_error( _TXT("error creating database client interface"));
	m_cursor.reset( m_database->createCursor( DatabaseOptions()));
	if (!m_cursor.get()) throw std::runtime_error( _TXT("error creating database cursor"));
	m_key = m_cursor->seekFirst( keyprefix.c_str(), keyprefix.size());
}

static void dumpKeyValue(
		std::ostream& out,
		const DatabaseClientInterface* database,
		const DatabaseCursorInterface::Slice& key,
		const DatabaseCursorInterface::Slice& value)
{
	try
	{
		switch (key.ptr()[0])
		{
			case DatabaseKey::TermTypePrefix:
			{
				TermTypeData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::TermValuePrefix:
			{
				TermValueData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::DocIdPrefix:
			{
				DocIdData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::VariablePrefix:
			{
				VariableData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::DocAttributePrefix:
			{
				DocAttributeData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::TermTypeInvPrefix:
			{
				TermTypeInvData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::TermValueInvPrefix:
			{
				TermValueInvData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::ForwardIndexPrefix:
			{
				ForwardIndexData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::DocMetaDataPrefix:
			{
				MetaDataDescription metadescr( database);
				DocMetaDataData data( &metadescr, key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::UserNamePrefix:
			{
				UserNameData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::DocFrequencyPrefix:
			{
				DocFrequencyData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::PosinfoBlockPrefix:
			{
				PosinfoBlockData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::UserAclBlockPrefix:
			{
				UserAclBlockData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::AclBlockPrefix:
			{
				AclBlockData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::DocListBlockPrefix:
			{
				DocListBlockData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::InverseTermPrefix:
			{
				InverseTermData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::MetaDataDescrPrefix:
			{
				MetaDataDescrData data( key, value);
				data.print( out);
				break;
			}
			case DatabaseKey::AttributeKeyPrefix:
			{
				AttributeKeyData data( key, value);
				data.print( out);
				break;
			}
			default:
			{
				throw strus::runtime_error( "%s",  _TXT( "illegal data base key prefix for this storage"));
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		std::string ks( extractKeyString( key));
		throw strus::runtime_error( _TXT( "error in dumped dkey '%s': %s"), ks.c_str(), err.what());
	}
}

bool StorageDump::nextChunk( const char*& chunk, std::size_t& chunksize)
{
	try
	{
		unsigned int ii = 0, nn = NofKeyValuePairsPerChunk;
		std::ostringstream output;
		for (; m_key.defined() && ii<nn; m_key = m_cursor->seekNext(),++ii)
		{
			if (m_key.size() == 0)
			{
				throw strus::runtime_error( "%s",  _TXT( "found empty key"));
			}
			dumpKeyValue( output, m_database.get(), m_key, m_cursor->value());
		};
		m_chunk = output.str();
		chunk = m_chunk.c_str();
		chunksize = m_chunk.size();
		return (chunksize != 0);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error fetching next chunk of storage dump: %s"), *m_errorhnd, false);
}



