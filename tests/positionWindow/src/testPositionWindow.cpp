#include "positionWindow.hpp"
#include "strus/lib/error.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/reference.hpp"
#include "strus/base/local_ptr.hpp"
#include <cstdio>
#include <iostream>
#include <memory>

#undef STRUS_LOWLEVEL_DEBUG

static strus::ErrorBufferInterface* g_errorbuf = 0;

class SimplePostingIterator
	:public strus::PostingIteratorInterface
{
public:
	enum {MaxPosArraySize=64};
	SimplePostingIterator( const unsigned int* posar_, unsigned int id)
		:m_posarsize(0),m_posidx(0),m_docno(0),m_posno(0)
	{
		for (;m_posarsize < MaxPosArraySize && posar_[m_posarsize]; ++m_posarsize)
		{
			m_posar[ m_posarsize] = posar_[m_posarsize];
		}
		snprintf( m_featureid, sizeof(m_featureid), "simple(%u)", id);
	}

	virtual strus::Index skipDoc( const strus::Index& docno_)
	{
		return m_docno = docno_?docno_:docno_+1;
	}

	virtual strus::Index skipDocCandidate( const strus::Index& docno_)
	{
		return m_docno = docno_?docno_:docno_+1;
	}

	virtual strus::Index skipPos( const strus::Index& firstpos)
	{
		if (m_posarsize == 0) return 0;
		while (m_posidx < m_posarsize && m_posar[m_posidx] < firstpos)
		{
			++m_posidx;
		}
		if (m_posidx == m_posarsize)
		{
			m_posidx = 0;
			return m_posno = 0;
		}
		while (m_posidx > 0 && m_posar[m_posidx-1] >= firstpos)
		{
			--m_posidx;
		}
		return m_posno = m_posar[m_posidx];
	}

	virtual const char* featureid() const
	{
		return m_featureid;
	}

	virtual strus::Index documentFrequency() const
	{
		return 0;
	}

	virtual int frequency()
	{
		return m_posarsize;
	}

	virtual strus::Index docno() const
	{
		return m_docno;
	}

	virtual strus::Index posno() const
	{
		return m_posno;
	}

	virtual strus::Index length() const
	{
		return m_posno ? 1:0;
	}

private:
	char m_featureid[ 32];
	strus::Index m_posar[ MaxPosArraySize];
	std::size_t m_posarsize;
	std::size_t m_posidx;
	strus::Index m_docno;
	strus::Index m_posno;
};


static void testWinWindow()
{
	struct Result
	{
		unsigned int pos;
		unsigned int size;
	};
	static const unsigned int ar[3][32] = {{1,5,9,15,0},{2,6,10,16,0},{5,11,18,0}};
	static const Result res[32] = {{1,4},{2,3},{5,1},{5,4},{6,5},{9,2},{10,5},{11,5},{15,3},{0,0}};

	std::vector<strus::Reference<SimplePostingIterator> > argbufs;
	std::vector<strus::PostingIteratorInterface*> args;
	for (std::size_t ii=0; ii<3; ++ii)
	{
		argbufs.push_back( new SimplePostingIterator( ar[ii], ii+1));
		args.push_back( argbufs.back().get());
	}
	Result const* ri = res;
	strus::PositionWindow win( args.data(), args.size(), 10, 0, 0, strus::PositionWindow::MinWin);
	bool more=win.first();
	for (; more && ri->pos; more=win.next(),++ri)
	{
		if (win.pos() != ri->pos || win.size() != ri->size)
		{
			std::cerr << "error window position " << win.pos() << " size " << win.size() << ", expected position " << ri->pos << " size " << ri->size << std::endl;
			throw std::runtime_error( "test failed");
		}
#ifndef STRUS_LOWLEVEL_DEBUG
		std::cerr << "window position " << win.pos() << " size " << win.size() << std::endl;
#endif
	}
	if (more) throw std::runtime_error( "test failed: more matches than expected");
	if (ri->pos) throw std::runtime_error( "test failed: not all matches found");
}

int main( int argc, char** argv)
{
	try
	{
		strus::local_ptr<strus::ErrorBufferInterface> errorbuf( strus::createErrorBuffer_standard( stderr, 2, NULL/*debug trace interface*/));
		g_errorbuf = errorbuf.get();

		testWinWindow();
	}
	catch (const std::exception& err)
	{
		std::cerr << "ERROR " << err.what() << std::endl;
		return -1;
	}
	return 0;
}

