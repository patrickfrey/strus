#include "strus/kcfstorage.hpp"
#include "strus/position.hpp"
#include <algorithm>
#include <stdexcept>

using namespace strus;

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

struct Packer
{

};

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
	return docno;
}

std::string StorageImpl::getDocumentId( const DocNumber& /*docnum*/)
{
	return std::string();
}

std::size_t StorageImpl::getDocumentSize( const DocNumber& /*docnum*/)
{
	return 0;
}

DocNumber StorageImpl::getDocumentNumber( const std::string& docid)
{
	return m_db.findDocumentNumber( docid);
}

TermNumber StorageImpl::getTermNumber( const std::string& type, const std::string& value)
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




