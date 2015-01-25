#include "metaDataRestriction.cpp"
#include "metaDataDescription.cpp"
#include "metaDataElement.cpp"
#include "metaDataRecord.cpp"
#include "floatConversions.cpp"
#include "arithmeticVariantAsString.cpp"

using namespace strus;

//[PF:HACK] Resolve unresolved externals not used
void KeyValueStorage::store( const BlockKey&, const KeyValueStorage::Value&, leveldb::WriteBatch&)
{}

const KeyValueStorage::Value* KeyValueStorage::load( const BlockKey&, const Index&)
{
	return 0;
}


