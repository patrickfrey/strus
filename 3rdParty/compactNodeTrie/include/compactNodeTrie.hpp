/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _COMPACT_NODE_TRIE_HPP_INCLUDED
#define _COMPACT_NODE_TRIE_HPP_INCLUDED
#include <cstdlib>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstring>
#include <iostream>
#include <stdint.h>

namespace conotrie {

/// \class CompactNodeTrie
/// \brief Implements a prefix trie that represents nodes in a compacted way. 
///	Unless other compact prefix trie implementations the compaction does not focus on the tail only.
///	It tries to minimize space by storing nodes with a distinct number of successors in different
///	structures. For each of these nodes types there exist a block type that stores the nodes
///	as array of equally dimensioned structures. A node in the trie is addressed with a virtual
///	24 bit address that uses the most significant 3 bits for the node type, that identifies the
///	block where the node is stored. The least significant 21 bits are used for the offset of
///	the node in the block. This representation makes it possible to store nodes with only one
///	successor, that is in dictionary tries for european languages the far most frequent node
///	type, in only 32 bits (24 bit for the virtual node address of the successor and 8 bits for
///	the character. Values assigned to keys are stored in 32 bit integer number.
class CompactNodeTrie
{
public:
	typedef uint32_t NodeData;	///< Value type of the node
	typedef uint32_t NodeAddress;	///< Virtual address of a node
	typedef uint32_t NodeIndex;	///< Index of the node in its block
	enum {NullNodeIndex=0xFFffFFffU};

public:
	/// \brief Default constructor
	CompactNodeTrie()
		:m_rootaddr(0)
	{
		m_datablock.push_back(0);
		//... address 0 is reserved for NULL
	}

	/// \brief Insert a new unique entry with key and value or update it if it already exists
	/// \param[in] key the key of the entry (NUL terminated)
	/// \param[in] val the value assigned to the key
	/// \return true on success and false, if the insertion/update failed because a node allocation error due to the limited node address space
	bool set( const char* key, const NodeData& val);

	/// \brief Get the value of a new unique entry
	/// \param[in] key the key of the entry (NUL terminated)
	/// \param[in] val the value assigned to the key
	/// \return true, if the entry was found, false else
	bool get( const char* key, NodeData& val) const;

	/// \class const_iterator
	/// \brief Read only iterator on the trie
	class const_iterator
	{
	public:
		/// \brief Default constructor
		const_iterator()
			:m_tree(0)
		{}
		/// \brief Constructor
		/// \param[in] tree_ the trie to iterate on 
		/// \param[in] rootaddr the trie root node address of the trie
		const_iterator( const CompactNodeTrie* tree_, NodeAddress rootaddr);

		/// \brief Copy constructor
		/// \param[in] o the iterator to copy
		const_iterator( const const_iterator& o);

		/// \brief Get the key of the current entry
		/// \return the key
		const std::string& key() const				{return m_key;}
		/// \brief Get the value of the current entry
		/// \return the value
		const NodeData& data() const				{return m_data;}

		/// \brief Equality comparison
		/// \param[in] o the iterator to compare
		/// \return true if the iterators point to the same node
		bool operator==( const const_iterator& o) const		{return isequal(o);}
		/// \brief Inequality comparison
		/// \param[in] o the iterator to compare
		/// \return true if the iterators point to distinct nodes
		bool operator!=( const const_iterator& o) const		{return !isequal(o);}

		/// \brief Preincrement operator
		const_iterator& operator++()				{skip(); return *this;}
		/// \brief Postincrement operator
		const_iterator operator++(int)				{const_iterator rt(*this); skip(); return rt;}

	private:
		void extractData();			///< initialize the data of the current node
		void skipNode();			///< skip to the next current node
		void skip();				///< skip to the next trie value stored

	private:
		/// \brief Equality comparison
		/// \param[in] o the iterator to compare
		/// \return true if the iterators point to the same node
		bool isequal( const const_iterator& o) const;
		/// \brief Print the current state (tracing for debugging purposes)
		/// \param[out] out where to print the current state to
		void printState( std::ostream& out) const;

		/// \struct StackElement
		/// \brief Stack element for storing the visiting state of the ancessor nodes visited
		struct StackElement
		{
			NodeAddress m_addr;	///< virtual address of the parent of the current node
			std::size_t m_itr;	///< Index of the current node as child of its parent
			char m_chr;		///< character of the node (if defined)

			/// \brief Constructor
			StackElement( NodeAddress addr_, std::size_t itr_, char chr_=0)
				:m_addr(addr_),m_itr(itr_),m_chr(chr_){}
			/// \brief Copy constructor
			StackElement( const StackElement& o)
				:m_addr(o.m_addr),m_itr(o.m_itr),m_chr(o.m_chr){}
		};
		const CompactNodeTrie* m_tree;	///< trie visited
		std::vector<StackElement> m_stack;	///< current state stack
		std::string m_key;			///< key of the current node
		NodeData m_data;			///< data of the current node
	};

	/// \brief Get the iterator pointing to the first element stored in the trie
	/// \return the iterator
	const_iterator begin() const
	{
		return const_iterator( this, m_rootaddr);
	}
	/// \brief Get the iterator identifying the end of the trie
	/// \return the iterator
	const_iterator end() const
	{
		return const_iterator();
	}

	/// \brief Print the trie in readable for for tracing and debugging purposes
	/// \param[out] out where to print the trie to
	void print( std::ostream& out)
	{
		if (m_rootaddr) printNode( out, m_rootaddr, std::string());
	}

	/// \brief Clear the contents of the trie, releasing all memory allocated
	/// \remark This clearing function is suitable for cases where the trie is really destroyed
	void clear();

	/// \brief Clear the contents of the trie without releasing any memory allocated
	/// \remark This clearing function is suitable for cases where the trie is refilled again with content of similar dimension
	void reset();

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
			MaxNofNodes=((1<<21)-1)	///< Maximum number of nodes in a block
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
	static void printLexem( std::ostream& out, unsigned char chr);

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

		static bool replaceNode( UnitType& unit, unsigned char lexem_, const NodeAddress& addr_)
		{
			if (lexem( unit) != 0) return false;
			if ((addr_ & (LexemMask << LexemShift)) != lexem_) return false;
			unit = (addr_ & AddressMask) | ((UnitType)lexem_ << LexemShift);
			return true;
		}

		static void patchNodeAddress(
				UnitType& unit, unsigned char chr,
				const NodeAddress& oldaddr, const NodeAddress& newaddr)
		{
			if (lexem( unit) != chr)
			{
				throw std::logic_error( "illegal patch operation (lexem does not match)");
			}
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

		static bool replaceNode( UnitType& unit, unsigned char lexem_, const NodeAddress& addr_)
		{
			std::size_t ofs = 0;
			for (; ofs < NN && unit.succ[ofs] != lexem_; ++ofs){}
			if (ofs == NN) return false;
			unit.succ[ ofs] = lexem_;
			packAddress3( unit.node+(ofs*3), addr_);
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

		static bool replaceNode( UnitType& unit, unsigned char lexem, const NodeAddress& addr)
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
	class BlockBase
	{
	public:
		explicit BlockBase( std::size_t elemsize_)
			:m_ar(0),m_elemsize(elemsize_),m_size(0),m_allocsize(0),m_freelist(0){}
		~BlockBase();

		void* blockPtr( std::size_t idx) const
		{
			return (char*)m_ar + (idx * m_elemsize);
		}

		NodeIndex allocNode( bool useFreeList=true);

		void checkAccess( const std::size_t& idx) const
		{
			if (idx >= m_size)
			{
				throw std::logic_error( "accessing block with node index out of range");
			}
		}

		void releaseNode( const NodeIndex& idx);

		void clear();

		void reset();

		NodeIndex spaceLeft() const
		{
			return NodeClass::MaxNofNodes - m_size;
		}

	private:
		void* m_ar;
		std::size_t m_elemsize;
		std::size_t m_size;
		std::size_t m_allocsize;
		uint32_t m_freelist;
	};

	template <class NODETYPE>
	class Block
		:public BlockBase
	{
	public:
		typedef NODETYPE NodeType;

	public:
		Block()
			:BlockBase(sizeof(typename NodeType::UnitType)){}

		~Block(){}

		void checkAddress( const NodeAddress& addr) const
		{
			if ((int)((addr >> NodeClass::Shift) & NodeClass::Mask)
				!=  (int)NodeType::ThisNodeClass)
			{
				throw std::logic_error( "accessing block with illegal node address");
			}
		}

		typename NodeType::UnitType& operator[]( std::size_t idx)
		{
			checkAccess( idx);
			return *(typename NodeType::UnitType*)blockPtr(idx);
		}

		const typename NodeType::UnitType& operator[]( std::size_t idx) const
		{
			checkAccess( idx);
			return *(typename NodeType::UnitType*)blockPtr(idx);
		}
	};

	template <class DestNodeType, class SourceNodeType>
	NodeAddress moveBlock( Block<DestNodeType>& dstblk, Block<SourceNodeType>& srcblk, NodeAddress srcaddr)
	{
		NodeIndex dstidx = dstblk.allocNode();
		if (dstidx == NullNodeIndex) return 0;
		NodeIndex srcidx = nodeIndex( srcaddr);
		DestNodeType::copy( dstblk[ dstidx], srcblk[ srcidx]);
		srcblk.releaseNode( srcidx);
		return nodeAddress( (NodeClass::Id)DestNodeType::ThisNodeClass, dstidx);
	}

	NodeAddress successorNodeAddress( const NodeAddress& addr, unsigned char lexem) const;

	/// \brief Try to add a new successor node to a node
	/// \return false, if the capacity of the parent node is too small
	bool addNode( const NodeAddress& addr, unsigned char lexem, const NodeAddress& follow);

	bool replaceNode( const NodeAddress& addr, unsigned char lexem, const NodeAddress& follow);

	void patchNodeAddress( const NodeAddress& addr, unsigned char chr,
				const NodeAddress& oldaddr,
				const NodeAddress& newaddr);

	/// \brief Add a node and eventually replace it with an expanded node, if the capacity is too small to hold the new successor node
	bool addNodeExpand( const NodeAddress& parentaddr,
				unsigned char parentchr, NodeAddress& addr,
				unsigned char lexem, const NodeAddress& followaddr);

	void unlinkNode( const NodeAddress& addr, unsigned char lexem);

	NodeAddress expandNode( const NodeAddress& addr);

	bool addTail( const NodeAddress& parentaddr, unsigned char parentchr,
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

