/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_METADATA_TABLE_IMPL_HPP_INCLUDED
#define _STRUS_STORAGE_METADATA_TABLE_IMPL_HPP_INCLUDED
#include "strus/storageMetaDataTableUpdateInterface.hpp"
#include "storageMetaDataTransaction.hpp"
#include <vector>
#include <string>

namespace strus {
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class StorageMetaDataTableUpdate
/// \brief Implementation of the interface to alter the metadata table structure within a storage transaction
class StorageMetaDataTableUpdate
	:public StorageMetaDataTableUpdateInterface
{
private:
#if __cplusplus >= 201103L
	StorageMetaDataTableUpdate( StorageMetaDataTableUpdate&) = delete;	//... non copyable
	void operator=( StorageMetaDataTableUpdate&) = delete;			//... non copyable
#endif
private:
	struct Operation
	{
		enum Id {Rename,Add,Delete,Clear,Alter};
		Id id;
		std::string name;
		std::string type;
		std::string oldname;

		Operation( Id id_, const std::string& name_, const std::string& type_, const std::string& oldname_)
			:id(id_),name(name_),type(type_),oldname(oldname_){}
		Operation( const Operation& o)
			:id(o.id),name(o.name),type(o.type),oldname(o.oldname){}
	};

public:
	StorageMetaDataTableUpdate(
			StorageMetaDataTransaction* transaction_,
			ErrorBufferInterface* errorhnd_)
		:m_transaction(transaction_),m_errorhnd(errorhnd_),m_oplist(){}
	~StorageMetaDataTableUpdate(){}

	void alterElement(
			const std::string& oldname,
			const std::string& name,
			const std::string& datatype);

	void addElement(
			const std::string& name,
			const std::string& datatype);

	void renameElement(
			const std::string& oldname,
			const std::string& name);

	void deleteElement(
			const std::string& name);

	void deleteElements();

	void clearElement(
			const std::string& name);

	bool done();

private:
	StorageMetaDataTransaction* m_transaction;		///< storage to call refresh after commit or rollback
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
	std::vector<Operation> m_oplist;
};

}//namespace
#endif

