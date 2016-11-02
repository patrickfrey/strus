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

StorageDump::StorageDump( const DatabaseInterface* database_, const std::string& configsrc, const std::string& keyprefix, ErrorBufferInterface* errorhnd_)
	:m_database(database_->createClient( configsrc))
	,m_cursor()
	,m_errorhnd(errorhnd_)
{
	m_cursor.reset( m_database->createCursor( DatabaseOptions()));
	if (!m_cursor.get()) throw strus::runtime_error(_TXT("error creating database cursor"));
	m_key = m_cursor->seekFirst( keyprefix.c_str(), keyprefix.size());
}

static void dumpKeyValue(
		std::ostream& out,
		const strus::DatabaseClientInterface* database,
		const strus::DatabaseCursorInterface::Slice& key,
		const strus::DatabaseCursorInterface::Slice& value)
{
	try
	{
		switch (key.ptr()[0])
		{
			case strus::DatabaseKey::TermTypePrefix:
			{
				strus::TermTypeData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::TermValuePrefix:
			{
				strus::TermValueData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocIdPrefix:
			{
				strus::DocIdData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::VariablePrefix:
			{
				strus::VariableData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocAttributePrefix:
			{
				strus::DocAttributeData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::TermTypeInvPrefix:
			{
				strus::TermTypeInvData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::TermValueInvPrefix:
			{
				strus::TermValueInvData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::ForwardIndexPrefix:
			{
				strus::ForwardIndexData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocMetaDataPrefix:
			{
				strus::MetaDataDescription metadescr( database);
				strus::DocMetaDataData data( &metadescr, key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::UserNamePrefix:
			{
				strus::UserNameData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocFrequencyPrefix:
			{
				strus::DocFrequencyData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::PosinfoBlockPrefix:
			{
				strus::PosinfoBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::UserAclBlockPrefix:
			{
				strus::UserAclBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::AclBlockPrefix:
			{
				strus::AclBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocListBlockPrefix:
			{
				strus::DocListBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::InverseTermPrefix:
			{
				strus::InverseTermData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::MetaDataDescrPrefix:
			{
				strus::MetaDataDescrData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::AttributeKeyPrefix:
			{
				strus::AttributeKeyData data( key, value);
				data.print( out);
				break;
			}
			default:
			{
				throw strus::runtime_error( _TXT( "illegal data base prefix"));
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
				throw strus::runtime_error( _TXT( "found empty key in storage"));
			}
			dumpKeyValue( output, m_database, m_key, m_cursor->value());
		};
		m_chunk = output.str();
		chunk = m_chunk.c_str();
		chunksize = m_chunk.size();
		return (chunksize != 0);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error fetching next chunk of storage dump: %s"), *m_errorhnd, false);
}



