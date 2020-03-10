/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ffIterator.hpp"
#include "storageClient.hpp"
#include "private/internationalization.hpp"

using namespace strus;

GlobalCounter FfIterator::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = m_storage->documentFrequency( m_termtypeno, m_termvalueno);
	}
	return m_documentFrequency;
}

