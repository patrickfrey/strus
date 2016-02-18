/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_STORAGE_TRANSACTION_HPP_INCLUDED
#define _STRUS_STORAGE_TRANSACTION_HPP_INCLUDED
#include "strus/storageTransactionInterface.hpp"
#include "strus/arithmeticVariant.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "metaDataMap.hpp"
#include "metaDataReader.hpp"
#include "attributeMap.hpp"
#include "booleanBlock.hpp"
#include "userAclMap.hpp"
#include "posinfoBlock.hpp"
#include "invertedIndexMap.hpp"
#include "forwardIndexBlock.hpp"
#include "forwardIndexMap.hpp"
#include "keyMap.hpp"
#include "keyMapInv.hpp"
#include "keyAllocatorInterface.hpp"
#include "private/stringMap.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

namespace strus {

/// \brief Forward declaration
class StorageClient;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \class StorageTransaction
class StorageTransaction
	:public StorageTransactionInterface
{
public:
	///\param[in] peerMessageBuilder_ Builder for peer messages owned by caller
	StorageTransaction( 
		StorageClient* storage_,
		DatabaseClientInterface* database_,
		const MetaDataDescription* metadescr_,
		const Index& maxtypeno_,
		ErrorBufferInterface* errorhnd_);

	~StorageTransaction();

	virtual void deleteDocument(
			const std::string& docid);

	virtual void deleteUserAccessRights(
			const std::string& username);

	virtual StorageDocumentInterface*
		createDocument(
			const std::string& docid_);

	virtual StorageDocumentUpdateInterface*
		createDocumentUpdate(
			const Index& docno_);

	virtual void updateMetaData(
			const Index& docno, const std::string& varname, const ArithmeticVariant& value);

	/// \brief Transaction commit
	virtual bool commit();
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

	void definePosinfoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno, const std::vector<Index>& posinfo);

	void openForwardIndexDocument( const Index& docno);

	void defineForwardIndexTerm(
		const Index& typeno, const Index& pos, const std::string& termstring);

	void closeForwardIndexDocument();
	void countDocument();

private:
	StorageClient* m_storage;				///< Storage to call refresh after commit or rollback
	DatabaseClientInterface* m_database;			///< database handle
	const MetaDataDescription* m_metadescr;			///< description of metadata

	AttributeMap m_attributeMap;				///< map of document attributes for writing
	MetaDataMap m_metaDataMap;				///< map of meta data blocks for writing

	InvertedIndexMap m_invertedIndexMap;			///< map of posinfo postings for writing
	ForwardIndexMap m_forwardIndexMap;			///< map of forward index for writing
	UserAclMap m_userAclMap;				///< map of user rights for writing (forward and inverted)

	KeyMap m_termTypeMap;					///< map of term types
	KeyMap m_termValueMap;					///< map of term values
	KeyMap m_docIdMap;					///< map of document ids
	KeyMap m_userIdMap;					///< map of user ids
	KeyMap m_attributeNameMap;				///< map of document attribute names

	KeyMapInv m_termTypeMapInv;				///< inverse map of term types
	KeyMapInv m_termValueMapInv;				///< inverse map of term values

	int m_nof_documents;					///< total adjustment for the number of documents added minus number of documents deleted
	bool m_commit;						///< true, if the transaction has been committed
	bool m_rollback;					///< true, if the transaction has been rolled back

	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}//namespace
#endif

