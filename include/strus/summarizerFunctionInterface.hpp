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
/// \brief Interface for a summarizer function type
/// \file summarizerFunctionInterface.hpp
#ifndef _STRUS_SUMMARIZER_FUNCTION_INTERFACE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_FUNCTION_INTERFACE_HPP_INCLUDED
#include <string>
#include <vector>

namespace strus
{
/// \brief Forward declaration
class SummarizerFunctionInstanceInterface;
/// \brief Forward declaration
class QueryProcessorInterface;


/// \brief Interface for summarization functions (additional info about the matches in the result ranklist of a retrieval query)
class SummarizerFunctionInterface
{
public:
	/// \brief Destructor
	virtual ~SummarizerFunctionInterface(){}

	/// \brief Create an instance of this summarization function for parametrization
	/// \param[in] processor provider for query processing functions
	/// \return the created summarization function instance (ownership to caller)
	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const=0;

	/// \brief Structure that describes the summarizer function (for introspection)
	struct Description
	{
		/// \brief Structure that describes a parameter
		struct Param
		{
			enum Type
			{
				Feature,
				Attribute,
				Metadata,
				Numeric,
				String
			};
			/// \brief Constructor
			Param( Type type_, const std::string& name_, const std::string& text_)
				:m_type(type_),m_name(name_),m_text(text_){}
			/// \brief Copy constructor
			Param( const Param& o)
				:m_type(o.m_type),m_name(o.m_name),m_text(o.m_text){}

			///< Get the type of the parameter
			Type type() const			{return m_type;}
			///< Get the name of the parameter
			const std::string& name() const		{return m_name;}
			///< Get the description text of the parameter
			const std::string& text() const		{return m_text;}

		private:
			Type m_type;			///< type of parameter
			std::string m_name;		///< name of parameter
			std::string m_text;		///< description of parameter
		};

		/// \brief Add a parameter description
		Description& operator()( Param::Type type_, const std::string& name_, const std::string& text_)
		{
			m_param.push_back( Param( type_, name_, text_));
			return *this;
		}

		/// \brief Default constructor
		Description(){}

		/// \brief Constructor
		explicit Description( const std::string& text_)
			:m_text(text_){}

		/// \brief Copy constructor
		Description( const Description& o)
			:m_text(o.m_text),m_param(o.m_param){}

		/// \brief Get the description text
		const std::string& text() const			{return m_text;}
		/// \brief Get the description parameter list
		const std::vector<Param>& param() const		{return m_param;}

	private:
		std::string m_text;		///< description text
		std::vector<Param> m_param;	///< list of parameter descriptions
	};

	/// \brief Get a description of the function for user help and introspection
	/// \return the description structure
	virtual Description getDescription() const=0;
};

}//namespace
#endif


