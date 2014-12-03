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
#ifndef _STRUS_LVDB_VARIABLE_SIZE_NODE_TREE_HPP_INCLUDED
#define _STRUS_LVDB_VARIABLE_SIZE_NODE_TREE_HPP_INCLUDED
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace strus {

class VarSizeNodeTree
{
public:
	typedef uint32_t Value;

	VarSizeNodeTree()
		:m_rootaddr(0){}

	bool set( const char* key, const Data& data)
	{
		NodeAddress addr = m_rootaddr;
		if (!addr) return 0;
		char const* ki = key;
		for (; *ki; ++ki)
		{
			NodeAddress next = successorNodeAddress( addr, *ki);
			if (!next)
			{
				addTail( addr, ki, data);
			}
			else
			{
				addr = next;
			}
		}
	}

	Value get( const char* key) const
	{
		NodeAddress addr = m_rootaddr;
		if (!addr) return 0;
		char const* ki = key;
		for (; *ki; ++ki)
		{
			addr = successorNodeAddress( addr, *ki);
			if (!addr) return 0;
		}
		if (nodeClassId(addr) == NodeClass::NodeData)
		{
			return m_datablock[ nodeIndex( addr)];
		}
		else
		{
			addr = successorNodeAddress( addr, 0xFF);
			if (nodeClassId(addr) != NodeClass::NodeData)
			{
				throw std::runtime_error( "currupt data (non UTF-8 string inserted)");
			}
			return m_datablock[ nodeIndex( addr)];
		}
	}

	class const_iterator
	{
	public:
		const_iterator()
			:m_tree(0)
		{}

		const_iterator( const VarSizeNodeTree* tree_, NodeAddress rootaddr)
			:m_tree(tree_)
		{
			m_stack.push_back( StackElement( rootaddr, 0));
			skip();
		}

		const std::string& key() const				{return m_key;}
		const Value& value() const				{return m_value;}

		bool operator==( const const_iterator& o) const		{return isequal(o);}
		bool operator!=( const const_iterator& o) const		{return !isequal(o);}

	private:
		void skip()
		{
			if (m_stack.empty()) return;
			m_key.clear();
			m_value = 0;

			for (;;)
			{
				NodeAddress addr = m_stack.back().m_addr;
				if (addr == NodeClass::NodeData && m_stack.back().m_itr==0)
				{
					std::vector<StackElement>::const_iterator
						si = m_stack.begin(), se = m_stack.end();
					for (; si != se; ++si)
					{
						if (*si) m_key.push_back( *si);
					}
					m_value = m_tree->m_datablock[
						nodeIndex( m_stack.back().m_addr)];
					m_stack.back().m_itr = 1;
					return;
				}
				else
				{
					std::size_t itr = m_stack.back().m_itr;
					char lexem;
					NodeAddress follow;
					if (getFollowNode( addr, itr, lexem, follow))
					{
						char chr = lexem==0xFF?0:lexem;
						m_stack.push_back( follow, 0, chr);
					}
					else for (;;)
					{
						m_stack.pop_back();
						if (m_stack.empty()) return;
						if (getNextNode( addr,
							m_stack.back().m_itr))
						{
							break;
						}
					}
				}
			}
		}

	private:
		bool isequal( const const_iterator& o) const
		{
			if (m_stack.size() != o.m_stack.size()) return false;
			std::vector<StackElement>::const_iterator
				ti = m_stack.begin(), te = m_stack.end(),
				oi = o.m_stack.begin(), oe = o.m_stack.end();
			for (; ti != te && oi != oe; ++ti,++oi)
			{
				if (ti->m_addr != oi->m_addr) return false;
				if (ti->m_itr != oi->m_itr) return false;
				if (ti->m_chr != oi->m_chr) return false;
			}
			return true;
		}

		struct StackElement
		{
			NodeAddress m_addr;
			std::size_t m_itr;
			char m_chr;

			StackElement( NodeAddress adr_, std::size_t itr_, char chr_=0)
				:m_chr(0),m_addr(addr_),m_itr(itr_),m_chr(chr_){}
			StackElement( const StackElement& o)
				:m_addr(o.m_addr),m_itr(o.m_itr),m_chr(o.m_chr){}
		};
		const VarSizeNodeTree* m_tree;
		std::vector<StackElement> m_stack;
		std::string m_key;
		NodeData::UnitType m_value;
	};

	const_iterator begin() const
	{
		return const_iterator( this, m_rootaddr);
	}
	const_iterator end() const
	{
		return const_iterator( this, m_rootaddr);
	}

private:
	typedef uint32_t NodeAddress;
	typedef uint32_t NodeIndex;

	struct NodeClass
	{
		enum Id
		{
			NodeData,
			Node1,
			Node4,
			Node16,
			Node256
		};
		enum {
			Mask  = 0x7,		///< 3 bit for node class
			Shift = 29,		///< 8 bit (char) for lexical value
			MaxNofNodes=(1<<21)	///< Maximum number of nodes in a block
		};
	};
	static inline NodeClass::Id nodeClassId( const NodeAddress& addr)
	{
		return (addr >> NodeClass::Shift) & NodeClass::Mask;
	}
	static inline NodeIndex nodeIndex( const NodeAddress& addr)
	{
		return addr & ((1<<NodeClass::Shift)-1);
	}
	static inline NodeAddress nodeAddress( NodeClass::Id classid, const NodeIndex& idx)
	{
		return idx | ((NodeAddress)classid << NodeClass::Shift); 
	}

	struct NodeData
	{
		typedef uint32_t UnitType;
	};

	class NodeIteratorData
	{
	public:
		NodeIteratorData( const NodeIteratorData& o)
			:m_lexem(o.m_lexem),m_address(o.m_address){}

		void init( unsigned char lexem_, const NodeAddress& address_)
		{
			m_lexem = successor_;
			m_address = address_;
		}

		bool end() const			{return m_lexem==0;}
		unsigned char lexem() const		{return m_lexem;}
		NodeAddress address() const		{return m_address;}

	private:
		unsigned char m_lexem;
		NodeAddress m_address;
	};

	struct Node1
	{
		enum {ThisNodeClass = NodeClass::Node1};
		enum {NodeSize      = 1};

		enum {
			LexemMask   = 0xFF,
			LexemShift  = 22,		///< 8 bit (char) for lexical value
			IndexMask   = (1<<22)-1,	///< 21 bit for node address
			IndexShift  = 0,
			AddressMask = (NodeClass::Mask << NodeClass::Shift)|IndexMask
		};

		typedef uint32_t UnitType;

		static inline unsigned char lexem( const UnitType& unit)
		{
			return (unit >> LexemShift) & LexemMask;
		}

		static inline NodeAddress successor( const UnitType& unit, unsigned char chr)
		{
			return (lexem( unit) == chr)?(unit&AddressMask):0;
		}

		static bool addNode( UnitType& node, unsigned char lexem, const NodeAddress& addr)
		{
			if (lexem( unit) != 0) return false;
			if ((addr & (LexemMask << LexemShift)) != 0) return false;
			node = addr | ((UnitType)lexem << LexemShift);
			return true;
		}

		class Iterator
			:NodeIteratorData
		{
			Iterator( const Iterator& o)
				:NodeIteratorData(o){}
			Iterator( const UnitType& unit)
				:NodeIteratorData(lexem(unit),unit&AddressMask){}
			Iterator( const UnitType* unitref, std::size_t idx_)
				:NodeIteratorData(idx_?0:lexem(*unitref),idx_?0:(*unitref&AddressMask)){}

			Iterator& operator++()		{init(0,0); return *this;}
			Iterator operator++(int)	{Iterator rt(unit); init(0,0); return rt;}
		};
	};

	static inline NodeAddress unpackAddress3( const unsigned char* ptr)
	{
		return    ((uint32_t)(ptr[0] & 0xE0) << NodeClass::Shift)
			| ((uint32_t)(ptr[0] & 0x1F) << 16)
			| ((uint32_t)(ptr[1]) <<  8)
			| ((uint32_t)(ptr[2]));
	}
	static inline void packAddress3( unsigned char* ptr, const NodeAddress& addr)
	{
		ptr[0] = (((addr >> (NodeClass::Shift)) & NodeClass::Mask) << 5)
			| ((addr >> 16) & 0x1F);
		ptr[1] =  ((addr >> 8)  & 0xFF);
		ptr[2] =  (addr         & 0xFF);
	}

	template <int NN>
	struct NodeN
		:public NodeClass
	{
		enum {NodeSize = NN};

		struct UnitType
		{
			unsigned char succ[ NN];
			unsigned char node[ 3*NN];
		};

		static inline NodeAddress successor( const UnitType& unit, unsigned char chr)
		{
			const char* nn = std::memchr( succ, chr, sizeof(succ));
			if (!nn) return 0;
			unsigned int ofs = (nn-succ)*3;

			NodeAddress rt = unpackAddress3( node + ofs);
		}

		static bool addNode( UnitType& node, unsigned char lexem, const NodeAddress& addr)
		{
			std::size_t ofs = 0;
			for (; ofs < NN && succ[ofs]; ++ofs){}
			if (ofs == NN) return false;
			succ[ ofs] = lexem;
			packAddress3( node+(ofs*3), addr);
			return true;
		}

		class Iterator
			:NodeIteratorData
		{
			Iterator( const Iterator& o)
				:NodeIteratorData(o),m_unitref(o.m_unitref),m_itr(o.m_itr){}
			explicit Iterator( const UnitType& unit)
				:NodeIteratorData(unit.succ[0],unpackAddress3(unit.node))
				,m_unitref(&unit)
				,m_itr(0){}
			Iterator( const UnitType* unitref_, std::size_t itr_)
				:NodeIteratorData(0,0)
				,m_unitref(unitref_)
				,m_itr(itr_)
			{
				extractData();
			}

			Iterator& operator++()		{skip(); return *this;}
			Iterator operator++(int)	{Iterator rt(*this); skip(); return rt;}

		private:
			void extractData()
			{
				NodexAddr aa = unpackAddress3( m_unitref->node+m_itr*3);
				init( m_unitref->succ[m_itr], aa);
			}

			void skip()
			{
				if (m_itr == NN-1)
				{
					init(0,0);
				}
				else
				{
					++m_itr;
					extractData();
				}
			}

		private:
			const UnitType* m_unitref;
			std::size_t m_itr;
		};
	};

	struct Node4
		:public NodeN<4>
	{
		enum {
			ThisNodeClass = NodeClass::Node4
		};
	};

	struct Node16
		:public NodeN<16>
	{
		enum {
			ThisNodeClass = NodeClass::Node16
		};
	};

	struct Node256
		:public NodeClass
	{
		enum {
			ThisNodeClass = NodeClass::Node256
		};

		struct UnitType
		{
			unsigned char node[256*3];
		};

		static inline NodeAddress successor( const UnitType& unit, unsigned char chr)
		{
			unsigned int ofs = chr*3;
			NodeAddress rt = unpackAddress3( node + ofs);
		}

		static bool addNode( UnitType& node, unsigned char lexem, const NodeAddress& addr)
		{
			std::size_t ofs = lexem;
			packAddress3( node+(ofs*3), addr);
			return true;
		}

		class Iterator
			:NodeIteratorData
		{
			Iterator( const Iterator& o)
				:NodeIteratorData(o)
				,m_unitref(o.m_unitref)
				,m_itr(o.m_itr){}
			explicit Iterator( const UnitType& unit)
				:NodeIteratorData(0,0)
				,m_unitref(&unit)
				,m_itr(0)
			{
				skip();
			}
			Iterator( const UnitType* unitref_, std::size_t itr_)
				:NodeIteratorData(0,0)
				,m_unitref(unitref_)
				,m_itr(itr_)
			{
				extractData();
			}

			Iterator& operator++()
			{
				if (m_itr < 256)
				{
					++m_itr; skip();
				}
				return *this;
			}
			Iterator operator++(int)
			{
				Iterator rt(*this);
				if (m_itr < 256)
				{
					++m_itr; skip();
				}
				return rt;
			}

		private:
			void extractData()
			{
				NodexAddr aa = unpackAddress3( m_unitref->node + m_itr*3);
				init( m_itr, aa);
			}

			void skip()
			{
				if (m_itr == 256)
				{
					init(0,0);
				}
				else
				{
					std::size_t ii = m_itr*3;
					while (ii<(256*3) && !m_unitref->node[ii])
					{
						m_itr++;
						ii += 3;
					}
					if (ii == (256*3))
					{
						init(0,0);
					}
					else
					{
						extractData();
					}
				}
			}

		private:
			const UnitType* m_unitref;
			std::size_t m_itr;
		};
	};

private:
	template <class NODETYPE>
	class Block
	{
	public:
		typedef typename NODETYPE NodeType;

	public:
		Block()
			:m_ar(0),m_size(0),m_allocsize(0),m_freelist(0){}

		~Block()
		{
			if (m_ar) std::free( m_ar);
		}

		NodeIndex allocNode( bool useFreeList=true)
		{
			if (m_freelist && useFreeList)
			{
				NodeIndex rt = m_freelist-1;
				m_freelist = *reinterpret_cast<uint32_t*>(m_ar + rt)+1;
				std::memset( m_ar + rt, 0, sizeof( m_ar[0]));
				return rt;
			}
			if (m_size == m_allocsize)
			{
				unsigned int mm = m_allocsize?(m_allocsize * 4):16;
				if (mm < m_allocsize) throw std::bad_alloc();
				NodeType::UnitType* newar
					= (NodeType::UnitType*)std::realloc(
						m_ar, mm * sizeof( NodeType::UnitType));
				if (!newar) throw std::bad_alloc();
				m_ar = newar;
				m_allocsize = mm;
			}
			std::memset( m_ar + m_size, 0, sizeof( m_ar[0]));
			if (m_size > NodeClass::MaxNofNodes)
			{
				throw std::logic_error( "number of nodes exceeds maximum limit");
			}
			return m_size++;
		}

		void releaseNode( const NodeIndex& idx)
		{
			checkAccess(idx);
			*reinterpret_cast<uint32_t*>(m_ar + idx) = (m_freelist-1);
			m_freelist = idx+1;
		}

		void checkAddress( const NodeAddress& addr)
		{
			if ((int)((addr >> NodeClass::Shift) & NodeClass::Mask)
			!=  (int)NodeType::ThisNodeClass)
			{
				throw std::logic_error( "accessing block with illegal node address");
			}
		}
		void checkAccess( const NodeIndex& idx)
		{
			if (idx >= m_size) throw std::logic_error( "accessing block with node index out of range");
		}

		NodeType::Iterator nodeIterator( const NodeAddress& addr)
		{
			checkAddress( addr);
			NodeIndex idx = nodeIndex( addr);
			checkAccess( idx);
			return NodeType::Iterator( m_ar[ idx]);
		}

		NodeType::UnitType& operator[]( std::size_t idx)
		{
			checkAccess( idx);
			return m_ar[ idx];
		}

		const NodeType::UnitType& operator[]( std::size_t idx) const
		{
			checkAccess( idx);
			return m_ar[ idx];
		}

		void clear()
		{
			m_size = 0;
			m_freelist = 0;
		}

	private:
		NodeType::UnitType* m_ar;
		std::size_t m_size;
		std::size_t m_allocsize;
		uint32_t m_freelist;
	};

	template <class DestNodeType, class SourceNodeType>
	NodeAddress moveBlock( Block<DestNodeType>& dstblk, Block<SourceNodeType>& srcblk, NodeAddress srcadr)
	{
		NodeIndex dstidx = dstblk.allocNode();
		NodeAddress dstadr = nodeAddress( DestNodeType::ThisNodeClass, dstidx);
		SourceNodeType::Iterator srcitr = srcblk.nodeIterator( srcaddr);
		NodeType::UnitType& dstunit = dstblk[ dstidx];
		for (; !srcitr.end(); ++srcitr)
		{
			if (!DestNodeType::addNode(
				dstunit, srcitr.lexem(), srcitr.address()))
			{
				throw std::runtime_error( "destination node does not have the capacity of the source node");
			}
		}
		NodeIndex srcidx = nodeIndex( srcadr);
		srcblk.releaseNode( srcidx);
	}

	NodeAddress successorNodeAddress( const NodeAddress& addr, unsigned char lexem)
	{
		NodeClass::Id classid = nodeClassId(addr);
		NodeIndex idx = nodeIndex( addr);
		switch (classid)
		{
			case NodeClass::NodeData:
				return 0;
			case NodeClass::Node1:
				return Node1::successor( m_block1[ idx], lexem);
			case NodeClass::Node4:
				return Node4::successor( m_block4[ idx], lexem);
			case NodeClass::Node16:
				return Node16::successor( m_block16[ idx], lexem);
			case NodeClass::Node256:
				return Node256::successor( m_block256[ idx], lexem);
		}
	}

	bool addNode( const NodeAddress& addr, unsigned char lexem, const NodeAddress& follow)
	{
		NodeClass::Id classid = nodeClassId( addr);
		NodeIndex idx = nodeIndex( addr);
		switch (classid)
		{
			case NodeClass::NodeData:
				return false;
			case NodeClass::Node1:
				return Node1::addNode( m_block1[ idx], lexem, follow);
			case NodeClass::Node4:
				return Node4::addNode( m_block4[ idx], lexem, follow);
			case NodeClass::Node16:
				return Node16::addNode( m_block16[ idx], lexem, follow);
			case NodeClass::Node256:
				return Node256::addNode( m_block256[ idx], lexem, follow);
		}
	}

	NodeAddress expandNode( const NodeAddress& addr)
	{
		switch (nodeClassId( addr))
		{
			case NodeClass::NodeData:
			{
				NodeIndex newidx = m_block4.allocNode();
				NodeAddress newaddr = nodeAddress( NodeClass::Node4, newidx);
				Node4::addNode( m_block4[ newidx], 0xFF, addr);
				return newaddr;
			}
			case NodeClass::Node1:
				return moveBlock( m_block4, m_block1, addr);
			case NodeClass::Node4:
				return moveBlock( m_block16, m_block4, addr);
			case NodeClass::Node16:
				return moveBlock( m_block256, m_block16, addr);
			case NodeClass::Node256:
				throw std::logic_error("called expand block on block with nodes having maximum capacity");
		}
	}

	NodeAddress addTail( const NodeAddress& addr, const char* tail, const NodeData::UnitType& data)
	{
		if (!*tail)
		{
			NodeIndex idx = m_datablock.size();
			followaddr = nodeAddress( NodeClass::NodeData, idx);
			m_datablock.push_back( data);
			addNode( addr, 0xFF, followaddr);
		}
		else
		{
			char const* ti = tail;
			NodeAddress aa = addr;
			for (;ti[1]; ++ti)
			{
				idx = m_block1.allocNode( false);
				// ... do not use free list for tail blocks to put
				//	them close to each other in memory
				followaddr = nodeAddress( NodeClass::Node1, idx);
				if (!addNode( aa, *ti, followaddr))
				{
					aa = expandNode( aa);
					if (!addNode( aa, *ti, followaddr))
					{
						throw std::logic_error("add node failed after expand node");
					}
				}
				aa = followaddr;
			}
			NodeIndex idx = m_datablock.size();
			followaddr = nodeAddress( NodeClass::NodeData, idx);
			m_datablock.push_back( data);
			addNode( addr, *ti, followaddr);
		}
	}

	bool getFollowNode( const NodeAddress& addr, std::size_t idx, char& lexem, NodeAddress& follow)
	{
		NodeIndex idx = nodeIndex( addr);
	
		switch (nodeClassId( addr))
		{
			case NodeClass::NodeData:
			{
				return false; 
			}
			case NodeClass::Node1:
			{
				if (idx >= 1) return false;
				Node1::Iterator itr( m_block1[ idx]);
				lexem = itr->lexem();
				follow = itr->address();
				return true;
			}
			case NodeClass::Node4:
			{
				if (idx >= 4) return false;
				Node4::Iterator itr( m_block4[ idx]);
				lexem = itr->lexem();
				follow = itr->address();
				return true;
			}
			case NodeClass::Node16:
			{
				if (idx >= 16) return false;
				Node16::Iterator itr( m_block16[ idx]);
				lexem = itr->lexem();
				follow = itr->address();
				return true;
			}
			case NodeClass::Node256:
			{
				if (idx >= 256) return false;
				Node256::Iterator itr( m_block256[ idx]);
				lexem = itr->lexem();
				follow = itr->address();
				return true;
			}
		}
	}

	bool getNextNode( const NodeAddress& addr, std::size_t& idx)
	{
		NodeIndex idx = nodeIndex( addr);
	
		switch (nodeClassId( addr))
		{
			case NodeClass::NodeData:
			{
				return false; 
			}
			case NodeClass::Node1:
			{
				return false;
			}
			case NodeClass::Node4:
			{
				if (idx >= 3) return false;
				++idx;
				Node4::Iterator itr( m_block4[ idx]);
				return (itr->lexem()!=0);
			}
			case NodeClass::Node16:
			{
				if (idx >= 15) return false;
				++idx;
				Node16::Iterator itr( m_block16[ idx]);
				return (itr->lexem()!=0);
			}
			case NodeClass::Node256:
			{
				if (idx >= 255) return false;
				++idx;
				Node256::Iterator itr( m_block256[ idx]);
				itr.skip();
				return (itr->lexem()!=0);
			}
		}
	}

private:
	std::vector<NodeData::UnitType> m_datablock;
	Block<Node1> m_block1;
	Block<Node4> m_block4;
	Block<Node16> m_block16;
	Block<Node256> m_block256;
	NodeAddress m_rootaddr;
};

}//namespace
#endif

