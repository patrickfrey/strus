#include "iterator/postingIteratorUnion.hpp"
#include <cstdlib>

using namespace strus;


IteratorUnion::IteratorUnion( std::size_t nofargs_, PostingIteratorInterface** args_)
	:m_docno(0)
	,m_posno(0)
	,m_argarsize(nofargs_)
	,m_selected(0)
	,m_documentFrequency(-1)
{
	if (nofargs_ > 64)
	{
		throw std::runtime_error( "number of arguments of union out of range (> 64)");
	}
	m_argar = (PostingIteratorInterface**)malloc( nofargs_ * sizeof(m_argar[0]));
	if (!m_argar) throw std::bad_alloc();
	try
	{
		std::size_t ii=0;
		for (; ii<nofargs_; ++ii)
		{
			m_argar[ii] = args_[ii];
			if (ii) m_featureid.push_back('=');
			m_featureid.append( args_[ii]->featureid());
		}
		m_featureid.push_back( 'U');
	}
	catch (const std::bad_alloc&)
	{
		std::free( m_argar);
		throw std::bad_alloc();
	}
}

IteratorUnion::~IteratorUnion()
{
	std::size_t ii=0;
	for (; ii<m_argarsize; ++ii)
	{
		delete m_argar[ii];
	}
	std::free( m_argar);
}

std::vector<const PostingIteratorInterface*>
	IteratorUnion::subExpressions( bool positive) const
{
	std::vector<const PostingIteratorInterface*> rt;
	if (positive)
	{
		rt.reserve( m_argarsize);
		std::size_t ii=0;
		for (; ii<m_argarsize; ++ii)
		{
			rt.push_back( m_argar[ ii]);
		}
	}
	return rt;
}

static inline Index selectSmallerNotNull( Index idx0, Index idx1)
{
	if (idx0 <= idx1)
	{
		if (idx0)
		{
			return idx0;
		}
		else
		{
			return idx1;
		}
	}
	else
	{
		if (idx1)
		{
			return idx1;
		}
		else
		{
			return idx0;
		}
	}
}

Index IteratorUnion::skipDoc( const Index& docno_)
{
	if (m_docno == docno_ && docno_)
	{
		return m_docno;
	}
	m_docno = docno_;
	std::size_t ai = 0, ae = m_argarsize;
	if (ai == ae) return 0;
	Index base = docno_?docno_:1;
	Index minimum = 0;

	clearSelected();
	int aidx=0;
	for (; ai != ae; ++ai,++aidx)
	{
		minimum = m_argar[ ai]->skipDoc( base);
		if (minimum) break;
	}
	if (!minimum)
	{
		m_docno = 0;
		return 0;
	}
	setSelected( aidx);

	for (aidx++,ai++; ai != ae; ++ai,++aidx)
	{
		Index next = m_argar[ ai]->skipDoc( base);
		if (next && next <= minimum)
		{
			if (next < minimum)
			{
				clearSelected();
				minimum = next;
			}
			setSelected( aidx);
		}
	}
	m_docno = minimum;
	return m_docno;
}

Index IteratorUnion::skipPos( const Index& pos_)
{
	selected_iterator si = selected_begin(), se = selected_end();
	Index pos = 0;
	Index basepos = pos_?pos_:1;
	for (; si != se; ++si)
	{
		pos = selectSmallerNotNull( pos, si->skipPos( basepos));
	}
	return m_posno=pos;
}

Index IteratorUnion::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		std::size_t ai = 0, ae = m_argarsize;
		if (ai == ae) return 0;
		m_documentFrequency = m_argar[ ai]->documentFrequency();
		for (++ai; ai != ae && m_documentFrequency < 0; ++ai)
		{
			Index df = m_argar[ ai]->documentFrequency();
			if (df > m_documentFrequency)
			{
				m_documentFrequency = df;
			}
		}
	}
	return m_documentFrequency;
}


