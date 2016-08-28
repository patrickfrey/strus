/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the vector space model interface
#ifndef _STRUS_VECTOR_SPACE_MODEL_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_IMPLEMENTATION_HPP_INCLUDED
#include "strus/vectorSpaceModelInterface.hpp"

namespace strus {

/// \brief Forward declaration
class VectorSpaceModelInstanceInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Interface to the create,destroy the key value store database
class VectorSpaceModel
	:public VectorSpaceModelInterface
{
public:
	virtual ~VectorSpaceModel(){}

	explicit VectorSpaceModel( ErrorBufferInterface* errorhnd_);
	virtual VectorSpaceModelInstanceInterface* createModel() const;

private:
	ErrorBufferInterface* m_errorhnd;	///< buffer for reporting errors
};


