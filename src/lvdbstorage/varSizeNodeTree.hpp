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
#include <vector>
#include <cstring>
#include <iostream>
#include <stdint.h>

namespace strus {

class VarSizeNodeTree
{
public:
	typedef uint32_t NodeData;
	typedef uint32_t NodeAddress;
	typedef uint32_t NodeIndex;

public:
	VarSizeNodeTree()
		:m_rootaddr(0){}

	void set( const char* key, const NodeData& data)
	{
		if (!m_rootaddr)
		{
			m_rootaddr = nodeAddress( NodeClass::Node1, m_block1.allocNode());
		}
		NodeAddress addr = m_rootaddr;
		NodeAddress prev_addr = 0;

		unsigned char const* ki = (const unsigned char*)key;
		for (; *ki; ++ki)
		{
			NodeAddress next = successorNodeAddress( addr, *ki);
			if (!next)
			{
				if (m_rootaddr == addr)
				{
					addTail( 0, addr, ki, data);
					m_rootaddr = addr;
				}
				else
				{
					addTail( prev_addr, addr, ki, data);
				}
				return;
			}
			else if (nodeClassId( next) == NodeClass::NodeData)
			{
				NodeIndex idx = m_block4.allocNode();
				NodeAddress newaddr = nodeAddress( NodeClass::Node4, idx);
				if (!addNode( newaddr, 0xFF, next))
				{
					throw std::logic_error("illegal state (empty addNode4 returns false)");
				}
				unlinkNode( addr, *ki);
				if (!addNode( addr, *ki, newaddr))
				{
					throw std::logic_error("illegal state (addNode after unlink returns false)");
				}
				if (m_rootaddr == addr)
				{
					m_rootaddr = newaddr;
				}
				addr = newaddr;
			}
			else
			{
				prev_addr = addr;
				addr = next;
			}
		}
		if (m_rootaddr == addr)
		{
			addTail( 0, addr, ki, data);
			m_rootaddr = addr;
		}
		else
		{
			addTail( prev_addr, addr, ki, data);
		}
	}

	bool find( const char* key, NodeData& val) const
	{
		NodeAddress addr = m_rootaddr;
		if (!addr) return false;
		unsigned char const* ki = (const unsigned char*)key;
		for (; *ki; ++ki)
		{
			addr = successorNodeAddress( addr, *ki);
			if (!addr) return false;
		}
		if (nodeClassId(addr) == NodeClass::NodeData)
		{
			val = m_datablock[ nodeIndex( addr)];
			return true;
		}
		else
		{
			addr = successorNodeAddress( addr, 0xFF);
			if (!addr) return false;

			if (nodeClassId(addr) != NodeClass::NodeData)
			{
				throw std::runtime_error( "currupt data (non UTF-8 string inserted)");
			}
			val = m_datablock[ nodeIndex( addr)];
			return true;
		}
	}

	class const_iterator
	{
	public:
		const_iterator()
			:m_tree(0)
		{}
		const_iterator( const const_iterator& o)
			:m_tree(o.m_tree)
			,m_stack(o.m_stack)
			,m_key(o.m_key)
			,m_data(o.m_data){}

		const_iterator( const VarSizeNodeTree* tree_, NodeAddress rootaddr)
			:m_tree(tree_)
		{
			m_stack.push_back( StackElement( rootaddr, 0));
			skip();
		}

		const std::string& key() const				{return m_key;}
		const NodeData& data() const				{return m_data;}

		bool operator==( const const_iterator& o) const		{return isequal(o);}
		bool operator!=( const const_iterator& o) const		{return !isequal(o);}

		const_iterator& operator++()				{skip(); return *this;}
		const_iterator operator++(int)				{const_iterator rt(*this); skip(); return rt;}

	private:
		void extractData()
		{
			std::vector<StackElement>::const_iterator
				si = m_stack.begin(), se = m_stack.end();
			for (; si != se; ++si)
			{
				if (si->m_chr) m_key.push_back( si->m_chr);
			}
			NodeIndex dataidx = nodeIndex( m_stack.back().m_addr);
			m_data = m_tree->m_datablock[ dataidx];
		}

		void skipNode()
		{
			unsigned char lexem;
			NodeAddress follow;
			if (m_tree->getFollowNode( 
					m_stack.back().m_addr,
					m_stack.back().m_itr,
					lexem, follow))
			{
				char chr = lexem==0xFF?0:lexem;
				m_stack.push_back( StackElement( follow, 0, chr));
			}
			else for (;;)
			{
				m_stack.pop_back();
				if (m_stack.empty()) return;
				if (m_tree->getNextNode( 
						m_stack.back().m_addr,
						m_stack.back().m_itr))
				{
					break;
				}
			}
		}

		void skip()
		{
			/*[-]*/std::cout << "ITERATOR STATE:" << std::endl;
			/*[-]*/printState( std::cout);
			if (m_stack.empty()) return;
			m_key.clear();
			m_data = 0;

			for (;;)
			{
				NodeAddress addr = m_stack.back().m_addr;
				if (nodeClassId(addr) == NodeClass::NodeData
				&&  m_stack.back().m_itr == 0)
				{
					extractData();
					m_stack.back().m_itr = 1;
					return;
				}
				else
				{
					skipNode();
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
		void printState( std::ostream& out)
		{
			out << "(" << m_key << "," << m_data << "):";
			std::vector<StackElement>::const_iterator si = m_stack.begin(), se = m_stack.end();
			for (; si != se; ++si)
			{
				out << ' ';
				printLexem( out, si->m_chr);
				out << ' ';
				printNodeAddress( out, si->m_addr);
				out << "~" << si->m_itr << ",";
			}
			out << std::endl;
		}

		struct StackElement
		{
			NodeAddress m_addr;
			std::size_t m_itr;
			char m_chr;

			StackElement( NodeAddress addr_, std::size_t itr_, char chr_=0)
				:m_addr(addr_),m_itr(itr_),m_chr(chr_){}
			StackElement( const StackElement& o)
				:m_addr(o.m_addr),m_itr(o.m_itr),m_chr(o.m_chr){}
		};
		const VarSizeNodeTree* m_tree;
		std::vector<StackElement> m_stack;
		std::string m_key;
		NodeData m_data;
	};

	const_iterator begin() const
	{
		return const_iterator( this, m_rootaddr);
	}
	const_iterator end() const
	{
		return const_iterator();
	}

	void print( std::ostream& out)
	{
		if (m_rootaddr) printNode( out, m_rootaddr, std::string());
	}

	void clear()
	{
		m_datablock.clear();
		m_block1.clear();
		m_block2.clear();
		m_block4.clear();
		m_block8.clear();
		m_block16.clear();
		m_block256.clear();
		m_rootaddr = 0;
	}

private:
	struct NodeClass
	{
		enum Id
		{
			NodeData,
			Node1,
			Node2,
			Node4,
			Node8,
			Node16,
			Node256
		};
		static const char* idName( Id i)
		{
			static const char* ar[] = {"Data","S1","S2","S4","S8","S16","S256"};
			return ar[i];
		}
		enum {
			Mask  = 0x7,		///< 3 bit for node class
			Shift = 29,		///< most significant 3 bits
			MaxNofNodes=(1<<21)	///< Maximum number of nodes in a block
		};
	};
	static NodeClass::Id nodeClassId( const NodeAddress& addr)
	{
		return (NodeClass::Id)(((unsigned int)addr >> (unsigned int)NodeClass::Shift) & (unsigned int)NodeClass::Mask);
	}
	static NodeIndex nodeIndex( const NodeAddress& addr)
	{
		return addr & (NodeAddress)((1<<(unsigned int)NodeClass::Shift)-1);
	}
	static NodeAddress nodeAddress( NodeClass::Id classid, const NodeIndex& idx)
	{
		return idx | ((NodeAddress)classid << (unsigned int)NodeClass::Shift); 
	}
	static void printNodeAddress( std::ostream& out, NodeAddress addr)
	{
		out << NodeClass::idName( nodeClassId( addr)) << ":" << nodeIndex(addr);
	}
	static void printLexem( std::ostream& out, unsigned char chr)
	{
		if (chr >= 32 && chr <= 128)
		{
			out << "'" << (char)chr << "' ";
		}
		else
		{
			out << "[" << std::hex
				<< (unsigned int)(unsigned char)chr
				<< std::dec << "] ";
		}
	}

	class NodeIteratorData
	{
	public:
		NodeIteratorData()
			:m_lexem(0),m_address(0),m_idx(0){}
		NodeIteratorData( const NodeIteratorData& o)
			:m_lexem(o.m_lexem),m_address(o.m_address),m_idx(o.m_idx){}
		NodeIteratorData( unsigned char lexem_, const NodeAddress& address_, std::size_t idx_=0)
			:m_lexem(lexem_),m_address(address_),m_idx(idx_){}

		void init( unsigned char lexem_, const NodeAddress& address_, std::size_t idx_=0)
		{
			m_lexem = lexem_;
			m_address = address_;
			m_idx = 0;
		}

		bool end() const			{return m_lexem==0;}
		unsigned char lexem() const		{return m_lexem;}
		NodeAddress address() const		{return m_address;}
		std::size_t index() const		{return m_idx;}

	protected:
		unsigned char m_lexem;
		NodeAddress m_address;
		std::size_t m_idx;
	};

	struct Node1
	{
		/* Node1 unit structure
		*		int:3	NodeClass::Id
		*		int:8	In case of one successor nodes used as lexical value for the successor node
		*		int:21	Index part of the address
		*/
		enum {ThisNodeClass = NodeClass::Node1};
		enum {NodeSize      = 1};

		enum {
			LexemMask   = 0xFF,
			LexemShift  = 21,		///< 8 bit (char) for lexical value
			IndexMask   = (1<<21)-1,	///< 21 bit for node address
			IndexShift  = 0,
			AddressMask = (NodeClass::Mask << NodeClass::Shift)|IndexMask
		};

		typedef uint32_t UnitType;

		static unsigned char lexem( const UnitType& unit)
		{
			return (unit >> LexemShift) & LexemMask;
		}

		static NodeAddress address( const UnitType& unit)
		{
			return unit & AddressMask;
		}

		static NodeAddress successor( const UnitType& unit, unsigned char chr)
		{
			return (lexem( unit) == chr)?address(unit):0;
		}

		static void unlink( UnitType& unit, unsigned char chr)
		{
			if (lexem( unit) == chr)
			{
				unit = 0;
			}
		}

		static bool addNode( UnitType& unit, unsigned char lexem_, const NodeAddress& addr_)
		{
			if (lexem( unit) != 0) return false;
			if ((addr_ & (LexemMask << LexemShift)) != 0) return false;
			unit = addr_ | ((UnitType)lexem_ << LexemShift);
			return true;
		}

		static void patchNodeAddress( UnitType& unit, const NodeAddress& oldaddr, const NodeAddress& newaddr)
		{
			if (address( unit) != oldaddr) throw std::logic_error( "illegal patch operation (previous address does not match)");
			unit = newaddr | (lexem(unit) << LexemShift);
		}

		class Iterator
			:public NodeIteratorData
		{
		public:
			Iterator( const Iterator& o)
				:NodeIteratorData(o){}
			Iterator( const UnitType& unit)
				:NodeIteratorData(
					Node1::lexem(unit),
					Node1::address(unit),
					0){}
			Iterator( const UnitType* unitref, std::size_t idx_)
			{
				if (idx_)
				{
					init( 0,0,1);
				}
				else
				{
					init( Node1::lexem(*unitref),
						Node1::address(*unitref), 0);
				}
			}

			Iterator& operator++()		{init(0,0,1); return *this;}
			Iterator operator++(int)	{Iterator rt(*this); init(0,0,1); return rt;}
		};
	};

	static NodeAddress unpackAddress3( const unsigned char* ptr)
	{
		return    ((uint32_t)(ptr[0] & 0xE0) << NodeClass::Shift)
			| ((uint32_t)(ptr[0] & 0x1F) << 16)
			| ((uint32_t)(ptr[1]) <<  8)
			| ((uint32_t)(ptr[2]));
	}
	static void packAddress3( unsigned char* ptr, const NodeAddress& addr)
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

		static NodeAddress successor( const UnitType& unit, unsigned char chr)
		{
			const unsigned char* nn = (const unsigned char*)std::memchr( unit.succ, chr, sizeof(unit.succ));
			if (!nn) return 0;
			unsigned int ofs = (nn-unit.succ)*3;

			return unpackAddress3( unit.node + ofs);
		}

		static bool addNode( UnitType& unit, unsigned char lexem, const NodeAddress& addr)
		{
			std::size_t ofs = 0;
			for (; ofs < NN && unit.succ[ofs]; ++ofs){}
			if (ofs == NN) return false;
			unit.succ[ ofs] = lexem;
			packAddress3( unit.node+(ofs*3), addr);
			return true;
		}

		static void patchNodeAddress( UnitType& unit, const NodeAddress& oldaddr, const NodeAddress& newaddr)
		{
			unsigned char buf[3];
			packAddress3( buf, oldaddr);
			std::size_t sofs = 0;
			std::size_t nofs = 0;
			for (; sofs < NN; ++sofs,nofs+=3)
			{
				if (std::memcmp( buf, unit.node+nofs, 3) == 0) break;
			}
			if (sofs == NN) std::logic_error( "illegal patch address operation (old address not found)");
			packAddress3( unit.node + nofs, newaddr);
		}

		static void unlink( UnitType& unit, unsigned char chr)
		{
			std::size_t ofs = 0;
			for (; ofs < NN && unit.succ[ofs] && unit.succ[ofs] != chr; ++ofs){}
			if (ofs == NN || unit.succ[ofs] == 0) return;
			unsigned char* si = unit.succ + ofs;
			unsigned char* se = unit.succ + NN;
			std::memmove( si, si+1, se-si-1);
			unit.succ[ NN -1] = 0;
			unsigned char* ni = unit.node+(ofs*3);
			unsigned char* ne = unit.node+(NN*3);
			std::memmove( ni, ni+3, ne-ni-3);
			std::memset( ne-3, 0, 3);
		}

		class Iterator
			:public NodeIteratorData
		{
		public:
			Iterator( const Iterator& o)
				:NodeIteratorData(o),m_unitref(o.m_unitref){}
			explicit Iterator( const UnitType& unit)
				:NodeIteratorData(unit.succ[0],unpackAddress3(unit.node),0)
				,m_unitref(&unit){}
			Iterator( const UnitType* unitref_, std::size_t itr_)
				:NodeIteratorData(0,0,itr_)
				,m_unitref(unitref_)
			{
				extractData();
			}

			Iterator& operator++()		{skip(); return *this;}
			Iterator operator++(int)	{Iterator rt(*this); skip(); return rt;}

		private:
			void extractData()
			{
				NodeAddress aa = unpackAddress3( m_unitref->node+m_idx*3);
				init( m_unitref->succ[m_idx], aa, m_idx);
			}

			void skip()
			{
				if (m_idx == NN-1)
				{
					init(0,0,NN);
				}
				else
				{
					++m_idx;
					extractData();
				}
			}

		private:
			const UnitType* m_unitref;
		};
	};

	struct Node2
		:public NodeN<2>
	{
		enum {
			ThisNodeClass = NodeClass::Node2
		};

		static NodeAddress successor( const UnitType& unit, unsigned char chr)
		{
			if (unit.succ[0] == chr)
			{
				return unpackAddress3( unit.node + 0);
			}
			else if (unit.succ[1] == chr)
			{
				return unpackAddress3( unit.node + 3);
			}
			else
			{
				return 0;
			}
		}
	};

	struct Node4
		:public NodeN<4>
	{
		enum {
			ThisNodeClass = NodeClass::Node4
		};
	};

	struct Node8
		:public NodeN<8>
	{
		enum {
			ThisNodeClass = NodeClass::Node8
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
			ThisNodeClass = NodeClass::Node256,
			NodeSize = 256
		};

		struct UnitType
		{
			unsigned char node[256*3];
		};

		static NodeAddress successor( const UnitType& unit, unsigned char chr)
		{
			unsigned int ofs = chr*3;
			return unpackAddress3( unit.node + ofs);
		}

		static void unlink( UnitType& unit, unsigned char chr)
		{
			unsigned int ofs = chr*3;
			std::memset( unit.node+ofs, 0, 3);
		}

		static bool addNode( UnitType& unit, unsigned char lexem, const NodeAddress& addr)
		{
			std::size_t ofs = lexem;
			packAddress3( unit.node+(ofs*3), addr);
			return true;
		}

		static void patchNodeAddress( UnitType& unit, const NodeAddress& oldaddr, const NodeAddress& newaddr)
		{
			unsigned char buf[3];
			packAddress3( buf, oldaddr);
			std::size_t sofs = 0;
			std::size_t nofs = 0;
			for (; sofs < 256; ++sofs,nofs+=3)
			{
				if (std::memcmp( buf, unit.node+nofs, 3) == 0) break;
			}
			if (sofs == 256) std::logic_error( "illegal patch address operation (old address not found)");
			packAddress3( unit.node + nofs, newaddr);
		}

		class Iterator
			:public NodeIteratorData
		{
		public:
			Iterator( const Iterator& o)
				:NodeIteratorData(o)
				,m_unitref(o.m_unitref){}
			explicit Iterator( const UnitType& unit)
				:NodeIteratorData(0,0,0)
				,m_unitref(&unit)
			{
				skip();
			}
			Iterator( const UnitType* unitref_, std::size_t itr_)
				:NodeIteratorData(0,0,itr_)
				,m_unitref(unitref_)
			{
				extractData();
			}

			Iterator& operator++()
			{
				if (m_idx < 256)
				{
					++m_idx; skip();
				}
				return *this;
			}
			Iterator operator++(int)
			{
				Iterator rt(*this);
				if (m_idx < 256)
				{
					++m_idx; skip();
				}
				return rt;
			}

			std::size_t index() const
			{
				return m_idx;
			}

		private:
			void extractData()
			{
				NodeAddress aa = unpackAddress3( m_unitref->node + m_idx*3);
				init( m_idx, aa);
			}

			void skip()
			{
				if (m_idx == 256)
				{
					init(0,0);
				}
				else
				{
					std::size_t ii = m_idx*3;
					while (ii<(256*3) && !m_unitref->node[ii])
					{
						m_idx++;
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
		};
	};

private:
	template <class NODETYPE>
	class Block
	{
	public:
		typedef NODETYPE NodeType;

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
				typename NodeType::UnitType* newar
					= (typename NodeType::UnitType*)std::realloc(
						m_ar, mm * sizeof( typename NodeType::UnitType));
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

		void checkAddress( const NodeAddress& addr) const
		{
			if ((int)((addr >> NodeClass::Shift) & NodeClass::Mask)
			!=  (int)NodeType::ThisNodeClass)
			{
				throw std::logic_error( "accessing block with illegal node address");
			}
		}
		void checkAccess( const std::size_t& idx) const
		{
			if (idx >= m_size)
			{
				throw std::logic_error( "accessing block with node index out of range");
			}
		}

		typename NodeType::Iterator nodeIterator( const NodeAddress& addr)
		{
			checkAddress( addr);
			NodeIndex idx = nodeIndex( addr);
			checkAccess( idx);
			return typename NodeType::Iterator( m_ar[ idx]);
		}

		typename NodeType::UnitType& operator[]( std::size_t idx)
		{
			checkAccess( idx);
			return m_ar[ idx];
		}

		const typename NodeType::UnitType& operator[]( std::size_t idx) const
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
		typename NodeType::UnitType* m_ar;
		std::size_t m_size;
		std::size_t m_allocsize;
		uint32_t m_freelist;
	};

	template <class DestNodeType, class SourceNodeType>
	NodeAddress moveBlock( Block<DestNodeType>& dstblk, Block<SourceNodeType>& srcblk, NodeAddress srcaddr)
	{
		NodeIndex dstidx = dstblk.allocNode();
		typename SourceNodeType::Iterator srcitr = srcblk.nodeIterator( srcaddr);
		typename DestNodeType::UnitType& dstunit = dstblk[ dstidx];
		for (; !srcitr.end(); ++srcitr)
		{
			if (!DestNodeType::addNode(
				dstunit, srcitr.lexem(), srcitr.address()))
			{
				throw std::runtime_error( "destination node does not have the capacity of the source node");
			}
		}
		NodeIndex srcidx = nodeIndex( srcaddr);
		srcblk.releaseNode( srcidx);
		return nodeAddress( (NodeClass::Id)DestNodeType::ThisNodeClass, dstidx);
	}

	NodeAddress successorNodeAddress( const NodeAddress& addr, unsigned char lexem) const
	{
		NodeClass::Id classid = nodeClassId(addr);
		NodeIndex idx = nodeIndex( addr);
		switch (classid)
		{
			case NodeClass::NodeData:
				return 0;
			case NodeClass::Node1:
				return Node1::successor( m_block1[ idx], lexem);
			case NodeClass::Node2:
				return Node2::successor( m_block2[ idx], lexem);
			case NodeClass::Node4:
				return Node4::successor( m_block4[ idx], lexem);
			case NodeClass::Node8:
				return Node8::successor( m_block8[ idx], lexem);
			case NodeClass::Node16:
				return Node16::successor( m_block16[ idx], lexem);
			case NodeClass::Node256:
				return Node256::successor( m_block256[ idx], lexem);
		}
		throw std::logic_error("called successor node address on unknown block type");
	}

	/// \brief Try to add a new successor node to a node
	/// \return false, if the capacity of the parent node is too small
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
			case NodeClass::Node2:
				return Node2::addNode( m_block2[ idx], lexem, follow);
			case NodeClass::Node4:
				return Node4::addNode( m_block4[ idx], lexem, follow);
			case NodeClass::Node8:
				return Node8::addNode( m_block8[ idx], lexem, follow);
			case NodeClass::Node16:
				return Node16::addNode( m_block16[ idx], lexem, follow);
			case NodeClass::Node256:
				return Node256::addNode( m_block256[ idx], lexem, follow);
		}
		throw std::logic_error("called add node on unknown block type");
	}

	void patchNodeAddress( const NodeAddress& addr, const NodeAddress& oldaddr, const NodeAddress& newaddr)
	{
		NodeClass::Id classid = nodeClassId( addr);
		NodeIndex idx = nodeIndex( addr);
		switch (classid)
		{
			case NodeClass::NodeData:
				throw std::logic_error("called patch node address on data block type");
			case NodeClass::Node1:
				Node1::patchNodeAddress( m_block1[ idx], oldaddr, newaddr);
			case NodeClass::Node2:
				Node2::patchNodeAddress( m_block2[ idx], oldaddr, newaddr);
			case NodeClass::Node4:
				Node4::patchNodeAddress( m_block4[ idx], oldaddr, newaddr);
			case NodeClass::Node8:
				Node8::patchNodeAddress( m_block8[ idx], oldaddr, newaddr);
			case NodeClass::Node16:
				Node16::patchNodeAddress( m_block16[ idx], oldaddr, newaddr);
			case NodeClass::Node256:
				Node256::patchNodeAddress( m_block256[ idx], oldaddr, newaddr);
		}
		throw std::logic_error("called patch node address on unknown block type");
	}

	/// \brief Add a node and eventually replace it with an expanded node, if the capacity is too small to hold the new successor node
	void addNodeExpand( const NodeAddress& parentaddr, NodeAddress& addr, unsigned char lexem, const NodeAddress& followaddr)
	{
		if (!addNode( addr, lexem, followaddr))
		{
			NodeAddress newaddr = expandNode( addr);
			if (parentaddr)
			{
				patchNodeAddress( parentaddr, addr, newaddr);
			}
			addr = newaddr;
			if (!addNode( addr, lexem, followaddr))
			{
				throw std::logic_error( "add node failed even after expand node");
			}
		}
	}

	void unlinkNode( const NodeAddress& addr, unsigned char lexem)
	{
		NodeClass::Id classid = nodeClassId( addr);
		NodeIndex idx = nodeIndex( addr);
		switch (classid)
		{
			case NodeClass::NodeData:
				return;
			case NodeClass::Node1:
				Node1::unlink( m_block1[ idx], lexem);
				break;
			case NodeClass::Node2:
				Node2::unlink( m_block2[ idx], lexem);
				break;
			case NodeClass::Node4:
				Node4::unlink( m_block4[ idx], lexem);
				break;
			case NodeClass::Node8:
				Node8::unlink( m_block8[ idx], lexem);
				break;
			case NodeClass::Node16:
				Node16::unlink( m_block16[ idx], lexem);
				break;
			case NodeClass::Node256:
				Node256::unlink( m_block256[ idx], lexem);
				break;
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
				return moveBlock( m_block2, m_block1, addr);
			case NodeClass::Node2:
				return moveBlock( m_block4, m_block2, addr);
			case NodeClass::Node4:
				return moveBlock( m_block8, m_block4, addr);
			case NodeClass::Node8:
				return moveBlock( m_block16, m_block8, addr);
			case NodeClass::Node16:
				return moveBlock( m_block256, m_block16, addr);
			case NodeClass::Node256:
				throw std::logic_error("called expand block on block with nodes having maximum capacity");
		}
		throw std::logic_error("called expand block on unknown block type");
	}

	void addTail( const NodeAddress& parentaddr, NodeAddress& addr, const unsigned char* tail, const NodeData& data)
	{
		if (!*tail)
		{
			NodeIndex idx = m_datablock.size();
			NodeAddress followaddr = nodeAddress( NodeClass::NodeData, idx);
			m_datablock.push_back( data);
			
			addNodeExpand( parentaddr, addr, 0xFF, followaddr);
		}
		else
		{
			NodeIndex aa = addr, prev_aa = parentaddr;
			unsigned char const* ti = tail;
			for (;ti[1]; ++ti)
			{
				NodeIndex idx = m_block1.allocNode( false);
				// ... do not use free list for tail blocks to put
				//	them close to each other in memory
				NodeAddress followaddr = nodeAddress( NodeClass::Node1, idx);

				if (aa == addr)
				{
					addNodeExpand( prev_aa, aa, *ti, followaddr);
					addr = aa;
				}
				else
				{
					addNodeExpand( prev_aa, aa, *ti, followaddr);
				}
				prev_aa = aa;
				aa = followaddr;
			}
			NodeIndex idx = m_datablock.size();
			NodeAddress followaddr = nodeAddress( NodeClass::NodeData, idx);
			m_datablock.push_back( data);

			if (aa == addr)
			{
				addNodeExpand( prev_aa, aa, *ti, followaddr);
				addr = aa;
			}
			else
			{
				addNodeExpand( prev_aa, aa, *ti, followaddr);
			}
		}
	}

	template <class NODETYPE>
	bool extractNodeInfo(
			const Block<NODETYPE>& block,
			NodeIndex& addridx, std::size_t itridx,
			unsigned char& lexem, NodeAddress& follow) const
	{
		if (itridx >= (std::size_t)NODETYPE::NodeSize) return false;
		typename NODETYPE::Iterator itr( &block[ addridx], itridx);
		lexem = itr.lexem();
		follow = itr.address();
		return true;
	}

	bool getFollowNode( const NodeAddress& addr, std::size_t itridx, unsigned char& lexem, NodeAddress& follow) const
	{
		NodeIndex addridx = nodeIndex( addr);
	
		switch (nodeClassId( addr))
		{
			case NodeClass::NodeData:
				return false; 

			case NodeClass::Node1:
				return extractNodeInfo( 
					m_block1, addridx, itridx, lexem, follow);

			case NodeClass::Node2:
				return extractNodeInfo( 
					m_block2, addridx, itridx, lexem, follow);

			case NodeClass::Node4:
				return extractNodeInfo( 
					m_block4, addridx, itridx, lexem, follow);

			case NodeClass::Node8:
				return extractNodeInfo( 
					m_block8, addridx, itridx, lexem, follow);

			case NodeClass::Node16:
				return extractNodeInfo( 
					m_block16, addridx, itridx, lexem, follow);

			case NodeClass::Node256:
				return extractNodeInfo( 
					m_block256, addridx, itridx, lexem, follow);
		}
		throw std::logic_error("called get follow node on unknown block type");
	}

	template <class NODETYPE>
	bool nextNodeSibling(
			const Block<NODETYPE>& block,
			NodeIndex& addridx, std::size_t& itridx) const
	{
		if (itridx >= (std::size_t)NODETYPE::NodeSize) return false;
		typename NODETYPE::Iterator itr( &block[ addridx], itridx);
		++itr;
		itridx = itr.index();
		return (itr.lexem() != 0);
	}

	bool getNextNode( const NodeAddress& addr, std::size_t& itridx) const
	{
		NodeIndex addridx = nodeIndex( addr);
	
		switch (nodeClassId( addr))
		{
			case NodeClass::NodeData:
				return false; 

			case NodeClass::Node1:
				return false;

			case NodeClass::Node2:
				return nextNodeSibling( m_block2, addridx, itridx);

			case NodeClass::Node4:
				return nextNodeSibling( m_block4, addridx, itridx);

			case NodeClass::Node8:
				return nextNodeSibling( m_block8, addridx, itridx);

			case NodeClass::Node16:
				return nextNodeSibling( m_block16, addridx, itridx);

			case NodeClass::Node256:
				return nextNodeSibling( m_block256, addridx, itridx);
		}
		throw std::logic_error("called get next node on unknown block type");
	}

private:
	void printNode( std::ostream& out, NodeAddress addr, const std::string& indent)
	{
		std::size_t itridx = 0;
		unsigned char lexem;
		NodeAddress follow;

		do
		{
			if (getFollowNode( addr, itridx, lexem, follow))
			{
				out << indent;
				printLexem( out, lexem);
				out << " (";
				printNodeAddress( out, follow);
				out << ")";
				if (nodeClassId( follow) == NodeClass::NodeData)
				{
					out << "->" << m_datablock[ nodeIndex( follow)] << std::endl;
				}
				else
				{
					out << ":" << std::endl;
					printNode( out, follow, indent + std::string( 3, ' '));
				}
			}
		}
		while (getNextNode( addr, itridx));
	}
	
private:
	std::vector<NodeData> m_datablock;
	Block<Node1> m_block1;
	Block<Node2> m_block2;
	Block<Node4> m_block4;
	Block<Node8> m_block8;
	Block<Node16> m_block16;
	Block<Node256> m_block256;
	NodeAddress m_rootaddr;
};

}//namespace
#endif

