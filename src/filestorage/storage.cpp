#include "strus/storagelib.hpp"
#include "strus/position.hpp"
#include "database.hpp"
#include "inserter.hpp"
#include <algorithm>
#include <stdexcept>

using namespace strus;

///\class StorageImpl
///\brief Implementation for the storage of IR terms with their occurrencies with kytocabinet and files for the blocks
class StorageImpl
	:public Storage
{
public:
	StorageImpl( const std::string& name, const std::string& path)
		:m_db(name,path){}

	virtual void open( Mode mode_);
	virtual void close();

	virtual DocNumber storeDocument( const std::string& docid, const TermDocPositionMap& content);

	virtual std::string getDocumentId( const DocNumber& docnum) const;
	virtual std::pair<std::string,std::string> getTerm( const TermNumber& termnum) const;

	virtual DocNumber findDocumentNumber( const std::string& docid) const;
	virtual TermNumber findTermNumber( const std::string& type, const std::string& value) const;

	virtual bool openIterator( PositionChunk& itr, const TermNumber& termnum);
	virtual bool nextIterator( PositionChunk& itr);
	virtual void closeIterator( PositionChunk& itr);

private:
	StorageDB m_db;
};

void strus::createStorage( const char* name, const char* cfg)
{
	StorageDB::Configuration cfgstruct(cfg?cfg:"");
	StorageDB::create( name, cfgstruct);
}

Storage* strus::allocStorage( const char* name, const char* cfg)
{
	StorageDB::Configuration cfgstruct(cfg?cfg:"");
	StorageImpl* rt = new StorageImpl( name, cfgstruct.path);
	return rt;
}

void strus::destroyStorage( Storage* storage)
{
	delete storage;
}

void StorageImpl::open( Mode mode_)
{
	switch (mode_)
	{
		case Storage::Read: m_db.open( false); break;
		case Storage::Write: m_db.open( true); break;
	}
	throw std::runtime_error( "illegal mode parameter to open storage");
}

void StorageImpl::close()
{
	m_db.close();
}

DocNumber StorageImpl::storeDocument( const std::string& docid, const TermDocPositionMap& content)
{
	return inserter::storeDocument( m_db, docid, content);
}

std::string StorageImpl::getDocumentId( const DocNumber& docnum) const
{
	return m_db.getDocumentId( docnum);
}

std::pair<std::string,std::string> StorageImpl::getTerm( const TermNumber& termnum) const
{
	return m_db.getTerm( termnum);
}

DocNumber StorageImpl::findDocumentNumber( const std::string& docid) const
{
	return m_db.findDocumentNumber( docid);
}

TermNumber StorageImpl::findTermNumber( const std::string& type, const std::string& value) const
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


