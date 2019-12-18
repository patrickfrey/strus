class StructBlock
{
	class MemberScanner
	{
	public:
		MemberScanner()
			:m_structureType(StructureDef::TypeRangeList)
		{
			m_itr.memberIterator=new (mem) StructureMemberRange::Iterator(0,0);
		}
		MemberScanner( const StructureMember* ar_, int size_)
			:m_structureType(StructureDef::TypeRangeList)
		{
			m_itr.memberIterator=new (mem) StructureMember::Iterator(ar_,size_);
		}
		MemberScanner( const StructBlockMemberEnum* ar_, int size_)
			:m_structureType(StructureDef::TypeEnumList)
		{
			m_itr.enumerationIterator=new (mem) StructBlockMemberEnum::Iterator(ar_,size_);
		}
		MemberScanner( const MemberScanner& o)
			:m_structureType(o.m_structureType)
		{
			switch (m_structureType)
			{
				case StructureDef::TypeRangeList:
					m_itr.memberIterator=new (mem) StructureMemberRange::Iterator(*o.m_itr.memberIterator);
					break;
				case StructureDef::TypeEnumList:
					m_itr.enumerationIterator=new (mem) StructBlockMemberEnum::Iterator(*o.m_itr.enumerationIterator);
					break;
			}
		}
		void init( const StructureMemberRange* ar_, int size_)
		{
			m_structureType = StructureDef::TypeRangeList;
			m_itr.memberIterator=new (mem) StructureMemberRange::Iterator(ar_,size_);
		}
		void init( const StructBlockMemberEnum* ar_, int size_)
		{
			m_structureType = StructureDef::TypeEnumList;
			m_itr.enumerationIterator=new (mem) StructBlockMemberEnum::Iterator(ar_,size_);
		}
		IndexRange next()
		{
			switch (m_structureType)
			{
				case StructureDef::TypeRangeList:
					return m_itr.memberIterator->next();
				case StructureDef::TypeEnumList:
					return m_itr.enumerationIterator->next();
			}
		}

		strus::IndexRange skip( Index pos)
		{
			switch (m_structureType)
			{
				case StructureDef::TypeRangeList:
					return m_itr.memberIterator->skip( pos);
				case StructureDef::TypeEnumList:
					return m_itr.enumerationIterator->skip( pos);
			}
			return strus::IndexRange();
		}

	private:
		StructureDef::StructureType m_structureType;
		StructureMember::Iterator m_itr;
	};
};

class StructBlockBuilder
{
/// \brief Take the last structure defined and try to pack it into a enumeration structure if it gets smaller in size
bool tryMoveRangeListBlockMembersToEnumeration();
bool addFittingRepeatMember( const strus::IndexRange& sink);

strus::Index currentRepeatNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
strus::Index nextRepeatNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
strus::Index firstRepeatNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
strus::Index currentMemberNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
strus::Index nextMemberNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
strus::Index nextStructureFirstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
strus::Index resolveDocFirstStructure( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
strus::Index nextDocFirstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
strus::Index nextIndexFirstDoc( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
strus::Index firstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
strus::Index nextNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const;
};


