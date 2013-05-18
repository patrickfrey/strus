#include "inserter.hpp"

using namespace strus;
using namespace strus::inserter;

static bool compareDocPosition( const DocPosition& p1, const DocPosition& p2)
{
	return (p1 < p2);
}

DocNumber inserter::storeDocument( StorageDB& db, const std::string& docid, const Storage::TermDocPositionMap& content)
{
	typedef Storage::TermDocPositionMap TermDocPositionMap;
	typedef std::map<TermNumber,std::string> TermPackedPositionMap;
	TermPackedPositionMap tpmap;
	std::map<TermNumber, bool> tpmap_sort;
	std::string docno_packed;

	TermDocPositionMap::const_iterator ci = content.begin(), ce = content.end();
	for (; ci != ce; ++ci)
	{
		Storage::DocPositionList poslist = ci->second;

		TermNumber tn = db.findTermNumber( ci->first.term.type, ci->first.term.value);
		if (!tn)
		{
			tn = db.insertTermNumber( ci->first.term.type, ci->first.term.value);
		}
		Storage::DocPositionList::const_iterator pi = poslist.begin(), pe = poslist.end();
		DocPosition prevpos = (pi==pe)?0:*pi;
		for (++pi; pi != pe; ++pi)
		{
			if (*pi < prevpos) break;
			prevpos = *pi;
		}
		if (pi != pe)
		{
			std::sort( poslist.begin(), poslist.end(), compareDocPosition);
		}
		std::unique( poslist.begin(), poslist.end(), compareDocPosition);

		std::string poslist_packed;
		pi = poslist.begin();
		DocPosition prevpos = 0;
		if (pi != poslist.end())
		{
			packIndex( poslist_packed, prevpos = *pi);
		}
		for (++pi; pi != poslist.end(); ++pi)
		{
			packIndex( poslist_packed, *pi - prevpos);
			prevpos = *pi;
		}
		tpmap.insert( std::pair<TermNumber,std::string>( tn, poslist_packed));
	}
	db.lock();
	DocNumber docno = db.findDocumentNumber( docid);
	if (docno) throw std::runtime_error( std::string("document '") + docid + "' already inserted");
	docno = db.insertDocumentNumber( docid);
	packIndex( docno_packed, docno);

	TermPackedPositionMap::const_iterator ti = tpmap.begin(), te = tpmap.end();
	for (; ti != te; ++ti)
	{
		std::pair<BlockNumber,bool> ib = db.getTermBlockNumber( ti->first);
		BlockNumber bn = ib.first;
		bool isSmallBlock = ib.second;
		if (!bn)
		{
			std::string blockdata;
			blockdata.append( docno_packed);
			blockdata.append( poslist_packed);
			blockdata.push_back( 0);

			if (blockdata.size() <= StorageDB::SmallBlockSize)
			{
				bn = db.allocSmallBlock();
				writeSmallBlock( bn, blockdata.c_str(), blockdata.size());
				db.setTermBlockNumber( ti->first, bn, true);
				continue;
			}
			unsigned char blocktype = 0;
			std::size_t bsize = blockdata.size();
			std::size_t uu = StorageDB::PackBlock::Size;

			while (bsize >= uu)
			{
				uu *= StorageDB::INodeBlock::NofElem;
				++blocktype;
				if (blocktype > 4) throw std::bad_alloc();
			}
			unsigned char bi;
			StorageDB::INodeBlock inodeblock[ 4];
			BlockNumber inodebn[4];

			for (bi = 0; bi != blocktype; ++bi)
			{
				inodebn[bi] = db.allocIndexBlock();
				std::memset( &inodeblock[bi], 0, sizeof( inodeblock[bi]));
				inodeblock[bi].hdr.type = blocktype-bi;
				block.hdr.fillpos = 1;
				writeIndexBlock( bn, &block);
				db.setTermBlockNumber( ti->first, bn, false);
				continue;
			}
			if (blockdata.size() <= StorageDB::PackBlock::Size)
			{
				bn = db.allocIndexBlock();
				StorageDB::PackBlock block;
				std::memset( &block, 0, sizeof( block));
				block.hdr.type = 0;
				block.hdr.fillpos = blockdata.size();
				writeIndexBlock( bn, &block);
				db.setTermBlockNumber( ti->first, bn, false);
				continue;
			}

		}
	}

	db.unlock();
	return docno;
}


