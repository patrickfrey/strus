/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/reference.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/lib/error.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"
#include "strus/lib/queryproc.hpp"
#include "strus/lib/queryeval.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/shared_ptr.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/valueIteratorInterface.hpp"
#include "private/errorUtils.hpp"
#include "random.hpp"
#include <string>
#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <map>
#include <set>

static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::Random g_random;

class Storage
{
public:
	Storage(){}
	Storage( const Storage& o)
		:dbi(o.dbi),sti(o.sti),sci(o.sci){}
	~Storage(){}

	strus::shared_ptr<strus::DatabaseInterface> dbi;
	strus::shared_ptr<strus::StorageInterface> sti;
	strus::shared_ptr<strus::StorageClientInterface> sci;

	void open( const char* options, bool reset);
	void close()
	{
		sci.reset();
		sti.reset();
		dbi.reset();
	}
};


void Storage::open( const char* config, bool reset)
{
	dbi.reset( strus::createDatabaseType_leveldb( "", g_errorhnd));
	if (!dbi.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	sti.reset( strus::createStorageType_std( "", g_errorhnd));
	if (!sti.get() || g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	if (reset)
	{
		(void)dbi->destroyDatabase( config);
		(void)g_errorhnd->fetchError();
	
		if (!sti->createStorage( config, dbi.get()))
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
	}
	const strus::StatisticsProcessorInterface* statisticsMessageProc = 0;
	sci.reset( sti->createClient( config, dbi.get(), statisticsMessageProc));
	if (!sci.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
}

static void destroyStorage( const char* config)
{
	strus::shared_ptr<strus::DatabaseInterface> dbi;
	dbi.reset( strus::createDatabaseType_leveldb( "", g_errorhnd));
	if (!dbi.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	dbi->destroyDatabase( config);
}

static void testDeleteNonExistingDoc()
{
	Storage storage;
	storage.open( "path=storage", true);
	strus::local_ptr<strus::StorageTransactionInterface> transactionInsert( storage.sci->createTransaction());
	strus::local_ptr<strus::StorageDocumentInterface> doc( transactionInsert->createDocument( "ABC"));
	doc->addSearchIndexTerm( "word", "hello", 1);
	doc->addSearchIndexTerm( "word", "world", 2);
	doc->addForwardIndexTerm( "word", "Hello, ", 1);
	doc->addForwardIndexTerm( "word", "world!", 2);
	doc->setAttribute( "title", "Hello World");
	doc->done();
	transactionInsert->commit();

	std::cerr << "Document inserted " << storage.sci->documentNumber( "ABC") << std::endl;

	strus::local_ptr<strus::StorageTransactionInterface> transactionDelete1( storage.sci->createTransaction());
	transactionDelete1->deleteDocument( "ABC");
	transactionDelete1->commit();

	std::cerr << "Document deleted " << storage.sci->documentNumber( "ABC") << std::endl;

	strus::local_ptr<strus::StorageTransactionInterface> transactionDelete2( storage.sci->createTransaction());
	transactionDelete2->deleteDocument( "ABC");
	transactionDelete2->commit();

	std::cerr << "Document deleted " << storage.sci->documentNumber( "ABC") << std::endl;
	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
}

static std::string featureString( const std::string& prefix, unsigned int num)
{
	return strus::string_format( "%s%u", prefix.c_str(), num);
}

static void dumpStorage( const std::string& config)
{
	strus::local_ptr<strus::DatabaseInterface> dbi( strus::createDatabaseType_leveldb( "", g_errorhnd));
	if (!dbi.get()) throw std::runtime_error( g_errorhnd->fetchError());
	strus::local_ptr<strus::StorageInterface> sti( strus::createStorageType_std( "", g_errorhnd));
	if (!sti.get()) throw std::runtime_error( g_errorhnd->fetchError());
	strus::local_ptr<strus::StorageDumpInterface> dump( sti->createDump( config, dbi.get(), ""));

	const char* chunk;
	std::size_t chunksize;
	std::string dumpcontent;
	while (dump->nextChunk( chunk, chunksize))
	{
		dumpcontent.append( chunk, chunksize);
	}
	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	else
	{
		std::cout << dumpcontent << std::endl;
	}
}

static void testSimpleDocumentUpdate()
{
	Storage storage;
	storage.open( "path=storage", true);
	{
		std::string docid( featureString("D",1));

		strus::local_ptr<strus::StorageTransactionInterface> transaction( storage.sci->createTransaction());
		strus::local_ptr<strus::StorageDocumentInterface> doc( transaction->createDocument( docid));
		doc->addSearchIndexTerm( "a", featureString("a", 1), 1);
		doc->done();
		transaction->commit();

		transaction.reset( storage.sci->createTransaction());
		strus::Index docno = storage.sci->documentNumber( docid);
		strus::local_ptr<strus::StorageDocumentUpdateInterface> update( transaction->createDocumentUpdate( docno));
		update->addSearchIndexTerm( "a", featureString("a", 1), 1);
		update->addSearchIndexTerm( "a", featureString("a", 2), 1);
		update->done();
		transaction->commit();
	}
	storage.close();
	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	dumpStorage( "path=storage");
}

struct OccurrenceDef
{
	std::string docid;
	std::string type;
	std::string value;
	unsigned int pos;

	OccurrenceDef( const std::string& docid_, const std::string& type_, const std::string& value_, unsigned int pos_)
		:docid(docid_),type(type_),value(value_),pos(pos_){}
	OccurrenceDef( const OccurrenceDef& o)
		:docid(o.docid),type(o.type),value(o.value),pos(o.pos){}

	bool operator < (const OccurrenceDef& o) const
	{
		if (docid == o.docid)
		{
			if (type == o.type)
			{
				if (value == o.value)
				{
					return pos < o.pos;
				}
				else
				{
					return value < o.value;
				}
			}
			else
			{
				return type < o.type;
			}
		}
		else
		{
			return docid < o.docid;
		}
	}
};

struct ClearTypeDef
{
	std::string docid;
	std::string type;

	ClearTypeDef( const std::string& docid_, const std::string& type_)
		:docid(docid_),type(type_){}
	ClearTypeDef( const ClearTypeDef& o)
		:docid(o.docid),type(o.type){}

	bool operator < (const ClearTypeDef& o) const
	{
		if (docid == o.docid)
		{
			return type < o.type;
		}
		else
		{
			return docid < o.docid;
		}
	}
};

struct FeatDef
{
	std::string type;
	std::string value;

	FeatDef( const std::string& type_, const std::string& value_)
		:type(type_),value(value_){}
	FeatDef( const FeatDef& o)
		:type(o.type),value(o.value){}

	bool operator < (const FeatDef& o) const
	{
		if (type == o.type)
		{
			return value < o.value;
		}
		else
		{
			return type < o.type;
		}
	}
};

static std::map<FeatDef, unsigned int> getDfMap( const std::set<OccurrenceDef>& occurrenceSet)
{
	std::map<FeatDef, unsigned int> rt;
	std::set<OccurrenceDef>::const_iterator oi = occurrenceSet.begin(), oe = occurrenceSet.end();
	while (oi != oe)
	{
		std::set<FeatDef> fset;
		const std::string& docid = oi->docid;
		for (; oi != oe && oi->docid == docid; ++oi)
		{
			fset.insert( FeatDef( oi->type, oi->value));
		}
		std::set<FeatDef>::const_iterator fi = fset.begin(), fe = fset.end();
		for (; fi != fe; ++fi)
		{
			rt[ *fi] += 1;
		}
	}
	return rt;
}

static bool check_storage_df( strus::StorageClientInterface* storage, const std::set<OccurrenceDef>& occurrenceset, const char* statestr, unsigned int nofFeats, unsigned int nofTypes)
{
	bool rt = true;
	std::map<FeatDef, unsigned int> dfmap_searchindex = getDfMap( occurrenceset);
	for (unsigned int fi=1; fi<=nofFeats; ++fi)
	{
		for (unsigned int ti=1; ti <= nofTypes; ++ti)
		{
			FeatDef feat( featureString("t", ti), featureString("v", fi));
			unsigned int df = storage->documentFrequency( feat.type, feat.value);
			unsigned int df_expected = 0;
			std::map<FeatDef, unsigned int>::const_iterator dfi = dfmap_searchindex.find( feat);
			if (dfi != dfmap_searchindex.end())
			{
				df_expected = dfi->second;
			}
			if (df_expected != df)
			{
				rt = false;
				std::cerr << strus::string_format("document frequency of '%s %s' %s (%u) not as expected (%u)", feat.type.c_str(), feat.value.c_str(), statestr, df, df_expected) << std::endl;
			}
		}
	}
	return rt;
}

static std::set<OccurrenceDef> calculateUpdateOccurrenceDef(
		const std::set<OccurrenceDef>& origset,
		const std::set<OccurrenceDef>& updateset, 
		const std::set<ClearTypeDef>& clearset)
{
	std::set<OccurrenceDef> rt;
	std::set<OccurrenceDef>::const_iterator oi = origset.begin(), oe = origset.end();
	std::set<OccurrenceDef>::const_iterator ui = updateset.begin(), ue = updateset.end();
	while (oi != oe && ui != ue)
	{
		if (oi->docid < ui->docid)
		{
			ClearTypeDef cdef( oi->docid, oi->type);
			if (clearset.find( cdef) == clearset.end())
			{
				rt.insert( *oi);
			}
			++oi;
		}
		else if (oi->docid > ui->docid)
		{
			++ui;
		}
		else if (oi->type < ui->type)
		{
			ClearTypeDef cdef( oi->docid, oi->type);
			if (clearset.find( cdef) == clearset.end())
			{
				rt.insert( *oi);
			}
			++oi;
		}
		else if (oi->type > ui->type)
		{
			++ui;
		}
		else
		{
			const std::string& type = oi->type;
			const std::string& docid = oi->docid;
			for (; oi != oe && oi->docid == docid && oi->type == type; ++oi){}
			for (; ui != ue && ui->docid == docid && ui->type == type; ++ui){}
		}
	}
	for (; oi != oe; ++oi)
	{
		ClearTypeDef cdef( oi->docid, oi->type);
		if (clearset.find( cdef) == clearset.end())
		{
			rt.insert( *oi);
		}
	}
	rt.insert( updateset.begin(), updateset.end());
	return rt;
}


static void testDocumentUpdate()
{
	bool rt = true;
	Storage storage;
	enum {NofDocs=1000,NofFeats=20,NofTypes=5};
	std::set<OccurrenceDef> occurrence_searchindex;
	std::set<OccurrenceDef> occurrence_forwardindex;

	storage.open( "path=storage", true);
	{
		strus::local_ptr<strus::StorageTransactionInterface> transaction( storage.sci->createTransaction());
		for (unsigned int di=1; di<=NofDocs; ++di)
		{
			std::string docid( featureString("D",di));
			strus::local_ptr<strus::StorageDocumentInterface> doc( transaction->createDocument( docid));
			for (unsigned int fi=1; fi <= NofFeats; ++fi)
			{
				if (g_random.get( 0, 2) == 0) continue;
				for (unsigned int ti=1; ti <= NofTypes; ++ti)
				{
					if (g_random.get( 0, 2) == 0) continue;
					OccurrenceDef occ( docid, featureString("t", ti), featureString("v", fi), fi);
					occurrence_searchindex.insert( occ);
					doc->addSearchIndexTerm( occ.type, occ.value, occ.pos);
				}
				for (unsigned int ti=1; ti <= NofTypes; ++ti)
				{
					if (g_random.get( 0, 2) == 0) continue;
					OccurrenceDef occ( docid, featureString("t", ti), featureString("v", fi), fi);
					occurrence_forwardindex.insert( occ);
					doc->addForwardIndexTerm( occ.type, occ.value, occ.pos);
				}
			}
			doc->setAttribute( "title", featureString("title_", di));
			doc->done();
		}
		transaction->commit();
		rt &= check_storage_df( storage.sci.get(), occurrence_searchindex, "before update", NofFeats, NofTypes);
		storage.close();

		dumpStorage( "path=storage");
	}
	storage.open( "path=storage", false);
	{
		std::set<ClearTypeDef> clear_searchindex;
		std::set<ClearTypeDef> clear_forwardindex;
		std::set<OccurrenceDef> occurrence_update_searchindex;
		std::set<OccurrenceDef> occurrence_update_forwardindex;

		strus::local_ptr<strus::StorageTransactionInterface> transaction( storage.sci->createTransaction());
		for (unsigned int di=1; di<=NofDocs; ++di)
		{
			if (g_random.get( 0, NofDocs) == 1) continue;

			std::string docid( featureString("D",di));
			strus::Index docno = storage.sci->documentNumber( docid);

			strus::local_ptr<strus::StorageDocumentUpdateInterface> doc( transaction->createDocumentUpdate( docno));
			for (unsigned int fi=1; fi <= NofFeats; ++fi)
			{
				if (g_random.get( 0, NofFeats) == 1) continue;
				for (unsigned int ti=1; ti <= NofTypes; ++ti)
				{
					if (g_random.get( 0, 2) == 0) continue;
					if (g_random.get( 0, 2) == 0)
					{
						ClearTypeDef def( docid, featureString( "t", ti));
						if (g_random.get( 0, 2) == 0)
						{
							doc->clearSearchIndexTerm( def.type);
							clear_searchindex.insert( def);
						}
						else
						{
							doc->clearForwardIndexTerm( def.type);
							clear_forwardindex.insert( def);
						}
						break;
					}
					else
					{
						OccurrenceDef occ( docid, featureString("t", ti), featureString("v", fi), fi);
						if (g_random.get( 0, 2) == 0)
						{
							occurrence_update_searchindex.insert( occ);
							doc->addSearchIndexTerm( occ.type, occ.value, occ.pos);
						}
						else
						{
							occurrence_update_forwardindex.insert( occ);
							doc->addForwardIndexTerm( occ.type, occ.value, occ.pos);
						}
					}
				}
			}
			if (di % 2 == 0)
			{
				doc->setAttribute( "title", featureString("title_updated_", di));
			}
			doc->done();
		}
		transaction->commit();

		occurrence_searchindex
			= calculateUpdateOccurrenceDef(
				occurrence_searchindex, occurrence_update_searchindex, clear_searchindex);
		rt &= check_storage_df( storage.sci.get(), occurrence_searchindex, "after update", NofFeats, NofTypes);
	}
	storage.close();
	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	dumpStorage( "path=storage");
	if (!rt)
	{
		throw std::runtime_error( "got errors, see std error output");
	}
}

static void testTermTypeIterator()
{
	Storage storage;
	storage.open( "path=storage", true);
	strus::local_ptr<strus::StorageTransactionInterface> transactionInsert( storage.sci->createTransaction());
	strus::local_ptr<strus::StorageDocumentInterface> doc( transactionInsert->createDocument( "ABC"));
	for (unsigned int ii=0; ii<100; ++ii)
	{
		std::string key( "w");
		key.push_back( '0' + (ii % 10));
		key.push_back( '0' + (ii / 10) % 10);

		doc->addSearchIndexTerm( key, "hello", 1);
		doc->addSearchIndexTerm( key, "world", 2);
	}
	doc->done();
	transactionInsert->commit();

	strus::local_ptr<strus::ValueIteratorInterface> itr( storage.sci->createTermTypeIterator());
	std::vector<std::string> types = itr->fetchValues( 3);
	std::string res;
	while (types.size() > 0)
	{
		std::vector<std::string>::const_iterator ti = types.begin(), te = types.end();
		for (; ti != te; ++ti)
		{
			res.append( *ti);
			res.push_back( ' ');
		}
		types = itr->fetchValues( 3);
	}
	itr->skip( "w61", 3);
	types = itr->fetchValues( 2);
	std::vector<std::string>::const_iterator ti = types.begin(), te = types.end();
	for (; ti != te; ++ti)
	{
		res.append( *ti);
		res.push_back( ' ');
	}
	itr->skip( "w6", 3);
	types = itr->fetchValues( 2);
	ti = types.begin(), te = types.end();
	for (; ti != te; ++ti)
	{
		res.append( *ti);
		res.push_back( ' ');
	}
	itr->skip( "w", 3);
	types = itr->fetchValues( 2);
	ti = types.begin(), te = types.end();
	for (; ti != te; ++ti)
	{
		res.append( *ti);
		res.push_back( ' ');
	}
	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	if (res != "w00 w01 w02 w03 w04 w05 w06 w07 w08 w09 w10 w11 w12 w13 w14 w15 w16 w17 w18 w19 w20 w21 w22 w23 w24 w25 w26 w27 w28 w29 w30 w31 w32 w33 w34 w35 w36 w37 w38 w39 w40 w41 w42 w43 w44 w45 w46 w47 w48 w49 w50 w51 w52 w53 w54 w55 w56 w57 w58 w59 w60 w61 w62 w63 w64 w65 w66 w67 w68 w69 w70 w71 w72 w73 w74 w75 w76 w77 w78 w79 w80 w81 w82 w83 w84 w85 w86 w87 w88 w89 w90 w91 w92 w93 w94 w95 w96 w97 w98 w99 w61 w62 w60 w61 w00 w01 ")
	{
		throw std::runtime_error("result not as expected");
	}
}

struct Feature
{
	enum Kind
	{
		SearchIndex,
		ForwardIndex,
		Attribute,
		MetaData
	};
	Kind kind;
	std::string type;
	std::string value;
	unsigned int pos;

	Feature( Kind kind_, const std::string& type_, const std::string& value_, unsigned int pos_=0)
		:kind(kind_),type(type_),value(value_),pos(pos_){}
	Feature( const Feature& o)
		:kind(o.kind),type(o.type),value(o.value),pos(o.pos){}
};

struct DocumentBuilder
{
	struct Dim
	{
		unsigned int nofDocs;
		unsigned int nofTermTypes;
		unsigned int nofTermValues;
		unsigned int nofDiffTermValues;
		unsigned int nofAttributes;
		unsigned int nofMetaData;
	};

	static std::vector<Feature> create( unsigned int docno, const Dim& dim)
	{
		std::vector<Feature> rt;

		unsigned int ti=0, te=dim.nofTermTypes;
		for (ti=0; ti<te; ++ti)
		{
			char searchtype[ 32];
			snprintf( searchtype, sizeof(searchtype), "q%02u", (ti + 10 * docno) % dim.nofTermTypes);
			char forwardtype[ 32];
			snprintf( forwardtype, sizeof(forwardtype), "r%02u", (ti + 10 * docno) % dim.nofTermTypes);

			unsigned int vi=0, ve=dim.nofTermValues;
			for (vi=0; vi<ve; ++vi)
			{
				if ((vi + docno) % 4 == 0) continue;
				char value[ 32];
				snprintf( value, sizeof(value), "s%02u", (vi + 10 * docno) % dim.nofDiffTermValues);
				rt.push_back( Feature( Feature::SearchIndex, searchtype, value, vi+1));

				snprintf( value, sizeof(value), "f%02u", (vi + 10 * docno) % dim.nofDiffTermValues);
				rt.push_back( Feature( Feature::ForwardIndex, forwardtype, value, vi+1));
			}
		}
		unsigned int ai=0, ae=dim.nofAttributes;
		for (ai=0; ai<ae; ++ai)
		{
			char attributename[ 32];
			snprintf( attributename, sizeof(attributename), "A%02u", ai);
			char attributevalue[ 32];
			snprintf( attributevalue, sizeof(attributevalue), "a%02u", ai);

			rt.push_back( Feature( Feature::Attribute, attributename, attributevalue));
		}
		unsigned int mi=0, me=dim.nofMetaData;
		for (mi=0; mi<me; ++mi)
		{
			char metadataname[ 32];
			snprintf( metadataname, sizeof(metadataname), "M%1u", mi);
			char metadatavalue[ 32];
			snprintf( metadatavalue, sizeof(metadatavalue), "%1u", mi * 11);

			rt.push_back( Feature( Feature::MetaData, metadataname, metadatavalue));
		}
		return rt;
	}
};

static void insertDocument( strus::StorageDocumentInterface* doc, const std::vector<Feature>& featurelist)
{
	std::vector<Feature>::const_iterator fi = featurelist.begin(), fe = featurelist.end();
	for (; fi != fe; ++fi)
	{
		switch (fi->kind)
		{
			case Feature::SearchIndex:
				doc->addSearchIndexTerm( fi->type, fi->value, fi->pos);
				break;
			case Feature::ForwardIndex:
				doc->addForwardIndexTerm( fi->type, fi->value, fi->pos);
				break;
			case Feature::Attribute:
				doc->setAttribute( fi->type, fi->value);
				break;
			case Feature::MetaData:
			{
				strus::NumericVariant::UIntType val = atoi(fi->value.c_str());
				doc->setMetaData( fi->type, val);
				break;
			}
		}
	}
	doc->done();
	
}

static void insertCollection( strus::StorageClientInterface* storage, const DocumentBuilder::Dim& dim)
{
	strus::local_ptr<strus::StorageTransactionInterface> transaction( storage->createTransaction());
	unsigned int di=0, de=dim.nofDocs;
	for (; di != de; ++di)
	{
		char docid[ 32];
		snprintf( docid, sizeof(docid), "D%02u", di);
		strus::local_ptr<strus::StorageDocumentInterface>
			doc( transaction->createDocument( docid));
		if (!doc.get()) throw strus::runtime_error("error creating document to insert");

		std::vector<Feature> feats = DocumentBuilder::create( di, dim);
		insertDocument( doc.get(), feats);
	}
	if (!transaction->commit() || g_errorhnd->hasError())
	{
		throw strus::runtime_error( "transaction failed: %s", g_errorhnd->fetchError());
	}
}

typedef std::pair<std::string,std::string> DfMapKey;
typedef std::map<DfMapKey,strus::Index> DfMap;

static void calculateDocumentDfMap( DfMap& dfmap, const DocumentBuilder::Dim& dim, unsigned int docno)
{
	std::set<DfMapKey> visited;
	std::vector<Feature> feats = DocumentBuilder::create( docno, dim);
	std::vector<Feature>::const_iterator fi = feats.begin(), fe = feats.end();
	for (; fi != fe; ++fi)
	{
		if (fi->kind == Feature::SearchIndex)
		{
			DfMapKey key = DfMapKey( fi->type, fi->value);
			if (visited.find( key) == visited.end())
			{
				visited.insert( key);
				dfmap[ key] += 1;
			}
		}
	}
}

static DfMap calculateCollectionDfMap( const DocumentBuilder::Dim& dim)
{
	DfMap dfmap;
	unsigned int di=0, de=dim.nofDocs;
	for (; di != de; ++di)
	{
		calculateDocumentDfMap( dfmap, dim, di);
	}
	return dfmap;
}

static void testTrivialInsert()
{
	DocumentBuilder::Dim dim;
	dim.nofDocs = 100;
	dim.nofTermTypes = 10;
	dim.nofTermValues = 100;
	dim.nofDiffTermValues = 50;
	dim.nofAttributes = 3;
	dim.nofMetaData = 3;

	Storage storage;
	storage.open( "path=storage; metadata=M0 UINT32, M1 UINT16, M2 UINT8", true);
	insertCollection( storage.sci.get(), dim);

	unsigned int di=0,de=dim.nofDocs;
	for (; di != de; ++di)
	{
		char docid[ 32];
		snprintf( docid, sizeof(docid), "D%02u", di);
		const char* errlog = "checkindex.log";

		unsigned int ec = strus::writeFile( errlog, "");
		if (ec) throw strus::runtime_error("error opening logfile '%s' (%u)", errlog, ec);

		strus::local_ptr<strus::StorageDocumentInterface>
			doc( storage.sci->createDocumentChecker( docid, errlog));
		if (!doc.get())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}

		std::vector<Feature> feats = DocumentBuilder::create( di, dim);
		insertDocument( doc.get(), feats);

		if (g_errorhnd->hasError())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		std::string errors;
		ec = strus::readFile( errlog, errors);
		if (ec) throw strus::runtime_error("error opening logfile '%s' for reading (%u)", errlog, ec);
		if (errors.size() > 1000)
		{
			errors.resize(1000);
		}
		if (!errors.empty())
		{
			throw strus::runtime_error("error checking insert of %s: %s", docid, errors.c_str());
		}
	}
}

static void testDfCalculation()
{
	DocumentBuilder::Dim dim;
	dim.nofDocs = 100;
	dim.nofTermTypes = 3;
	dim.nofTermValues = 1000;
	dim.nofDiffTermValues = 200;
	dim.nofAttributes = 0;
	dim.nofMetaData = 0;

	Storage storage;
	storage.open( "path=storage; metadata=M0 UINT32, M1 UINT16, M2 UINT8", true);
	insertCollection( storage.sci.get(), dim);

	DfMap dfmap = calculateCollectionDfMap( dim);
	DfMap::const_iterator xi = dfmap.begin(), xe = dfmap.end();
	for (; xi != xe; ++xi)
	{
		strus::Index df = storage.sci->documentFrequency( xi->first.first, xi->first.second);
		std::cout << "GET FEATURE DF " << xi->first.first << " '" << xi->first.second << "' = " << df << std::endl;
		if (df != xi->second) throw strus::runtime_error("df of feature %s '%s' does not match: %d != %d", xi->first.first.c_str(), xi->first.second.c_str(), (int)df, (int)xi->second);
	}
	{
		strus::local_ptr<strus::StorageTransactionInterface> transaction( storage.sci->createTransaction());
		unsigned int di=0, de=dim.nofDocs;
		for (; di < de; di+=2)
		{
			char docid[ 32];
			snprintf( docid, sizeof(docid), "D%02u", di);
	
			transaction->deleteDocument( docid);
			DfMap doc_dfmap;
			calculateDocumentDfMap( doc_dfmap, dim, di);
			
			xi = doc_dfmap.begin(), xe = doc_dfmap.end();
			for (; xi != xe; ++xi)
			{
				DfMap::iterator mi = dfmap.find( DfMapKey( xi->first.first, xi->first.second));
				if (mi == dfmap.end()) throw strus::runtime_error("feature %s '%s' not found in dfmap", xi->first.first.c_str(), xi->first.second.c_str());
				if (mi->second < xi->second) throw strus::runtime_error("df of feature %s '%s' got negative after deletion", xi->first.first.c_str(), xi->first.second.c_str());
				mi->second -= xi->second;
			}
		}
		transaction->commit();
	}
	{
		strus::local_ptr<strus::StorageTransactionInterface> transaction( storage.sci->createTransaction());
		xi = dfmap.begin(), xe = dfmap.end();
		for (; xi != xe; ++xi)
		{
			strus::Index df = storage.sci->documentFrequency( xi->first.first, xi->first.second);
			if (df != xi->second) throw strus::runtime_error("df of feature %s '%s' does not match after deletion of documents: %d != %d", xi->first.first.c_str(), xi->first.second.c_str(), (int)df, (int)xi->second);
		}
		unsigned int di=0, de=dim.nofDocs;
		for (di=1; di < de; di+=2)
		{
			char docid[ 32];
			snprintf( docid, sizeof(docid), "D%02u", di);
	
			transaction->deleteDocument( docid);
		}
		transaction->commit();
	}
	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	xi = dfmap.begin(), xe = dfmap.end();
	for (; xi != xe; ++xi)
	{
		strus::Index df = storage.sci->documentFrequency( xi->first.first, xi->first.second);
		if (df != 0) throw strus::runtime_error("df of feature %s '%s' got not null after deletion of all documents: %d", xi->first.first.c_str(), xi->first.second.c_str(), (int)df);
	}
}


#define RUN_TEST( idx, TestName)\
	try\
	{\
		test ## TestName();\
		std::cerr << "Executing test (" << idx << ") " << #TestName << " [OK]" << std::endl;\
	}\
	catch (const std::runtime_error& err)\
	{\
		std::cerr << "Error in test (" << idx << ") " << #TestName << ": " << err.what() << std::endl;\
		return -1;\
	}\
	catch (const std::bad_alloc& err)\
	{\
		std::cerr << "Out of memory in test (" << idx << ") " << #TestName << std::endl;\
		return -1;\
	}\


int main( int argc, const char* argv[])
{
	bool do_cleanup = true;
	unsigned int ii = 1;
	unsigned int test_index = 0;
	for (; argc > (int)ii; ++ii)
	{
		if (std::strcmp( argv[ii], "-K") == 0)
		{
			do_cleanup = false;
		}
		else if (std::strcmp( argv[ii], "-T") == 0)
		{
			++ii;
			if (argc == (int)ii)
			{
				std::cerr << "option -T expects an argument" << std::endl;
				return -1;
			}
			test_index = atoi( argv[ ii]);
		}
		else if (std::strcmp( argv[ii], "-h") == 0)
		{
			std::cerr << "usage: testStorageOp [options]" << std::endl;
			std::cerr << "options:" << std::endl;
			std::cerr << "  -h      :print usage" << std::endl;
			std::cerr << "  -K      :keep artefacts, do not clean up" << std::endl;
			std::cerr << "  -T <i>  :execute only test with index <i>" << std::endl;
		}
		else if (argv[ii][0] == '-')
		{
			std::cerr << "unknown option " << argv[ii] << std::endl;
			return -1;
		}
		else
		{
			std::cerr << "unexpected argument" << std::endl;
			return -1;
		}
	}
	g_errorhnd = strus::createErrorBuffer_standard( stderr, 1);
	if (!g_errorhnd) return -1;

	unsigned int ti=test_index?test_index:1;
	for (;;++ti)
	{
		switch (ti)
		{
			case 1: RUN_TEST( ti, DfCalculation ) break;
			case 2: RUN_TEST( ti, DeleteNonExistingDoc ) break;
			case 3: RUN_TEST( ti, TermTypeIterator ) break;
			case 4: RUN_TEST( ti, TrivialInsert ) break;
			case 5: RUN_TEST( ti, SimpleDocumentUpdate) break;
			case 6: RUN_TEST( ti, DocumentUpdate) break;
			default: goto TESTS_DONE;
		}
		if (test_index) break;
	}
TESTS_DONE:
	if (do_cleanup)
	{
		destroyStorage( "path=storage");
	}
	return 0;
}


