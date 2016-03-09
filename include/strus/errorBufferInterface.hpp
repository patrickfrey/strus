/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Interface for reporting and catching errors in the core (storage)
/// \file errorBufferInterface.hpp
#ifndef _STRUS_STORAGE_ERROR_BUFFER_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_ERROR_BUFFER_INTERFACE_HPP_INCLUDED
#include <cstdio>

/// \brief strus toplevel namespace
namespace strus
{

/// \class ErrorBufferInterface
/// \brief Interface for reporting and catching errors in the core (storage)
class ErrorBufferInterface
{
public:
	enum ErrorClass
	{
		None,		///< no error
		RuntimeError,	///< runtime error
		BadAlloc	///< memory allocation error
	};

	/// \brief Destructor
	virtual ~ErrorBufferInterface(){}

	/// \brief Define error log file
	/// \param[in] hnd file handle to set as log file
	virtual void setLogFile( FILE* hnd)=0;

	/// \brief Redefine the maximum number of threads using the error buffer
	/// \param[in] maxNofThreads number of threads
	virtual bool setMaxNofThreads( unsigned int maxNofThreads)=0;

	/// \brief Report an error
	/// \param[in] format error message format string
	/// \remark must not throw
	virtual void report( const char* format, ...) const=0;

	/// \brief Report an error, overwriting the previous error
	/// \param[in] format error message format string
	/// \remark must not throw
	virtual void explain( const char* format) const=0;

	/// \brief Check, if an error has occurred and return it
	/// \return an error string, if defined, NULL else
	/// \remark resets the error
	virtual const char* fetchError()=0;

	/// \brief Check, if an error has occurred
	/// \return an error string, if defined, NULL else
	virtual bool hasError() const=0;

	/// \brief Allocate context for current thread
	/// \remark allocContext & releaseContext should only be called if assignement of thread ids is not fix
	virtual void allocContext()=0;

	/// \brief Dellocate context for current thread
	/// \remark allocContext & releaseContext should only be called if assignement of thread ids is not fix
	virtual void releaseContext()=0;
};

}//namespace
#endif

