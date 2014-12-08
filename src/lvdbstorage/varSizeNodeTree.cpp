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
#include "varSizeNodeTree.hpp"
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstring>
#include <iostream>
#include <stdint.h>

using namespace strus;

void VarSizeNodeTree::set( const char* key, const NodeData& data)
{
	if (!m_rootaddr)
	{
		m_rootaddr = nodeAddress( NodeClass::Node1, m_block1.allocNode());
	}
	NodeAddress addr = m_rootaddr;
	NodeAddress prev_addr = 0;
	unsigned char prev_chr = 0;

	unsigned char const* ki = (const unsigned char*)key;
	for (; *ki; ++ki)
	{
		NodeAddress next = successorNodeAddress( addr, *ki);
		if (!next)
		{
			if (m_rootaddr == addr)
			{
				addTail( 0, 0, addr, ki, data);
				m_rootaddr = addr;
			}
			else
			{
				addTail( prev_addr, prev_chr, addr, ki, data);
			}
			return;
		}
		else if (nodeClassId( next) == NodeClass::NodeData)
		{
			NodeIndex idx = m_block2.allocNode();
			NodeAddress newaddr = nodeAddress( NodeClass::Node2, idx);
			if (!addNode( newaddr, 0xFF, next))
			{
				throw std::logic_error("illegal state (empty addNode4 returns false)");
			}
			unlinkNode( addr, *ki);
			if (!addNode( addr, *ki, newaddr))
			{
				throw std::logic_error("illegal state (addNode after unlink returns false)");
			}
			prev_addr = addr;
			prev_chr = *ki;
			addr = newaddr;
		}
		else
		{
			prev_addr = addr;
			prev_chr = *ki;
			addr = next;
		}
	}
	if (m_rootaddr == addr)
	{
		addTail( 0, 0, addr, ki, data);
		m_rootaddr = addr;
	}
	else
	{
		addTail( prev_addr, prev_chr, addr, ki, data);
	}
}

bool VarSizeNodeTree::find( const char* key, NodeData& val) const
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

VarSizeNodeTree::const_iterator::const_iterator( const VarSizeNodeTree* tree_, NodeAddress rootaddr)
	:m_tree(tree_)
{
	m_stack.push_back( StackElement( rootaddr, 0));
	if (m_tree->getFirstNode( 
			m_stack.back().m_addr,
			m_stack.back().m_itr))
	{
		skip();
	}
	else
	{
		m_stack.pop_back();
	}
}

VarSizeNodeTree::const_iterator::const_iterator( const const_iterator& o)
	:m_tree(o.m_tree)
	,m_stack(o.m_stack)
	,m_key(o.m_key)
	,m_data(o.m_data){}

void VarSizeNodeTree::const_iterator::extractData()
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

void VarSizeNodeTree::const_iterator::skipNode()
{
	unsigned char lexem;
	NodeAddress follow;
	if (m_tree->getFollowNode(
			m_stack.back().m_addr,
			m_stack.back().m_itr,
			lexem, follow))
	{
		char chr = lexem==0xFF?0:lexem;
		std::size_t itridx = 0;
		if (m_tree->getFirstNode( follow, itridx))
		{
			m_stack.push_back( StackElement( follow, itridx, chr));
			return;
		}
	}
	for (;;)
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

void VarSizeNodeTree::const_iterator::skip()
{
	m_key.clear();
	m_data = 0;

	for (;;)
	{
		if (m_stack.empty()) return;

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

bool VarSizeNodeTree::const_iterator::isequal( const const_iterator& o) const
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

void VarSizeNodeTree::const_iterator::printState( std::ostream& out) const
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

void VarSizeNodeTree::clear()
{
	m_datablock.clear();
	m_datablock.push_back( 0);
	//... address 0 is reserved for NULL
	m_block1.clear();
	m_block2.clear();
	m_block4.clear();
	m_block8.clear();
	m_block16.clear();
	m_block256.clear();
	m_rootaddr = 0;
}

void VarSizeNodeTree::reset()
{
	m_datablock.resize( 1);
	//... address 0 is reserved for NULL
	m_block1.reset();
	m_block2.reset();
	m_block4.reset();
	m_block8.reset();
	m_block16.reset();
	m_block256.reset();
	m_rootaddr = 0;
}

VarSizeNodeTree::NodeAddress VarSizeNodeTree::successorNodeAddress( const NodeAddress& addr, unsigned char lexem) const
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

bool VarSizeNodeTree::addNode( const NodeAddress& addr, unsigned char lexem, const NodeAddress& follow)
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

void VarSizeNodeTree::patchNodeAddress( const NodeAddress& addr, unsigned char chr,
					const NodeAddress& oldaddr,
					const NodeAddress& newaddr)
{
	NodeClass::Id classid = nodeClassId( addr);
	NodeIndex idx = nodeIndex( addr);
	switch (classid)
	{
		case NodeClass::NodeData:
			throw std::logic_error("called patch node address on data block type");
		case NodeClass::Node1:
			Node1::patchNodeAddress( m_block1[ idx], chr, oldaddr, newaddr);
			return;
		case NodeClass::Node2:
			Node2::patchNodeAddress( m_block2[ idx], chr, oldaddr, newaddr);
			return;
		case NodeClass::Node4:
			Node4::patchNodeAddress( m_block4[ idx], chr, oldaddr, newaddr);
			return;
		case NodeClass::Node8:
			Node8::patchNodeAddress( m_block8[ idx], chr, oldaddr, newaddr);
			return;
		case NodeClass::Node16:
			Node16::patchNodeAddress( m_block16[ idx], chr, oldaddr, newaddr);
			return;
		case NodeClass::Node256:
			Node256::patchNodeAddress( m_block256[ idx], chr, oldaddr, newaddr);
			return;
	}
	throw std::logic_error("called patch node address on unknown block type");
}

void VarSizeNodeTree::addNodeExpand( const NodeAddress& parentaddr,
					unsigned char parentchr, NodeAddress& addr,
					unsigned char lexem, const NodeAddress& followaddr)
{
	if (!addNode( addr, lexem, followaddr))
	{
		NodeAddress newaddr = expandNode( addr);
		if (parentaddr)
		{
			patchNodeAddress( parentaddr, parentchr, addr, newaddr);
		}
		addr = newaddr;
		if (!addNode( addr, lexem, followaddr))
		{
			throw std::logic_error( "add node failed even after expand node");
		}
	}
}

void VarSizeNodeTree::unlinkNode( const NodeAddress& addr, unsigned char lexem)
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

VarSizeNodeTree::NodeAddress VarSizeNodeTree::expandNode( const NodeAddress& addr)
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

void VarSizeNodeTree::addTail( const NodeAddress& parentaddr, unsigned char parentchr,
				NodeAddress& addr, const unsigned char* tail, 
				const NodeData& data)
{
	if (!*tail)
	{
		NodeIndex idx = m_datablock.size();
		NodeAddress followaddr = nodeAddress( NodeClass::NodeData, idx);
		m_datablock.push_back( data);
		
		addNodeExpand( parentaddr, parentchr, addr, 0xFF, followaddr);
	}
	else
	{
		NodeIndex aa = addr, prev_aa = parentaddr;
		unsigned char prev_chr = parentchr;
		unsigned char const* ti = tail;
		for (;ti[1]; ++ti)
		{
			NodeIndex idx = m_block1.allocNode( false);
			// ... do not use free list for tail blocks to put
			//	them close to each other in memory
			NodeAddress followaddr = nodeAddress( NodeClass::Node1, idx);

			if (aa == addr)
			{
				addNodeExpand( prev_aa, prev_chr, aa, *ti, followaddr);
				addr = aa;
			}
			else
			{
				addNodeExpand( prev_aa, prev_chr, aa, *ti, followaddr);
			}
			prev_chr = ti[1];
			prev_aa = aa;
			aa = followaddr;
		}
		NodeIndex idx = m_datablock.size();
		NodeAddress followaddr = nodeAddress( NodeClass::NodeData, idx);
		m_datablock.push_back( data);

		if (aa == addr)
		{
			addNodeExpand( prev_aa, prev_chr, aa, *ti, followaddr);
			addr = aa;
		}
		else
		{
			addNodeExpand( prev_aa, prev_chr, aa, *ti, followaddr);
		}
	}
}

bool VarSizeNodeTree::getFollowNode( const NodeAddress& addr, std::size_t itridx, unsigned char& lexem, NodeAddress& follow) const
{
	NodeIndex addridx = nodeIndex( addr);

	switch (nodeClassId( addr))
	{
		case NodeClass::NodeData:
			return false; 

		case NodeClass::Node1:
			return Node1::getNodeBranch(
				m_block1[ addridx], itridx, lexem, follow);
			
		case NodeClass::Node2:
			return Node2::getNodeBranch(
				m_block2[ addridx], itridx, lexem, follow);

		case NodeClass::Node4:
			return Node4::getNodeBranch(
				m_block4[ addridx], itridx, lexem, follow);

		case NodeClass::Node8:
			return Node8::getNodeBranch(
				m_block8[ addridx], itridx, lexem, follow);

		case NodeClass::Node16:
			return Node16::getNodeBranch(
				m_block16[ addridx], itridx, lexem, follow);

		case NodeClass::Node256:
			return Node256::getNodeBranch(
				m_block256[ addridx], itridx, lexem, follow);
	}
	throw std::logic_error("called get follow node on unknown block type");
}

bool VarSizeNodeTree::getFirstNode( const NodeAddress& addr, std::size_t& itridx) const
{
	NodeIndex addridx = nodeIndex( addr);
	itridx = 0;
	if (nodeClassId( addr) == NodeClass::Node256)
	{
		const Node256::UnitType& unit = m_block256[ addridx];
		while (itridx < 256 && Node256::empty( unit, itridx))
		{
			++itridx;
		}
		return itridx < 256;
	}
	return true;
}

bool VarSizeNodeTree::getNextNode( const NodeAddress& addr, std::size_t& itridx) const
{
	NodeIndex addridx = nodeIndex( addr);

	switch (nodeClassId( addr))
	{
		case NodeClass::NodeData:
			return false; 

		case NodeClass::Node1:
			return Node1::nextBranch( m_block1[ addridx], itridx);

		case NodeClass::Node2:
			return Node2::nextBranch( m_block2[ addridx], itridx);

		case NodeClass::Node4:
			return Node4::nextBranch( m_block4[ addridx], itridx);

		case NodeClass::Node8:
			return Node8::nextBranch( m_block8[ addridx], itridx);

		case NodeClass::Node16:
			return Node16::nextBranch( m_block16[ addridx], itridx);

		case NodeClass::Node256:
			return Node256::nextBranch( m_block256[ addridx], itridx);
	}
	throw std::logic_error("called get next node on unknown block type");
}

void VarSizeNodeTree::printNode( std::ostream& out, NodeAddress addr, const std::string& indent)
{
	std::size_t itridx = 0;
	unsigned char lexem;
	NodeAddress follow;

	if (getFirstNode( addr, itridx))
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

