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
/// \brief Macros, classes and functions supporting error handling
/// \file errorUtils.hpp
#ifndef _STRUS_STORAGE_ERROR_UTILITIES_HPP_INCLUDED
#define _STRUS_STORAGE_ERROR_UTILITIES_HPP_INCLUDED
#include <stdexcept>
#include "private/internationalization.hpp"

/// \brief strus toplevel namespace
namespace strus
{

#define CATCH_ERROR_MAP( contextExplainText, errorBuffer)\
	catch (const std::bad_alloc&)\
	{\
		(errorBuffer).report( _TXT("memory allocation error"));\
	}\
	catch (const std::runtime_error& err)\
	{\
		(errorBuffer).report( contextExplainText, err.what());\
	}\
	catch (const std::exception& err)\
	{\
		(errorBuffer).report( _TXT("uncaught exception: %s"), err.what());\
	}

#define CATCH_ERROR_MAP_RETURN( contextExplainText, errorBuffer, errorReturnValue)\
	catch (const std::bad_alloc&)\
	{\
		(errorBuffer).report( _TXT("memory allocation error"));\
		return errorReturnValue;\
	}\
	catch (const std::runtime_error& err)\
	{\
		(errorBuffer).report( contextExplainText, err.what());\
		return errorReturnValue;\
	}\
	catch (const std::exception& err)\
	{\
		(errorBuffer).report( _TXT("uncaught exception: %s"), err.what());\
		return errorReturnValue;\
	}

}//namespace
#endif
