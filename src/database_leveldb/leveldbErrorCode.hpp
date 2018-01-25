/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_LEVELDB_ERRORCODES_HPP_INCLUDED
#define _STRUS_LEVELDB_ERRORCODES_HPP_INCLUDED
#include "strus/errorCodes.hpp"
#include <leveldb/db.h>

static strus::ErrorCode leveldbErrorCode( const strus::ErrorOperation& errop, const leveldb::Status& status)
{
	strus::ErrorCause cause = strus::ErrorCauseUnknown;
	if (status.IsNotFound())
	{
		cause = strus::ErrorCauseNotFound;
	}
	else if (status.IsCorruption())
	{
		cause = strus::ErrorCauseDataCorruption;
	}
	else if (status.IsIOError())
	{
		cause = strus::ErrorCauseIOError;
	}
	return strus::ErrorCode(strus::StrusComponentCore,errop,cause);
}
#endif

