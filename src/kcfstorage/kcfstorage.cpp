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

	virtual DocNumber storeDocument( const Document& doc);

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

static bool compareDocPosition( const DocPosition& p1, const DocPosition& p2)
{
	return (p1 < p2);
}

typedef std::map<TermNumber, std::vector<DocPosition> > TermMap;

DocNumber StorageImpl::storeDocument( const Document& doc)
{
	std::vector<Term> tar( doc.terms());
	std::sort( tar.begin(), tar.end(), compareTerm);
	DocNumber docno = m_db.insertDocumentNumber( doc.docid());

	TermMap tmap;
	std::map<TermNumber, bool> tmap_sort;
	std::vector<Term>::const_iterator ti = doc.terms().begin(), te = doc.terms().end();

	for (; ti != te; ++ti)
	{
		std::vector<DocPosition>& ref = tmap[ ti->number];
		if (!ref.empty())
		{
			if (ti->position == ref.back()) continue;
			if (ti->position < ref.back()) tmap_sort[ti->number] = true;
		}
		ref.push_back( ti->position);
	}
	// sort document positions per term
	std::map<TermNumber, bool>::const_iterator si = tmap_sort.begin(), se = tmap_sort.end();
	for (; si != se; ++si)
	{
		TermMap::iterator mi = tmap.find( si->first);
		if (mi == tmap.end()) throw std::logic_error( "internal error: corrupt term map");
		std::sort( mi->second.begin(), mi->second.end(), compareDocPosition);
	}
	// pack occurrencies positions per term
	TermMap::const_iterator mi = tmap.begin(), me = tmap.end();
	for (; mi != me; ++mi)
	{

	}
	return docno;
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


