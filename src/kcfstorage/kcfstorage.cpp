#include "strus/kcfstorage.hpp"

using namespace strus;

bool compareTerm( const Storage::Document::Term& t1, const Storage::Document::Term& t2)
{
	if (t1.number < t2.number) return true;
	if (t1.position < t2.position) return true;
	return false;
}

struct Packer
{

};

DocNumber StorageImpl::storeDocument( const Document& doc)
{
	std::vector<Term> tar( doc.terms());
	std::sort( tar.begin(), tar.end(), compareTerm);
	DocNumber docno = getDocumentNumber( doc.docid);
}

std::string StorageImpl::getDocumentId( const DocNumber& docnum)
{
}

std::size_t StorageImpl::getDocumentSize( const DocNumber& docnum)
{
}

DocNumber StorageImpl::getDocumentNumber( const std::string& docid)
{
}

TermNumber StorageImpl::getTermNumber( const std::string& type, const std::string& value)
{
}

bool StorageImpl::openIterator( PositionChunk& itr, const TermNumber& termnum)
{
}

bool StorageImpl::nextIterator( PositionChunk& itr)
{
}

void StorageImpl::closeIterator( PositionChunk& itr)
{
}




