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
		:m_rootaddr(0)
	{
		m_datablock.push_back(0);
		//... address 0 is reserved for NULL
	}

	void set( const char* key, const NodeData& data);

	bool find( const char* key, NodeData& val) const;

	class const_iterator
	{
	public:
		const_iterator()
			:m_tree(0)
		{}
		const_iterator( const VarSizeNodeTree* tree_, NodeAddress rootaddr);

		const_iterator( const const_iterator& o);

		const std::string& key() const				{return m_key;}
		const NodeData& data() const				{return m_data;}

		bool operator==( const const_iterator& o) const		{return isequal(o);}
		bool operator!=( const const_iterator& o) const		{return !isequal(o);}

		const_iterator& operator++()				{skip(); return *this;}
		const_iterator operator++(int)				{const_iterator rt(*this); skip(); return rt;}

	private:
		void extractData();
		void skipNode();
		void skip();

	private:
		bool isequal( const const_iterator& o) const;
		void printState( std::ostream& out) const;

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

	void clear();

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
		if (addr)
		{
			out << NodeClass::idName( nodeClassId( addr)) << ":" << nodeIndex(addr);
		}
		else
		{
			out << "NULL";
		}
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
			m_idx = idx_;
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

		static bool getNodeBranch(
			const UnitType& unit, std::size_t idx,
			unsigned char& lexem_, NodeAddress& follow_)
		{
			if (idx == 0)
			{
				lexem_ = lexem(unit);
				follow_ = address(unit);
			}
			else
			{
				lexem_ = 0;
				follow_ = 0;
			}
			return 0!=lexem_;
		}

		static bool nextBranch( const UnitType&, std::size_t& idx)
		{
			if (!idx) ++idx;
			return false;
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

		static void patchNodeAddress(
				UnitType& unit, unsigned char chr,
				const NodeAddress& oldaddr, const NodeAddress& newaddr)
		{
			if (lexem( unit) != chr) throw std::logic_error( "illegal patch operation (lexem does not match)");
			if (address( unit) != oldaddr) throw std::logic_error( "illegal patch operation (previous address does not match)");
			unit = newaddr | (lexem(unit) << LexemShift);
		}
	};

	static NodeAddress unpackAddress3( const unsigned char* ptr)
	{
		return    ((uint32_t)(ptr[0] & 0xE0) << (NodeClass::Shift-5))
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

		static bool getNodeBranch(
			const UnitType& unit, std::size_t idx,
			unsigned char& lexem_, NodeAddress& follow_)
		{
			if (idx < NN)
			{
				lexem_ = unit.succ[ idx];
				follow_ = unpackAddress3( unit.node + idx*3);
			}
			else
			{
				lexem_ = 0;
				follow_ = 0;
			}
			return 0!=lexem_;
		}

		static bool nextBranch( const UnitType&, std::size_t& idx)
		{
			if (idx < NN) ++idx;
			return (idx < NN);
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

		static void patchNodeAddress(
				UnitType& unit, unsigned char chr,
				const NodeAddress& oldaddr, const NodeAddress& newaddr)
		{
			const unsigned char* nn = (const unsigned char*)std::memchr( unit.succ, chr, sizeof(unit.succ));
			if (!nn) throw std::logic_error( "illegal patch address operation (lexem not found)");
			unsigned int ofs = (nn-unit.succ)*3;
			NodeAddress oa = unpackAddress3( unit.node + ofs);
			if (oa != oldaddr) throw std::logic_error( "illegal patch address operation (old address does not match)");
			packAddress3( unit.node + ofs, newaddr);
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

		static void copy( typename NodeN<NN>::UnitType& dst, const typename NodeN<NN/2>::UnitType& src)
		{
			std::memcpy( dst.node, src.node, (NN/2)*3);
			std::memcpy( dst.succ, src.succ, (NN/2));
		}
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

		static void copy( Node2::UnitType& dst, const Node1::UnitType& src)
		{
			packAddress3( dst.node, Node1::address( src));
			dst.succ[ 0] = Node1::lexem( src);
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

		static bool empty( const UnitType& unit, unsigned int idx)
		{
			unsigned int ofs = idx * 3;
			return 0==(unit.node[ofs]|unit.node[ofs+1]|unit.node[ofs+2]);
		}

		static NodeAddress successor( const UnitType& unit, unsigned char chr)
		{
			unsigned int ofs = chr*3;
			return unpackAddress3( unit.node + ofs);
		}

		static bool getNodeBranch(
			const UnitType& unit, std::size_t idx,
			unsigned char& lexem_, NodeAddress& follow_)
		{
			follow_ = unpackAddress3( unit.node + idx*3);
			lexem_ = follow_?(unsigned char)idx:0;
			return 0!=lexem_;
		}

		static bool nextBranch( const UnitType& unit, std::size_t& idx)
		{
			if (idx < 256)
			{
				for (++idx; idx<256 && empty( unit, idx); ++idx){}
			}
			return (idx<256);
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

		static void patchNodeAddress(
				UnitType& unit, unsigned char chr,
				const NodeAddress& oldaddr, const NodeAddress& newaddr)
		{
			std::size_t ofs = chr;
			NodeAddress oa = unpackAddress3( unit.node + ofs*3);
			if (oa != oldaddr) throw std::logic_error( "illegal patch address operation (old address not found)");
			packAddress3( unit.node + ofs*3, newaddr);
		}

		static void copy( Node256::UnitType& dst, const Node16::UnitType& src)
		{
			std::size_t si = 0;
			for (; si < 16 && src.succ[si]; ++si)
			{
				std::memcpy( dst.node + src.succ[si]*3, src.node + si*3, 3);
			}
		}
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
				unsigned int mm = m_allocsize?(m_allocsize * 2):16;
				if (mm < m_allocsize) throw std::bad_alloc();
				unsigned int bytes = mm * sizeof( m_ar[0]);
				if (bytes < mm) throw std::bad_alloc();
				typename NodeType::UnitType* newar
					= (typename NodeType::UnitType*)std::realloc(
						m_ar, bytes);
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
		NodeIndex srcidx = nodeIndex( srcaddr);
		DestNodeType::copy( dstblk[ dstidx], srcblk[ srcidx]);
		return nodeAddress( (NodeClass::Id)DestNodeType::ThisNodeClass, dstidx);
	}

	NodeAddress successorNodeAddress( const NodeAddress& addr, unsigned char lexem) const;

	/// \brief Try to add a new successor node to a node
	/// \return false, if the capacity of the parent node is too small
	bool addNode( const NodeAddress& addr, unsigned char lexem, const NodeAddress& follow);

	void patchNodeAddress( const NodeAddress& addr, unsigned char chr,
				const NodeAddress& oldaddr,
				const NodeAddress& newaddr);

	/// \brief Add a node and eventually replace it with an expanded node, if the capacity is too small to hold the new successor node
	void addNodeExpand( const NodeAddress& parentaddr,
				unsigned char parentchr, NodeAddress& addr,
				unsigned char lexem, const NodeAddress& followaddr);

	void unlinkNode( const NodeAddress& addr, unsigned char lexem);

	NodeAddress expandNode( const NodeAddress& addr);

	void addTail( const NodeAddress& parentaddr, unsigned char parentchr,
			NodeAddress& addr, const unsigned char* tail, 
			const NodeData& data);

	bool getFollowNode(
			const NodeAddress& addr, std::size_t itridx,
			unsigned char& lexem, NodeAddress& follow) const;

	bool getFirstNode( const NodeAddress& addr, std::size_t& itridx) const;
	bool getNextNode( const NodeAddress& addr, std::size_t& itridx) const;

private:
	void printNode( std::ostream& out, NodeAddress addr, const std::string& indent);
	
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

