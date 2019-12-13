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
