/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for creating a mapping of floating point vectors of a defined dimension to a list of features in an unsupervised way.
#ifndef _STRUS_VECTOR_SPACE_MODEL_BUILDER_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_BUILDER_INTERFACE_HPP_INCLUDED
#include <vector>

namespace strus {

/// \brief Interface for building a repository of vectors of floating point numbers representing document features and for learning of a model that relates added features to concept features.
/// \remark This interface has the transaction context logically enclosed in the object, though use in a multithreaded context does not make much sense. Thread safety of the interface is guaranteed, but not the performance in a multithreaded context. It is thought as class that internally makes heavily use of multithreading, but is not thought to be fed by mutliple threads.
class VectorSpaceModelBuilderInterface
{
public:
	/// \brief Destructor
	virtual ~VectorSpaceModelBuilderInterface(){}

	/// \brief Add a feature to the model for later retrieval and for learning of the concepts associated to features
	/// \param[in] vec vector to add
	virtual void addFeature( const std::string& name, const std::vector<double>& vec)=0;

	/// \brief Ensure the persistent storage of the features added with addFeature(const std::string&,const std::vector<double>&) till now
	/// \return true on success, false if failed
	virtual bool done()=0;

	/// \brief Execute a command or an unsupervised learning step and store the results persistently for later use.
	/// \param[in] command to expecute. The commands available are dependent on the model and can be introspected with 'commands()const'
	/// \return true on success, false if the command failed
	virtual bool run( const std::string& command)=0;

	/// \brief Get the list of commands available
	/// \return list of commands for calling 'run(const std::string&)'
	virtual std::vector<std::string> commands() const=0;

	/// \brief Get a short description of a command
	/// \return the command description text
	virtual std::string commandDescription( const std::string& command) const=0;
};

}//namespace
#endif



