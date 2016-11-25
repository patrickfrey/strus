/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
	catch (const std::logic_error& err)\
	{\
		(errorBuffer).report( _TXT("logic error in strus core: %s"), err.what());\
	}\
	catch (const std::exception& err)\
	{\
		(errorBuffer).report( _TXT("uncaught exception: %s"), err.what());\
	}

#define CATCH_ERROR_ARG1_MAP( contextExplainText, ARG, errorBuffer)\
	catch (const std::bad_alloc&)\
	{\
		(errorBuffer).report( _TXT("memory allocation error"));\
	}\
	catch (const std::runtime_error& err)\
	{\
		(errorBuffer).report( contextExplainText, ARG, err.what());\
	}\
	catch (const std::logic_error& err)\
	{\
		(errorBuffer).report( _TXT("logic error in strus core: %s"), err.what());\
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
	catch (const std::logic_error& err)\
	{\
		(errorBuffer).report( _TXT("logic error in strus core: %s"), err.what());\
		return errorReturnValue;\
	}\
	catch (const std::exception& err)\
	{\
		(errorBuffer).report( _TXT("uncaught exception: %s"), err.what());\
		return errorReturnValue;\
	}

#define CATCH_ERROR_ARG1_MAP_RETURN( contextExplainText, ARG, errorBuffer, errorReturnValue)\
	catch (const std::bad_alloc&)\
	{\
		(errorBuffer).report( _TXT("memory allocation error"));\
		return errorReturnValue;\
	}\
	catch (const std::runtime_error& err)\
	{\
		(errorBuffer).report( contextExplainText, ARG, err.what());\
		return errorReturnValue;\
	}\
	catch (const std::logic_error& err)\
	{\
		(errorBuffer).report( _TXT("logic error in strus core: %s"), err.what());\
		return errorReturnValue;\
	}\
	catch (const std::exception& err)\
	{\
		(errorBuffer).report( _TXT("uncaught exception: %s"), err.what());\
		return errorReturnValue;\
	}

}//namespace
#endif
