/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a metadata restriction instance
#ifndef _STRUS_METADATA_RESTRICTION_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_METADATA_RESTRICTION_INSTANCE_INTERFACE_HPP_INCLUDED
#include "strus/arithmeticVariant.hpp"
#include "strus/index.hpp"
#include <string>

namespace strus {

/// \brief Class for building up a metadata restriction
class MetaDataRestrictionInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~MetaDataRestrictionInstanceInterface(){}

	/// \brief Evaluate if a document matches the restriction condition
	/// \param[in] docno local internal document number of the document to match
	/// \return true, if it matches, false if not
	/// \remark A deleted document has every metadata element nulled out. So it depends on the restriction expression wheter the document number matches or not. There esists no other flag for the document number in the system telling wheter it exists or not. 
	virtual bool match( const Index& docno) const=0;
};

} //namespace
#endif

