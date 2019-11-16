/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Collection of items represented by a number as id containing all its prime factors as features
/// \file "aclReaderInterface.hpp"
#ifndef _STRUS_CORE_TEST_PRIME_FACTOR_COLLECTION_HPP_INCLUDED
#define _STRUS_CORE_TEST_PRIME_FACTOR_COLLECTION_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/reference.hpp"
#include "strus/base/pseudoRandom.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/shared_ptr.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/storageMetaDataTableUpdateInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/documentTermIteratorInterface.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"
#include "private/internationalization.hpp"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <iostream>

namespace strus {
namespace test {

/// \brief Collection of items represented by a number as id containing all its prime factors as features
class PrimeFactorCollection
{
public:
	enum {MinNumber=2};

	PrimeFactorCollection( const PrimeFactorCollection& o)
		:m_ar(o.m_ar),m_occurrencies(o.m_occurrencies){}
	
	explicit PrimeFactorCollection( int nofNumbers)
		:m_ar( nofNumbers, std::vector<int>())
	{
		int mi = MinNumber, me=nofNumbers+MinNumber;
		for (; mi<me; ++mi)
		{
			std::vector<int>& tfactors = m_ar[ mi-MinNumber];
			if (tfactors.empty())
			{
				tfactors.push_back( mi);
				m_occurrencies[ mi] += 1;
			}
			int bi = MinNumber, be = mi+1;
			for (; bi < be && bi*mi < me; ++bi)
			{
				std::vector<int>& ufactors = m_ar[ (bi*mi)-MinNumber];
				std::vector<int>& bfactors = m_ar[ bi-MinNumber];
				if (ufactors.empty())
				{
					ufactors.insert( ufactors.end(), bfactors.begin(), bfactors.end());
					ufactors.insert( ufactors.end(), tfactors.begin(), tfactors.end());
					std::sort( ufactors.begin(), ufactors.end());
					std::set<int> uniqueset( ufactors.begin(), ufactors.end());
					std::set<int>::const_iterator ui = uniqueset.begin(), ue = uniqueset.end();
					for (; ui != ue; ++ui)
					{
						++m_occurrencies[ *ui];
					}
				}
			}
		}
	}

	int maxNumber() const
	{
		return m_ar.size() + MinNumber;
	}

	const std::vector<int>& factorList( int number) const
	{
		return m_ar[ number-MinNumber];
	}

	std::set<int> factorSet( int number) const
	{
		return std::set<int>( m_ar[ number-MinNumber].begin(), m_ar[ number-MinNumber].end());
	}

	std::map<int,int> factorMap( int number) const
	{
		std::map<int,int> rt;
		std::vector<int>::const_iterator fi = m_ar[ number-MinNumber].begin(), fe = m_ar[ number-MinNumber].end();
		for (; fi != fe; ++fi)
		{
			++rt[ *fi];
		}
		return rt;
	}

	std::map<int,int> queryFeatToFfMap( int number, const std::vector<int>& query) const
	{
		std::map<int,int> rt;
		const std::vector<int>& factors = factorList( number);
		std::vector<int>::const_iterator fi = factors.begin(), fe = factors.end();
		for (; fi != fe; ++fi)
		{
			std::vector<int>::const_iterator qi = query.begin(), qe = query.end();
			for (; qi != qe && *fi != *qi; ++qi){}
			if (qi != qe) ++rt[ *qi];
		}
		return rt;
	}

	int frequency( int number) const
	{
		std::map<int,int>::const_iterator oi = m_occurrencies.find( number);
		return (oi == m_occurrencies.end()) ? 0 : oi->second;
	}

	int size() const
	{
		return m_ar.size();
	}

	int randomPrime( PseudoRandom& random) const
	{
		for (;;)
		{
			int rt = random.get( MinNumber, (int)m_ar.size()+MinNumber);
			if (frequency( rt)) return rt;
		}
	}

	static std::string docid( int docno)
	{
		return strus::string_format( "D%d", docno + MinNumber);
	}

	static int docIndex( const std::string& docid)
	{
		char const* di = docid.c_str();
		if (*di == 'D')
		{
			++di;
			while (*di >= '0' && *di <= '9')
			{
				++di;
			}
			if (*di == '\0')
			{
				return atoi( docid.c_str()+1) - MinNumber;
			}
		}
		throw std::runtime_error( "bad document id");
	}

private:
	std::vector<std::vector<int> > m_ar;
	std::map<int,int> m_occurrencies;
};

class Storage
{
public:
	explicit Storage( const strus::FileLocatorInterface* fileLocator_, strus::ErrorBufferInterface* errorhnd_)
		:fileLocator(fileLocator_),errorhnd(errorhnd_),dbi(),sti(),sci(){}
	Storage( const Storage& o)
		:fileLocator(o.fileLocator),errorhnd(o.errorhnd),dbi(o.dbi),sti(o.sti),sci(o.sci){}
	~Storage(){}

	const strus::FileLocatorInterface* fileLocator;
	strus::ErrorBufferInterface* errorhnd;
	strus::Reference<strus::DatabaseInterface> dbi;
	strus::Reference<strus::StorageInterface> sti;
	strus::Reference<strus::StorageClientInterface> sci;

	void open( const char* config, bool reset)
	{
		dbi.reset( strus::createDatabaseType_leveldb( fileLocator, errorhnd));
		if (!dbi.get())
		{
			throw std::runtime_error( errorhnd->fetchError());
		}
		sti.reset( strus::createStorageType_std( fileLocator, errorhnd));
		if (!sti.get() || errorhnd->hasError())
		{
			throw std::runtime_error( errorhnd->fetchError());
		}
		if (reset)
		{
			(void)dbi->destroyDatabase( config);
			(void)errorhnd->fetchError();
		
			if (!sti->createStorage( config, dbi.get()))
			{
				throw std::runtime_error( errorhnd->fetchError());
			}
		}
		const strus::StatisticsProcessorInterface* statisticsMessageProc = 0;
		sci.reset( sti->createClient( config, dbi.get(), statisticsMessageProc));
		if (!sci.get())
		{
			throw std::runtime_error( errorhnd->fetchError());
		}
	}

	void close()
	{
		sci.reset();
		sti.reset();
		dbi.reset();
	}

	struct MetaDataDef
	{
		const char* key;
		const char* type;
	};

	void defineMetaData( MetaDataDef const* deflist)
	{
		strus::Reference<strus::StorageTransactionInterface> transaction( sci->createTransaction());
		if (!transaction.get()) throw strus::runtime_error( "failed to create transaction: %s", errorhnd->fetchError());
		strus::Reference<strus::StorageMetaDataTableUpdateInterface> update( transaction->createMetaDataTableUpdate());
		if (!update.get()) throw strus::runtime_error( "failed to create structure for declaring meta data: %s", errorhnd->fetchError());
		
		int di = 0;
		for (; deflist[di].key; ++di)
		{
			update->addElement( deflist[di].key, deflist[di].type);
		}
		update->done();
		if (!transaction->commit()) throw strus::runtime_error( "failed to commit meta data structure definition: %s", errorhnd->fetchError());
	}

	std::string dump()
	{
		strus::local_ptr<strus::StorageDumpInterface> chunkitr( sci->createDump( ""/*keyprefix*/));
	
		const char* chunk;
		std::size_t chunksize;
		std::string dumpcontent;
		while (chunkitr->nextChunk( chunk, chunksize))
		{
			dumpcontent.append( chunk, chunksize);
		}
		if (errorhnd->hasError())
		{
			throw std::runtime_error( errorhnd->fetchError());
		}
		return dumpcontent;
	}

	static void destroy( const char* config, const strus::FileLocatorInterface* fileLocator_, strus::ErrorBufferInterface* errorhnd_)
	{
		strus::shared_ptr<strus::DatabaseInterface> dbi;
		dbi.reset( strus::createDatabaseType_leveldb( fileLocator_, errorhnd_));
		if (!dbi.get())
		{
			throw std::runtime_error( errorhnd_->fetchError());
		}
		dbi->destroyDatabase( config);
	}
};


struct Feature
{
	enum Kind
	{
		SearchIndex,
		ForwardIndex,
		Attribute,
		MetaData
	};
	static const char* kindName( Kind kind)
	{
		static const char* ar[] = {"searchindex","forwardindex","attribute","metadata"};
		return ar[ kind];
	}
	Kind kind;
	std::string type;
	std::string value;
	strus::Index pos;

	Feature()
		:kind(Attribute),type(),value(),pos(0){}
	Feature( Kind kind_, const std::string& type_, const std::string& value_, strus::Index pos_=0)
		:kind(kind_),type(type_),value(value_),pos(pos_){}
	Feature( const Feature& o)
		:kind(o.kind),type(o.type),value(o.value),pos(o.pos){}
	Feature& operator=( const Feature& o)
		{kind=o.kind;type=o.type;value=o.value;pos=o.pos; return *this;}

	bool operator <( const Feature& o) const	{return compare(o) < 0;}
	bool operator >( const Feature& o) const	{return compare(o) > 0;}
	bool operator <=( const Feature& o) const	{return compare(o) <= 0;}
	bool operator >=( const Feature& o) const	{return compare(o) >= 0;}
	bool operator ==( const Feature& o) const	{return compare(o) == 0;}
	bool operator !=( const Feature& o) const	{return compare(o) != 0;}

private:
	int compare( const Feature& o) const
	{
		if (kind != o.kind) return (int)kind - (int)o.kind;
		if (pos != o.pos) return (int)pos - (int)o.pos;
		if (type != o.type) return std::strcmp( type.c_str(), o.type.c_str());
		if (value != o.value) return std::strcmp( value.c_str(), o.value.c_str());
		return 0;
	}
};


struct PrimeFactorDocumentBuilder
{
	struct ObservedTerm
	{
		std::string docid;
		std::string value;

		ObservedTerm( const std::string& docid_, const std::string& value_)
			:docid(docid_),value(value_){}
		ObservedTerm( const ObservedTerm& o)
			:docid(o.docid),value(o.value){}
	};

	strus::ErrorBufferInterface* errorhnd;
	PrimeFactorCollection primeFactorCollection;
	int nofDocuments;
	bool verbose;
	std::vector<ObservedTerm> observedTermList;

	PrimeFactorDocumentBuilder( int nofDocuments_, bool verbose_, strus::ErrorBufferInterface* errorhnd_)
		:errorhnd(errorhnd_)
		,primeFactorCollection( nofDocuments_)
		,nofDocuments( nofDocuments_)
		,verbose(verbose_)
	{}

	static const Storage::MetaDataDef* metadata()
	{
		static const Storage::MetaDataDef rt[] = {{"doclen", "UINT8"},{"lo", "UINT16"},{"hi", "UINT16"},{"nn", "UINT8"},{0,0}};
		return rt;
	}

	static const char* searchIndexFeatureType()		{return "word";}
	std::string searchIndexFeatureValue( int fnum) const	{return strus::string_format( "s%u", fnum);}
	static const char* forwardIndexFeatureType()		{return "orig";}
	std::string forwardIndexFeatureValue( int fnum) const	{return strus::string_format( "f%u", fnum);}

	std::vector<Feature> createDocument( int didx) const
	{
		std::vector<Feature> rt;
		int number = didx + PrimeFactorCollection::MinNumber;
		std::string description;
		std::string title = strus::string_format( "number %d", number);

		const std::vector<int>& fc = primeFactorCollection.factorList( number);
		std::set<int> uc;
		int maxnum = 0;
		int minnum = 0;
		unsigned int ti=0, te=fc.size();
		for (ti=0; ti<te; ++ti)
		{
			uc.insert( fc[ti]);
			if (!minnum || minnum > fc[ti])
			{
				minnum = fc[ti];
			}
			if (maxnum < fc[ti])
			{
				maxnum = fc[ti];
			}
			if (ti) description.push_back( ' ');
			description.append( strus::string_format( "%u", fc[ti]));

			std::string searchtype = searchIndexFeatureType();
			std::string searchvalue = searchIndexFeatureValue( fc[ti]);
			rt.push_back( Feature( Feature::SearchIndex, searchtype, searchvalue, ti+1));

			std::string forwardtype = forwardIndexFeatureType();
			std::string forwardvalue = forwardIndexFeatureValue( fc[ti]);
			rt.push_back( Feature( Feature::ForwardIndex, forwardtype, forwardvalue, ti+1));
		}
		rt.push_back( Feature( Feature::Attribute, "title", title));
		rt.push_back( Feature( Feature::Attribute, "docid", PrimeFactorCollection::docid( didx)));
		rt.push_back( Feature( Feature::Attribute, "description", description));

		std::string sizestr = strus::string_format( "%u", (unsigned int)fc.size());
		std::string lostr = strus::string_format( "%u", fc.empty() ? 0:minnum);
		std::string histr = strus::string_format( "%u", fc.empty() ? 0:maxnum);
		std::string nnstr = strus::string_format( "%u", (unsigned int)uc.size());

		rt.push_back( Feature( Feature::MetaData, "doclen", sizestr));
		rt.push_back( Feature( Feature::MetaData, "lo", lostr));
		rt.push_back( Feature( Feature::MetaData, "hi", histr));
		rt.push_back( Feature( Feature::MetaData, "nn", nnstr));

		return rt;
	}

	static void printDocument( std::ostream& out, const std::string& docid, const std::vector<Feature>& featurelist)
	{
		out << strus::string_format( "docid %s\n", docid.c_str());
		std::vector<Feature>::const_iterator fi = featurelist.begin(), fe = featurelist.end();
		for (; fi != fe; ++fi)
		{
			if (fi->pos)
			{
				out << strus::string_format( "\tpos %d: %s %s '%s'\n", fi->pos, Feature::kindName( fi->kind), fi->type.c_str(), fi->value.c_str());
			}
			else
			{
				out << strus::string_format( "\t%s %s '%s'\n", Feature::kindName( fi->kind), fi->type.c_str(), fi->value.c_str());
			}
		}
		out << std::endl; 
	}

	template <class DocumentInterface>
	static void buildDocument( DocumentInterface* doc, const std::vector<Feature>& featurelist)
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

	std::vector<std::string> docids() const
	{
		std::vector<std::string> rt;
		unsigned int di=0, de=nofDocuments;
		for (; di != de; ++di)
		{
			rt.push_back( PrimeFactorCollection::docid( di));
		}
		return rt;
	}

	Feature randomFeature( const Feature& oldfeat, PseudoRandom& random)
	{
		int fidx = primeFactorCollection.randomPrime( random);
		switch (oldfeat.kind)
		{
			case Feature::SearchIndex:
				return Feature(
					Feature::SearchIndex, searchIndexFeatureType(),
					searchIndexFeatureValue( fidx), oldfeat.pos);
			case Feature::ForwardIndex:
				return Feature(
					Feature::ForwardIndex, forwardIndexFeatureType(),
					forwardIndexFeatureValue( fidx), oldfeat.pos);
			case Feature::Attribute:
				return Feature(
					Feature::Attribute, oldfeat.type,
					strus::string_format( "random %d", fidx), 0/*no pos*/);
			case Feature::MetaData:
				return Feature(
					Feature::MetaData, oldfeat.type,
					strus::string_format( "%d", fidx), 0/*no pos*/);
		}
		throw std::runtime_error("unknown feature class");
	}

	std::vector<Feature> randomAlterFeatures( const std::vector<Feature>& features, PseudoRandom& random)
	{
		std::vector<Feature> rt = features;
		int nofChanges = random.get( 1, random.get( 1, features.size()+2) +1);
		int ii=0, ie=nofChanges;
		for (; ii<ie; ++ii)
		{
			enum ChangeType {InsertFeat,AlterFeat,DeleteFeat};
			ChangeType changeType = (ChangeType)random.get( 0, 3);
			std::size_t chgidx = random.get( 0, rt.size());

			switch (changeType)
			{
				case InsertFeat:
					rt.insert( rt.begin()+chgidx, randomFeature( rt[ chgidx], random));
					break;
				case AlterFeat:
					rt[ chgidx] = randomFeature( rt[ chgidx], random);
					break;
				case DeleteFeat:
					rt.erase( rt.begin()+chgidx);
					break;
			}
		}
		return rt;
	}

	enum WriteMode {InsertMode, UpdateMode, InsertAlteredMode, UpdateAlteredMode};
	void insertCollection( strus::StorageClientInterface* storage, PseudoRandom& random, int commitSize, WriteMode mode=InsertMode)
	{
		strus::local_ptr<strus::StorageTransactionInterface> transaction( storage->createTransaction());
		unsigned int di=0, de=nofDocuments;
		for (; di != de; ++di)
		{
			if (commitSize > 0 && di > 0 && di % commitSize == 0)
			{
				if (errorhnd->hasError())
				{
					throw strus::runtime_error( "insert failed: %s", errorhnd->fetchError());
				}
				if (!transaction->commit())
				{
					throw strus::runtime_error( "transaction failed: %s", errorhnd->fetchError());
				}
				listObservedTerms( std::cerr, storage);
			}
			std::string docid = PrimeFactorCollection::docid( di);
			std::vector<Feature> feats = createDocument( di);
			switch (mode)
			{
				case InsertAlteredMode:
					feats = randomAlterFeatures( feats, random);
					/*no break here!*/
				case InsertMode:
				{
					strus::local_ptr<strus::StorageDocumentInterface>
						doc( transaction->createDocument( docid.c_str()));
					if (!doc.get()) throw strus::runtime_error("error creating document to insert");
					buildDocument( doc.get(), feats);
					break;
				}
				case UpdateAlteredMode:
					feats = randomAlterFeatures( feats, random);
					/*no break here!*/
				case UpdateMode:
				{
					strus::Index docno = storage->documentNumber( docid);
					if (!docno) throw strus::runtime_error("document '%s' to update does not exist", docid.c_str());
					strus::local_ptr<strus::StorageDocumentUpdateInterface>
						doc( transaction->createDocumentUpdate( docno));
					if (!doc.get()) throw strus::runtime_error("error creating document to insert");
					buildDocument( doc.get(), feats);
					break;
				}
			}
			if (verbose) printDocument( std::cerr, docid, feats);
		}
		if (errorhnd->hasError())
		{
			throw strus::runtime_error( "insert failed: %s", errorhnd->fetchError());
		}
		if (!transaction->commit())
		{
			throw strus::runtime_error( "transaction failed: %s", errorhnd->fetchError());
		}
		listObservedTerms( std::cerr, storage);
	}

	void checkAgainstContentTerms( const strus::StorageClientInterface* storage, int didx, const std::vector<Feature>& features)
	{
		// Check search index terms against document term inv:
		std::string docid = primeFactorCollection.docid( didx);
		strus::Index docno = storage->documentNumber( docid);

		strus::Reference<strus::DocumentTermIteratorInterface>
			titr( storage->createDocumentTermIterator( searchIndexFeatureType()));
		if (!titr.get()) throw std::runtime_error( errorhnd->fetchError());

		titr->skipDoc( docno);
		int nofSearchIndexFeatures = 0;
		std::vector<Feature>::const_iterator ri = features.begin(), re = features.end();
		for (; ri != re; ++ri)
		{
			if (ri->type == searchIndexFeatureType()) ++nofSearchIndexFeatures;
		}
		int nofTermOccurrencies = 0;
		strus::DocumentTermIteratorInterface::Term term;
		while (titr->nextTerm( term))
		{
			nofTermOccurrencies += term.tf;
			int cnt = 0;
			strus::Index firstpos = 0;
			std::string termval = titr->termValue( term.termno);
			ri = features.begin(), re = features.end();
			for (; ri != re; ++ri)
			{
				if (ri->type == searchIndexFeatureType() && ri->value == termval)
				{
					if (!firstpos || firstpos > ri->pos) firstpos = ri->pos;
					++cnt;
				}
			}
			if (firstpos != (int)term.firstpos)
			{
				throw std::runtime_error("first term position does not match");
			}
			if (cnt != (int)term.tf)
			{
				throw std::runtime_error("term occurrencies do not match");
			}
		}
		if (nofTermOccurrencies != nofSearchIndexFeatures)
		{
			throw std::runtime_error("number of term occurrencies do not match");
		}
	}

	std::vector<Feature> fetchContent( const strus::StorageClientInterface* storage, int didx)
	{
		std::vector<Feature> rt;
		std::string docid = primeFactorCollection.docid( didx);
		strus::Index docno = storage->documentNumber( docid);

		int fi = PrimeFactorCollection::MinNumber, fe = nofDocuments + PrimeFactorCollection::MinNumber;
		for (; fi != fe; ++fi,++didx)
		{
			if (primeFactorCollection.frequency( fi))
			{
				// Search index terms:
				std::string type = searchIndexFeatureType();
				std::string value = searchIndexFeatureValue( fi);
				strus::Reference<strus::PostingIteratorInterface>
					pitr( storage->createTermPostingIterator( type, value, 1/*length*/));
				if (!pitr.get()) throw std::runtime_error( errorhnd->fetchError());

				strus::Index dn = pitr->skipDoc( docno);
				int pos;
				if (dn == (strus::Index)docno)
				{
					pos = 0;
					while (0!=(pos = pitr->skipPos( pos+1)))
					{
						rt.push_back( Feature(
							Feature::SearchIndex, type, value, pos));
					}
				}
			}
		}{
			// Forward index terms:
			std::string type = forwardIndexFeatureType();
			strus::Reference<strus::ForwardIteratorInterface>
				fitr( storage->createForwardIterator( type));
			if (!fitr.get()) throw std::runtime_error( errorhnd->fetchError());
	
			fitr->skipDoc( docno);
			strus::Index pos = 0;
			while (0!=(pos = fitr->skipPos( pos+1)))
			{
				rt.push_back( Feature(
					Feature::ForwardIndex, type, fitr->fetch(), pos));
			}
		}{
			// Attributes:
			strus::Reference<strus::AttributeReaderInterface>
				aitr( storage->createAttributeReader());
			if (!aitr.get()) throw std::runtime_error( errorhnd->fetchError());
	
			aitr->skipDoc( docno);
			std::vector<std::string> anames = aitr->getNames();
			std::vector<std::string>::const_iterator ai = anames.begin(), ae = anames.end();
			for (; ai != ae; ++ai)
			{
				strus::Index eh = aitr->elementHandle( *ai);
				std::string aval = aitr->getValue( eh);
				if (!aval.empty())
				{
					rt.push_back( Feature( Feature::Attribute, *ai, aval, 0));
				}
			}
		}{
			// Meta data:
			strus::Reference<strus::MetaDataReaderInterface>
				mitr( storage->createMetaDataReader());
			if (!mitr.get()) throw std::runtime_error( errorhnd->fetchError());
	
			mitr->skipDoc( docno);
			std::vector<std::string> mnames = mitr->getNames();
			std::vector<std::string>::const_iterator mi = mnames.begin(), me = mnames.end();
			for (; mi != me; ++mi)
			{
				strus::Index eh = mitr->elementHandle( *mi);
				std::string mval = mitr->getValue( eh).tostring().c_str();
				if (!mval.empty())
				{
					rt.push_back( Feature( Feature::MetaData, *mi, mval, 0));
				}
			}
		}
		return rt;
	}

	static std::string getFeatureDescription( const Feature& feature, const strus::StorageClientInterface* storage)
	{
		std::string rt( feature.kindName( feature.kind));
		switch (feature.kind)
		{
			case Feature::SearchIndex:
				rt.append( strus::string_format(
					", type='%s' (%d), value='%s' (%d), pos=%d",
					feature.type.c_str(),
					(int)storage->termTypeNumber( feature.type),
					feature.value.c_str(),
					(int)storage->termValueNumber( feature.value),
					(int)feature.pos));
				break;
			case Feature::ForwardIndex:
				rt.append( strus::string_format(
					", type='%s' (%d), value='%s', pos=%d",
					feature.type.c_str(),
					(int)storage->termTypeNumber( feature.type),
					feature.value.c_str(),
					(int)feature.pos));
				break;
			case Feature::MetaData:
			case Feature::Attribute:
				rt.append( strus::string_format(
					", name='%s', value='%s'",
					feature.type.c_str(),
					feature.value.c_str()));
				break;
		}
		return rt;
	}

	void checkInsertedDocuments( std::ostream& out, const strus::StorageClientInterface* storage, int nofDocumentsToCheck)
	{
		int di = 0, de = nofDocumentsToCheck;
		for (; di != de; ++di)
		{
			std::string docid = PrimeFactorCollection::docid( di);
			std::vector<Feature> inserted = fetchContent( storage, di);
			std::vector<Feature> expected = createDocument( di);
	
			std::sort( inserted.begin(), inserted.end());
			std::sort( expected.begin(), expected.end());
	
			if (verbose) std::cerr << "* checking document " << docid << std::endl;
			bool success = (expected == inserted);
			if (!success || verbose)
			{
				out << "RESULT:" << std::endl;
				printDocument( out, docid, inserted);
				out << std::endl;
				out << "EXPECTED:" << std::endl;
				printDocument( out, docid, expected);
				out << std::endl;
			}
			if (!success)
			{
				out << "RESULT INFO:" << std::endl;
				out << "docid " << docid << ", docno " << storage->documentNumber( docid) << std::endl;
				std::vector<Feature>::const_iterator ri = inserted.begin(), re = inserted.end();
				std::vector<Feature>::const_iterator ei = expected.begin(), ee = expected.end();
				for (; ri != re && ei != ee && *ri == *ei; ++ri,++ei){}
				if (ei != ee)
				{
					if (ri != re)
					{
						if (*ei > *ri)
						{
							for (; ri != re && *ei > *ri; ++ri)
							{
								out << "* unexpected " << getFeatureDescription( *ri, storage) << std::endl;
							}
						}
						else
						{
							for (; ei != ee && *ei < *ri; ++ei)
							{
								out << "* missing " << getFeatureDescription( *ei, storage) << std::endl;
							}
						}
					}
					else
					{
						for (; ei != ee; ++ei)
						{
							out << "* missing " << getFeatureDescription( *ei, storage) << std::endl;
						}
					}
				}
				else
				{
					for (; ri != re; ++ri)
					{
						out << "* unexpected " << getFeatureDescription( *ri, storage) << std::endl;
					}
				}
				throw strus::runtime_error("inserted document %s not as expected", docid.c_str());
			}
			checkAgainstContentTerms( storage, di, inserted);
		}
	}

	void listObservedTerms( std::ostream& out, const strus::StorageClientInterface* storage)
	{
		std::vector<ObservedTerm> ::const_iterator oi = observedTermList.begin(), oe = observedTermList.end();
		for (; oi != oe; ++oi)
		{
			strus::Index docno = storage->documentNumber( oi->docid);

			// Search index terms:
			std::string type = searchIndexFeatureType();
			strus::Index valueno = storage->termValueNumber( oi->value);

			strus::Reference<strus::PostingIteratorInterface>
				pitr( storage->createTermPostingIterator( type, oi->value, 1/*length*/));
			if (!pitr.get()) throw std::runtime_error( errorhnd->fetchError());

			std::string occurrencies;
			strus::Index dn = pitr->skipDoc( docno);
			int pos;
			if (dn == (strus::Index)docno)
			{
				pos = 0;
				while (0!=(pos = pitr->skipPos( pos+1)))
				{
					occurrencies.append( strus::string_format(" %d", (int)pos));
				}
			}
			if (occurrencies.empty())
			{
				out << strus::string_format( "observed term %s (%d) in document %s (%d) not found\n", oi->value.c_str(), valueno, oi->docid.c_str(), docno);
			}
			else
			{
				out << strus::string_format( "observed term %s (%d) in document %s (%d) at %s\n", oi->value.c_str(), (int)valueno, oi->docid.c_str(), docno, occurrencies.c_str());
			}
		}
	}

	void addObservedTerm( const std::string& docid, const std::string& value)
	{
		observedTermList.push_back( ObservedTerm( docid, value));
	}
};

}}//namespace
#endif



