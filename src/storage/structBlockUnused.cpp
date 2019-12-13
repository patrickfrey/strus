bool StructBlockBuilder::addFittingRepeatMember( const strus::IndexRange& sink)
{
	if (!m_structurear.back().membersSize) return false;
	if ((Index)m_memberar.back().start == StructureRepeat::ID)
	{
		if (m_structurear.back().membersSize < 2 || m_memberar.size() < 2) throw std::runtime_error(_TXT("corrupt index: repeat structure not following a member describing the structure area"));
		StructureMemberRange* mmb = (StructureMemberRange*)&m_memberar[ m_memberar.size()-2];
		const StructureRepeat* rep = (const StructureRepeat*)&m_memberar.back();
		strus::Index membsize = sink.end() - sink.start();
		strus::Index membofs = sink.end() - mmb->end;

		if (membsize == (Index)(int)(unsigned int)rep->size
		&&  membofs  == (Index)(int)(unsigned int)rep->ofs)
		{
			mmb->end += membofs;
			return true;
		}
	}
	else
	{
		strus::Index membsize = sink.end() - sink.start();
		strus::Index membofs = sink.end() - m_memberar.back().end;

		if (membsize == m_memberar.back().end - m_memberar.back().start
		&&  membsize < (Index)std::numeric_limits<unsigned char>::max()
		&&  membofs  < (Index)std::numeric_limits<unsigned char>::max())
		{
			StructureRepeat rep( membofs, membsize);
			m_memberar.back().end += membofs;
			m_memberar.push_back( *(StructureMemberRange*)&rep);
			if (m_structurear.back().membersSize >= StructureDef::MaxMembersSize)
			{
				throw std::runtime_error(_TXT("number of structure members exceeds maximum size"));
			}
			++m_structurear.back().membersSize;
			return true;
		}
	}
	return false;
}

bool StructBlockBuilder::tryMoveRangeListBlockMembersToEnumeration()
{
	if (m_structurear.empty()) return false;
	StructureDef& st = m_structurear.back();
	if (st.structureType != StructureDef::TypeRangeList) return false;
	if ((std::size_t)st.membersIdx + st.membersSize != m_memberar.size()) return false;
	StructBlock::MemberScanner mscan( m_memberar.data()+st.membersIdx, st.membersSize);
	std::vector<StructBlockMemberEnum> enumar;
	std::size_t maxSize = (std::size_t)st.membersSize * sizeof(StructureMemberRange);
	strus::IndexRange rg = mscan.skip( 0);
	for (; rg.defined(); rg = mscan.skip( rg.end()))
	{
		strus::Index ri = rg.start(), re = rg.end();
		for (; ri < re; ++ri)
		{
			if (enumar.empty() || enumar.back().full())
			{
				enumar.push_back( StructBlockMemberEnum());
			}
			if (!enumar.back().append( ri)) return false;
		}
		if (enumar.size() * sizeof(StructBlockMemberEnum) >= maxSize) return false;
		if (enumar.size() >= StructureDef::MaxMembersSize) return false;
	}
	st.structureType = StructureDef::TypeEnumList;
	st.membersIdx = m_enumMemberar.size();
	st.membersSize = enumar.size();
	m_enumMemberar.insert( m_enumMemberar.end(), enumar.begin(), enumar.end());
	return true;
}


strus::Index StructBlockBuilder::currentRepeatNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	src = strus::IndexRange(
		m_structurear[ cursor.stuidx].header_start,
		m_structurear[ cursor.stuidx].header_end);
	sink = IndexRange( cursor.repstart, cursor.repstart + cursor.repsize);
	return cursor.docno;
}

strus::Index StructBlockBuilder::firstRepeatNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	if (cursor.stutype == StructureDef::TypeRangeList && cursor.mbridx+1 < cursor.mbrend && m_memberar[ cursor.mbridx+1].start == StructureRepeat::ID)
	{
		cursor.repstart = m_memberar[ cursor.mbridx].start;
		cursor.repend = m_memberar[ cursor.mbridx].end;
		++cursor.mbridx;
		const StructureRepeat* rep = (const StructureRepeat*)&m_memberar[ cursor.mbridx];
		cursor.repofs = (Index)(int)(unsigned int)rep->ofs;
		cursor.repsize = (Index)(int)(unsigned int)rep->size;
		return currentRepeatNode( cursor, src, sink);
	}
	return 0;
}

strus::Index StructBlockBuilder::nextRepeatNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	if (cursor.repstart < cursor.repend)
	{
		cursor.repstart += cursor.repofs;
		if (cursor.repstart >= cursor.repend)
		{
			cursor.repstart = 0;
			cursor.repend = 0;
			cursor.repofs = 0;
			cursor.repsize = 0;
		}
		else
		{
			return currentRepeatNode( cursor, src, sink);
		}
	}
	return 0;
}

strus::Index StructBlockBuilder::currentMemberNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	switch (cursor.stutype)
	{
		case StructureDef::TypeRangeList:
			if (cursor.mbridx >= cursor.mbrend)
			{
				return 0;
			}
			else
			{
				src = strus::IndexRange(
					m_structurear[ cursor.stuidx].header_start,
					m_structurear[ cursor.stuidx].header_end);
				sink = strus::IndexRange(
					m_memberar[ cursor.mbridx].start,
					m_memberar[ cursor.mbridx].end);
			}
			return cursor.docno;
		case StructureDef::TypeEnumList:
			src = strus::IndexRange(
				m_structurear[ cursor.stuidx].header_start,
				m_structurear[ cursor.stuidx].header_end);
			sink = cursor.mitr.current();
			return sink.defined() ? cursor.docno : 0;
			
	}
	return 0;
}

strus::Index StructBlockBuilder::nextMemberNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	switch (cursor.stutype)
	{
		case StructureDef::TypeRangeList:
			++cursor.mbridx;
			if (firstRepeatNode( cursor, src, sink))
			{
				return cursor.docno;
			}
			else
			{
				return currentMemberNode( cursor, src, sink);
			}
		case StructureDef::TypeEnumList:
			sink = cursor.mitr.next();
			if (sink.defined()) return cursor.docno;
	}
	return 0;
}

strus::Index StructBlockBuilder::nextStructureFirstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	for (++cursor.stuidx; cursor.stuidx < cursor.stuend; ++cursor.stuidx)
	{
		const StructureDef& stdef = m_structurear[ cursor.stuidx];
		cursor.stutype = (StructureDef::StructureType)stdef.structureType;
		cursor.mbridx = stdef.membersIdx;
		cursor.mbrend = cursor.mbridx + stdef.membersSize;
		if (cursor.mbridx < cursor.mbrend)
		{
			switch (cursor.stutype)
			{
				case StructureDef::TypeRangeList:
					if (firstRepeatNode( cursor, src, sink))
					{
						return cursor.docno;
					}
					else
					{
						return currentMemberNode( cursor, src, sink);
					}
				case StructureDef::TypeEnumList:
					cursor.mitr = StructBlockMemberEnum::Iterator( m_enumMemberar.data()+cursor.mbridx, cursor.mbrend-cursor.mbridx);
					sink = cursor.mitr.next();
					if (sink.defined()) return cursor.docno;
			}
		}
	}
	return 0;
}

strus::Index StructBlockBuilder::resolveDocFirstStructure( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	strus::Index ref = m_docIndexNodeArray[ cursor.aridx].ref[ cursor.docidx];
	cursor.stuidx = m_structurelistar[ ref].idx;
	cursor.stuend = cursor.stuidx + m_structurelistar[ ref].size;
	--cursor.stuidx;
	return nextStructureFirstNode( cursor, src, sink);
}

strus::Index StructBlockBuilder::nextDocFirstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	cursor.docno = m_docIndexNodeArray[ cursor.aridx].nextDoc( cursor.docidx);
	return cursor.docno ? resolveDocFirstStructure( cursor, src, sink) : 0;
}

strus::Index StructBlockBuilder::nextIndexFirstDoc( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	for (++cursor.aridx; cursor.aridx < m_docIndexNodeArray.size(); ++cursor.aridx)
	{
		cursor.docno = m_docIndexNodeArray[ cursor.aridx].firstDoc( cursor.docidx);
		return cursor.docno ? resolveDocFirstStructure( cursor, src, sink) : 0;
	}
	return 0;
}

strus::Index StructBlockBuilder::firstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	cursor.aridx = -1;
	return nextIndexFirstDoc( cursor, src, sink);
}

strus::Index StructBlockBuilder::nextNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	if (nextRepeatNode( cursor, src, sink)) return cursor.docno;
	if (nextMemberNode( cursor, src, sink)) return cursor.docno;
	if (nextStructureFirstNode( cursor, src, sink)) return cursor.docno;
	if (nextDocFirstNode( cursor, src, sink)) return cursor.docno;
	if (nextIndexFirstDoc( cursor, src, sink)) return cursor.docno;
	return 0;
}
