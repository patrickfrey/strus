/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ffBlock.hpp"

using namespace strus;

void FfBlock::initFrame()
{
	if (size() < sizeof(int))
	{
		m_ffIndexNodeArray.init( 0, 0);
	}
	else
	{
		int nofFfIndexNodes = size() / sizeof( FfIndexNode);
		const FfIndexNode* ffIndexNodes = (const FfIndexNode*)ptr();
		m_ffIndexNodeArray.init( ffIndexNodes, nofFfIndexNodes);
	}
}

void FfBlockBuilder::setId( const Index& id_)
{
	if (id_ && id_ < m_lastDoc) throw std::runtime_error(_TXT("assigning illegal id to block"));
	m_id = id_;
}

void FfBlockBuilder::append( const Index& docno, int ff)
{
	if (docno < m_lastDoc) throw std::runtime_error(_TXT("document numbers not added in ascending order"));
	if (m_id && m_id < docno) throw std::runtime_error(_TXT("assigned illegal id to block"));

	if (m_ffIndexNodeArray.empty() || !m_ffIndexNodeArray.back().setDocumentFf( docno, ff))
	{
		m_ffIndexNodeArray.push_back( FfIndexNode());
		if (!m_ffIndexNodeArray.back().setDocumentFf( docno, ff))
		{
			throw std::runtime_error( _TXT("corrupt structure in posinfo block builder"));
		}
	}
	m_lastDoc = docno;
}

void FfBlockBuilder::merge( const FfBlockBuilder& blk1, const FfBlockBuilder& blk2, FfBlockBuilder& newblk)
{
	std::size_t idx1 = 0;
	std::size_t idx2 = 0;

	if (blk1.m_ffIndexNodeArray.empty())
	{
		newblk = blk2;
		return;
	}
	if (blk2.m_ffIndexNodeArray.empty())
	{
		newblk = blk1;
		return;
	}
	FfIndexNode const* nd1 = &blk1.m_ffIndexNodeArray[ idx1];
	strus::Index ff1;
	strus::Index docno1 = nd1->skipDoc( 0, ff1);

	FfIndexNode const* nd2 = &blk2.m_ffIndexNodeArray[ idx2];
	strus::Index ff2;
	strus::Index docno2 = nd2->skipDoc( 0, ff2);

	while (docno1 && docno2)
	{
		if (docno1 < docno2)
		{
			newblk.append( docno1, ff1);
			docno1 = nd1->skipDoc( ++docno1, ff1);
			while (!docno1 && ++idx1 < blk1.m_ffIndexNodeArray.size())
			{
				nd1 = &blk1.m_ffIndexNodeArray[ idx1];
				docno1 = nd1->skipDoc( 0, ff1);
			}
		}
		else if (docno1 > docno2)
		{
			newblk.append( docno2, ff2);
			docno2 = nd2->skipDoc( ++docno2, ff2);
			while (!docno2 && ++idx2 < blk2.m_ffIndexNodeArray.size())
			{
				nd2 = &blk2.m_ffIndexNodeArray[ idx2];
				docno2 = nd2->skipDoc( 0, ff2);
			}
		}
		else/*(docno1 == docno2)*/
		{
			strus::Index ff_combined = ff1 > ff2 ? ff1 : ff2;
			newblk.append( docno1, ff_combined);

			docno1 = nd1->skipDoc( ++docno1, ff1);
			while (!docno1 && ++idx1 < blk1.m_ffIndexNodeArray.size())
			{
				nd1 = &blk1.m_ffIndexNodeArray[ idx1];
				docno1 = nd1->skipDoc( 0, ff1);
			}
			docno2 = nd2->skipDoc( ++docno2, ff2);
			while (!docno2 && ++idx2 < blk2.m_ffIndexNodeArray.size())
			{
				nd2 = &blk2.m_ffIndexNodeArray[ idx2];
				docno2 = nd2->skipDoc( 0, ff2);
			}
		}
	}
	while (docno1)
	{
		newblk.append( docno1, ff1);
		docno1 = nd1->skipDoc( ++docno1, ff1);
		while (!docno1 && ++idx1 < blk1.m_ffIndexNodeArray.size())
		{
			nd1 = &blk1.m_ffIndexNodeArray[ idx1];
			docno1 = nd1->skipDoc( 0, ff1);
		}
	}
	while (docno2)
	{
		newblk.append( docno2, ff2);
		docno2 = nd2->skipDoc( ++docno2, ff2);
		while (!docno2 && ++idx2 < blk2.m_ffIndexNodeArray.size())
		{
			nd2 = &blk2.m_ffIndexNodeArray[ idx2];
			docno2 = nd2->skipDoc( 0, ff2);
		}
	}
}

FfBlock FfBlockBuilder::createBlock() const
{
	if (empty()) throw std::runtime_error(_TXT("tried to create empty posinfo block"));

	std::size_t blksize = m_ffIndexNodeArray.size() * sizeof(FfIndexNode);
	return FfBlock( m_id?m_id:m_lastDoc, m_ffIndexNodeArray.data(), blksize, true/*allocated, means copied*/);
}




