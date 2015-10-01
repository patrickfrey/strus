/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Standard implemenation of interface for reporting and catching errors
/// \file errorBuffer.hpp
#ifndef _STRUS_ERROR_BUFFER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_ERROR_BUFFER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/errorBufferInterface.hpp"
#include "private/utils.hpp"
#include <cstdio>

/// \brief strus toplevel namespace
namespace strus
{

/// \class ProcessErrorBuffer
/// \brief Error buffer context for one thread
class ProcessErrorBuffer
{
public:
	ProcessErrorBuffer();
	~ProcessErrorBuffer(){}

	void report( FILE* logfilehandle, const char* format, va_list arg) const;
	void explain( FILE* logfilehandle, const char* format) const;

	const char* fetchError()
	{
		if (!m_hasmsg) return 0;
		m_hasmsg = false;
		return m_msgbuf;
	}

	bool hasError() const
	{
		return m_hasmsg;
	}

private:
	enum {ObjSize=512};
	enum {MsgBufSize=(ObjSize-sizeof(bool))};
	mutable char m_msgbuf[ MsgBufSize];
	mutable bool m_hasmsg;
};


/// \class ErrorBuffer
class ErrorBuffer
	:public ErrorBufferInterface
{
public:
	ErrorBuffer( FILE* logfilehandle_, std::size_t maxNofThreads_);
	virtual ~ErrorBuffer();

	virtual void report( const char* format, ...) const;

	virtual void explain( const char* format) const;

	virtual const char* fetchError();

	virtual bool hasError() const;

	virtual void allocContext();
	virtual void releaseContext();

private:
	std::size_t threadidx() const;

private:
	mutable FILE* m_logfilehandle;
	std::size_t m_size;
	struct Slot
	{
		typedef utils::ThreadId::Type Id;
		typedef utils::AtomicFlag Flag;

		Slot(){}
		~Slot(){}

		Id id;
		Flag flag;
	};

	mutable Slot* m_slots;
	mutable ProcessErrorBuffer* m_ar;
};

}//namespace
#endif

