/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "keyMap.hpp"
#include "keyMapInv.hpp"
#include "databaseAdapter.hpp"
#include "private/internationalization.hpp"

using namespace strus;
#undef STRUS_READ_KEYMAPS_DURING_INSERTION

KeyMap::KeyMap( DatabaseClientInterface* database_,
		DatabaseKey::KeyPrefix prefix_,
		DatabaseKey::KeyPrefix invprefix_,
		KeyAllocatorInterface* allocator_)
	:m_database(database_)
	,m_dbadapter(prefix_,database_)
	,m_dbadapterinv(invprefix_,database_)
	,m_unknownHandleCount(0)
	,m_allocator(allocator_)
	,m_invmap(0)
{}

KeyMap::KeyMap( DatabaseClientInterface* database_,
		DatabaseKey::KeyPrefix prefix_,
		KeyAllocatorInterface* allocator_)
	:m_database(database_)
	,m_dbadapter(prefix_,database_)
	,m_dbadapterinv(0,0)
	,m_unknownHandleCount(0)
	,m_allocator(allocator_)
	,m_invmap(0)
{}

void KeyMap::defineInv( KeyMapInv* invmap_)
{
	m_invmap = invmap_;
}

Index KeyMap::lookUp( const std::string& name)
{
	return m_dbadapter.get( name);
}

Index KeyMap::getOrCreate( const std::string& name, bool& isNew)
{
	Map::const_iterator mi = m_map.find( name);
	if (mi != m_map.end())
	{
		isNew = false;
		return mi->second;
	}
	Index rt;
#ifdef STRUS_READ_KEYMAPS_DURING_INSERTION
	if (m_dbadapter.load( name,rt))
	{
		m_map[ name] = rt;
		if (m_invmap) m_invmap->set( rt, name);
		isNew = false;
	}
	else
#endif
	if (m_allocator->immediate())
	{
		rt = m_allocator->getOrCreate( name, isNew);
		m_map[ name] = rt;
		if (m_invmap) m_invmap->set( rt, name);
		if (m_dbadapterinv.defined())
		{
			m_dbadapterinv.storeImm( rt, name);
		}
		isNew = true;
	}
	else
	{
		rt = ++m_unknownHandleCount;
		if (rt >= UnknownValueHandleStart)
		{
			throw strus::runtime_error( _TXT( "too many elements in keymap"));
		}
		rt += UnknownValueHandleStart;
		m_map[ name] = rt;
		if (m_invmap) m_invmap->set( rt, name);
		isNew = true;
	}
	return rt;
}

void KeyMap::deleteAllFromDeletedList( DatabaseTransactionInterface* transaction)
{
	StringVector::const_iterator di = m_deletedlist.begin(), de = m_deletedlist.end();
	for (; di != de; ++di)
	{
		if (m_dbadapterinv.defined())
		{
			Index value;
			m_dbadapter.load( *di, value);
			m_dbadapterinv.remove( transaction, value);
		}
		m_dbadapter.remove( transaction, *di);
	}
}

void KeyMap::getWriteBatch(
	DatabaseTransactionInterface* transaction)
{
	deleteAllFromDeletedList( transaction);
	// Clear maps:
	clear();
}

void KeyMap::getWriteBatch(
		std::map<Index,Index>& rewriteUnknownMap,
		DatabaseTransactionInterface* transaction,
		int* nofNewItems)
{
	deleteAllFromDeletedList( transaction);

	Map::iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if (mi->second > UnknownValueHandleStart)
		{
			Index idx = lookUp( mi->first);
			if (!idx)
			{
				idx = m_allocator->alloc();
				m_dbadapter.store( transaction, mi->first, idx);
				if (m_dbadapterinv.defined()) m_dbadapterinv.store( transaction, idx, mi->first);
				if (nofNewItems) ++*nofNewItems;
			}
			rewriteUnknownMap[ mi->second] = idx;
			if (m_invmap) m_invmap->set( idx, mi->first);
			mi->second = idx;
		}
	}
	// Clear maps:
	clear();
}

void KeyMap::deleteKey( const std::string& name)
{
	if (m_invmap)
	{
		Map::iterator mi = m_map.find( name);
		if (mi != m_map.end())
		{
			m_invmap->erase( mi->second);
		}
		m_map.erase( mi);
	}
	else
	{
		m_map.erase( name);
	}
	m_deletedlist.push_back( name);
}

void KeyMap::clear()
{
	m_map.clear();
	m_unknownHandleCount = 0;
	if (m_invmap)
	{
		m_invmap->clear();
	}
	m_deletedlist.clear();
}


