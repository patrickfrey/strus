#include "strus/storagelib.hpp"
#include "strus/position.hpp"
#include "database.hpp"
#include <algorithm>
#include <stdexcept>

using namespace strus;

///\class StorageImpl
///\brief Implementation for the storage of IR terms with their occurrencies with kytocabinet and files for the blocks
class StorageImpl
	:public Storage
{
public:
	StorageImpl( const std::string& name, const std::string& path, bool writemode=false)
		:m_db(name,path,writemode){}

	virtual DocNumber storeDocument( const std::string& docid, const TermDocPositionMap& content)=0;

	virtual std::string getDocumentId( const DocNumber& docnum);
	virtual std::pair<std::string,std::string> getTerm( const TermNumber& termnum);

	virtual DocNumber findDocumentNumber( const std::string& docid);
	virtual TermNumber findTermNumber( const std::string& type, const std::string& value);

	virtual bool openIterator( PositionChunk& itr, const TermNumber& termnum);
	virtual bool nextIterator( PositionChunk& itr);
	virtual void closeIterator( PositionChunk& itr);

private:
	StorageDB m_db;
};

void strus::createStorage( const char* name, const char* path)
{
	StorageDB::create( name, path?path:"");
}

Storage* strus::allocStorage( const char* name, const char* path, bool writemode)
{
	StorageImpl* rt = new StorageImpl( name, path, writemode);
	return rt;
}

void strus::destroyStorage( Storage* storage)
{
	delete storage;
}


typedef Storage::Document::Term Term;

static bool compareTerm( const Term& t1, const Term& t2)
{
	if (t1.number < t2.number) return true;
	if (t1.position < t2.position) return true;
	return false;
}

DocNumber StorageImpl::storeDocument( const std::string& docid, const TermDocPositionMap& content)
{
	return inserter::storeDocument( m_db, docid, content);
}

std::string StorageImpl::getDocumentId( const DocNumber& docnum)
{
	return m_db.getDocumentId( docnum);
}

std::pair<std::string,std::string> StorageImpl::getTerm( const TermNumber& termnum)
{
	return m_db.getTerm( termnum);
}

DocNumber StorageImpl::findDocumentNumber( const std::string& docid)
{
	return m_db.findDocumentNumber( docid);
}

TermNumber StorageImpl::findTermNumber( const std::string& type, const std::string& value)
{
	return m_db.findTermNumber( type, value);
}

bool StorageImpl::openIterator( PositionChunk& /*itr*/, const TermNumber& /*termnum*/)
{
	return false;
}

bool StorageImpl::nextIterator( PositionChunk& /*itr*/)
{
	return false;
}

void StorageImpl::closeIterator( PositionChunk& /*itr*/)
{
}


