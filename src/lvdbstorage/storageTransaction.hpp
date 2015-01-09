/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_LVDB_TRANSACTION_HPP_INCLUDED
#define _STRUS_LVDB_TRANSACTION_HPP_INCLUDED
#include "strus/storageTransactionInterface.hpp"
#include "strus/arithmeticVariant.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "metaDataBlockMap.hpp"
#include "metaDataReader.hpp"
#include "attributeMap.hpp"
#include "docnoBlock.hpp"
#include "docnoBlockMap.hpp"
#include "booleanBlock.hpp"
#include "userAclBlockMap.hpp"
#include "posinfoBlock.hpp"
#include "posinfoBlockMap.hpp"
#include "forwardIndexBlock.hpp"
#include "forwardIndexBlockMap.hpp"
#include "documentFrequencyMap.hpp"
#include "keyMap.hpp"
#include "keyAllocatorInterface.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <boost/thread/mutex.hpp>

namespace strus {
/// \brief Forward declaration
class Storage;

/// \class StorageTransaction
class StorageTransaction
	:public StorageTransactionInterface
{
public:
	StorageTransaction( 
		Storage* storage_,
		leveldb::DB* db_,
		const MetaDataDescription* metadescr_);

	~StorageTransaction();

	virtual void deleteDocument(
			const std::string& docid);

	virtual void deleteUserAccessRights(
			const std::string& username);

	virtual StorageDocumentInterface*
		createDocument(
			const std::string& docid_,
			const Index& docno=0);

	/// \brief Transaction commit
	virtual void commit();
	/// \brief Transaction rollback (automatically called with the destructor)
	virtual void rollback();

public:/*Document*/
	Index getOrCreateTermValue( const std::string& name);
	Index getOrCreateTermType( const std::string& name);
	Index getOrCreateAttributeName( const std::string& name);
	Index getOrCreateDocno( const std::string& name, bool& isNew);
	Index getOrCreateUserno( const std::string& name, bool& isNew);
	Index lookUpTermValue( const std::string& name);

	void defineMetaData( const Index& docno, const std::string& varname, const ArithmeticVariant& value);
	void deleteMetaData( const Index& docno, const std::string& varname);
	void deleteMetaData( const Index& docno);

	void defineAttribute( const Index& docno, const std::string& varname, const std::string& value);
	void deleteAttribute( const Index& docno, const std::string& varname);
	void deleteAttributes( const Index& docno);

	void defineAcl( const Index& userno, const Index& docno);
	void deleteAcl( const Index& userno, const Index& docno);
	void deleteAcl( const Index& docno);

	void deleteIndex( const Index& docno);

	void defineDocnoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno, unsigned int ff, float weight);

	void deleteDocnoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno);

	void definePosinfoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno, const std::vector<Index>& posinfo);

	void defineForwardIndexTerm(
		const Index& typeno, const Index& docno,
		const Index& pos, const std::string& termstring);

	void closeForwardIndexDocument( const Index& docno);

private:
	Storage* m_storage;					///< Storage to call refresh after commit or rollback
	leveldb::DB* m_db;					///< levelDB handle
	leveldb::Options m_dboptions;				///< options for levelDB
	const MetaDataDescription* m_metadescr;			///< description of metadata

	AttributeMap m_attributeMap;				///< map of document attributes for writing
	MetaDataBlockMap m_metaDataBlockMap;			///< map of meta data blocks for writing

	DocnoBlockMap m_docnoBlockMap;				///< map of docno postings for writing
	PosinfoBlockMap m_posinfoBlockMap;			///< map of posinfo postings for writing
	ForwardIndexBlockMap m_forwardIndexBlockMap;		///< map of forward index for writing
	UserAclBlockMap m_userAclBlockMap;			///< map of user rights for writing (forward and inverted)

	KeyMap m_termTypeMap;					///< map of term types
	KeyMap m_termValueMap;					///< map of term values
	KeyMap m_docIdMap;					///< map of document ids
	KeyMap m_userIdMap;					///< map of user ids
	KeyMap m_attributeNameMap;				///< map of document attribute names

	std::map<std::string,Index> m_newDocidMap;		///< map of new document identifiers (docid's allocated in ranges that must be written in the commit, because the were not written immediately)
	int m_nof_documents;					///< total adjustment for the number of documents added minus number of documents deleted
	bool m_commit;						///< true, if the transaction has been committed
	bool m_rollback;					///< true, if the transaction has been rolled back
};

}//namespace
#endif

