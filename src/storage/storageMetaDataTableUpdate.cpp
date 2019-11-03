/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storageMetaDataTableUpdate.hpp"
#include "storageMetaDataTransaction.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

void StorageMetaDataTableUpdate::alterElement(
		const std::string& oldname,
		const std::string& name,
		const std::string& datatype)
{
	try
	{
		m_oplist.push_back( Operation( Operation::Alter, name, datatype, oldname));
	}
	CATCH_ERROR_MAP( _TXT("error altering element: %s"), *m_errorhnd);
}

void StorageMetaDataTableUpdate::addElement(
		const std::string& name,
		const std::string& datatype)
{
	try
	{
		m_oplist.push_back( Operation( Operation::Add, name, datatype, std::string()/*oldname*/));
	}
	CATCH_ERROR_MAP( _TXT("error adding element: %s"), *m_errorhnd);
}

void StorageMetaDataTableUpdate::renameElement(
		const std::string& oldname,
		const std::string& name)
{
	try
	{
		m_oplist.push_back( Operation( Operation::Rename, name, std::string()/*datatype*/, oldname));
	}
	CATCH_ERROR_MAP( _TXT("error renaming element: %s"), *m_errorhnd);
}

void StorageMetaDataTableUpdate::deleteElement(
		const std::string& name)
{
	try
	{
		m_oplist.push_back( Operation( Operation::Delete, name, std::string()/*datatype*/, std::string()/*oldname*/));
	}
	CATCH_ERROR_MAP( _TXT("error deleting element: %s"), *m_errorhnd);
}

void StorageMetaDataTableUpdate::deleteElements()
{
	try
	{
		m_oplist.push_back( Operation( Operation::Delete, std::string()/*name*/, std::string()/*datatype*/, std::string()/*oldname*/));
	}
	CATCH_ERROR_MAP( _TXT("error deleting all elements: %s"), *m_errorhnd);
}

void StorageMetaDataTableUpdate::clearElement(
		const std::string& name)
{
	try
	{
		m_oplist.push_back( Operation( Operation::Clear, name, std::string()/*datatype*/, std::string()/*oldname*/));
	}
	CATCH_ERROR_MAP( _TXT("error clearing element value: %s"), *m_errorhnd);
}

bool StorageMetaDataTableUpdate::done()
{
	try
	{
		if (m_errorhnd->hasError())
		{
			throw std::runtime_error(_TXT("previous, uncaught error before resuming meta data update"));
		}
		std::vector<Operation>::const_iterator oi = m_oplist.begin(), oe = m_oplist.end();
		for (; oi != oe; ++oi)
		{
			switch (oi->id)
			{
				case Operation::Rename:
					m_transaction->renameElement( oi->oldname, oi->name);
					break;
				case Operation::Add:
					m_transaction->addElement( oi->name, oi->type);
					break;
				case Operation::Delete:
					if (!oi->name.empty())
					{
						m_transaction->deleteElement( oi->name);
					}
					else
					{
						m_transaction->deleteElements();
					}
					break;
				case Operation::Clear:
					m_transaction->clearElement( oi->name);
					break;
				case Operation::Alter:
					m_transaction->alterElement( oi->oldname, oi->name, oi->type);
					break;
			}
		}
		m_oplist.clear();
		if (m_errorhnd->hasError())
		{
			throw std::runtime_error( m_errorhnd->fetchError());
		}
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error resuming meta data updates: %s"), *m_errorhnd, false);
}

