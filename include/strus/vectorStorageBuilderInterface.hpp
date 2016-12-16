/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for building a repository of vectors with classifiers to map them to discrete features.
#ifndef _STRUS_VECTOR_STORGE_BUILDER_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_STORGE_BUILDER_INTERFACE_HPP_INCLUDED
#include <vector>

namespace strus {

/// \brief Interface for building a repository of vectors with classifiers to map them to discrete features.
/// \remark This interface has the transaction context logically enclosed in the object, though use in a multithreaded context does not make much sense. Thread safety of the interface is guaranteed, but not the performance in a multithreaded context. It is thought as class that internally makes heavily use of multithreading, but is not thought to be fed by mutliple threads.
class VectorStorageBuilderInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageBuilderInterface(){}

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
};

}//namespace
#endif



