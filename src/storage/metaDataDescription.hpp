/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_METADATA_DESCRIPTION_HPP_INCLUDED
#define _STRUS_STORAGE_METADATA_DESCRIPTION_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "private/internationalization.hpp"
#include "metaDataElement.hpp"
#include <utility>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;


/// \brief Description of a meta data record structure
class MetaDataDescription
{
public:
	MetaDataDescription();
	MetaDataDescription( const DatabaseClientInterface* database);
	MetaDataDescription( const std::string& str);
	MetaDataDescription( const MetaDataDescription& o);

	MetaDataDescription& operator=( const MetaDataDescription& o);
	bool operator==( const MetaDataDescription& o) const	{return isequal(o);}
	bool operator!=( const MetaDataDescription& o) const	{return !isequal(o);}
	bool isequal( const MetaDataDescription& o) const;

	std::string tostring() const;

	std::size_t nofElements() const
	{
		return m_ar.size();
	}

	std::size_t bytesize() const
	{
		return m_bytesize?(((m_bytesize+3)>>2)<<2):1;	//... aligned to 4 bytes or 1 if empty
	}

	bool defined( const std::string& name_);
	void add( MetaDataElement::Type type_, const std::string& name_);

	const MetaDataElement* get( int handle) const
	{
		if ((std::size_t)handle >= m_ar.size()) throw strus::runtime_error( _TXT( "array bound read in function %s"), __FUNCTION__);
		return &m_ar[ handle];
	}
	const char* getName( int handle) const
	{
		std::map<std::string,std::size_t>::const_iterator ni = m_namemap.begin(), ne = m_namemap.end();
		for (; ni != ne && (int)ni->second != handle; ++ni){}
		return (ni == ne)?0:ni->first.c_str();
	}
	int getHandle( const std::string& name_) const;
	bool hasElement( const std::string& name_) const;

	void load( const DatabaseClientInterface* database);
	void store( DatabaseTransactionInterface* database);
	void storeImm( DatabaseClientInterface* database);

	struct TranslationElement
	{
		const MetaDataElement* dst;
		const MetaDataElement* src;

		TranslationElement( const TranslationElement& o)
			:dst(o.dst),src(o.src){}
		TranslationElement( const MetaDataElement* dst_, const MetaDataElement* src_)
			:dst(dst_),src(src_){}
	};

	typedef std::vector<TranslationElement> TranslationMap;
	TranslationMap getTranslationMap(
			const MetaDataDescription& o,
			const std::vector<std::string>& resets) const;

	std::vector<std::string> columns() const;

	/// \brief Alter the table description by renaming one element
	/// \param[in] oldname old name of the element
	/// \param[in] newname new name of the element
	void renameElement( const std::string& oldname, const std::string& newname);

	/// \brief delete all elements
	void clear();

	class const_iterator
	{
	public:
		explicit const_iterator( const MetaDataDescription* descr_, int handle_)
			:m_descr(descr_),m_handle(handle_){}

		const_iterator& operator++()				{++m_handle; return *this;}
		const_iterator operator++(int)				{const_iterator rt(m_descr,m_handle); ++m_handle; return rt;}

		bool operator==( const const_iterator& o) const		{return m_handle==o.m_handle;}
		bool operator!=( const const_iterator& o) const		{return m_handle!=o.m_handle;}

		const char* name() const				{return m_descr->getName( m_handle);}
		const MetaDataElement& element() const			{return *m_descr->get( m_handle);}

	private:
		const MetaDataDescription* m_descr;
		int m_handle;
	};

	const_iterator begin() const					{return const_iterator( this, 0);}
	const_iterator end() const					{return const_iterator( this, nofElements());}

private:
	std::size_t m_bytesize;					///< sizeof in bytes of the meta data record
	std::vector<MetaDataElement> m_ar;			///< array of elements
	std::map<std::string,std::size_t> m_namemap;		///< map of names to elements (index in m_ar)
};

}//namespace
#endif

