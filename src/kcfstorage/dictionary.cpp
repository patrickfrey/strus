#include "dictionary.hpp"
#include <cstring>
#include <boost/cstdint.hpp>

typedef boost::int64_t Address;
typedef boost::int64_t NodeValue;

static NodeValue nodeValue( Address adr, unsigned char succ)
{
	return (adr << 8) + succ;
}

static Address nodeAddress( NodeValue value)
{
	return (value >> 8);
}

static unsigned char nodeChar( NodeValue value)
{
	return (unsigned char)(value & 0xff);
}


enum NodeType
{
	ValueType,
	Succ1Type,
	Succ2Type,
	Succ7Type,
	Succ14Type,
	Succ256Type
};

struct NodeValue
{
	Index val;
};

struct NodeSucc1
{
	NodeValue val;

	Address getsucc( unsigned char ch)
	{
		if (nodeChar( val) == ch) return nodeAddress( val);
		return 0;
	}

	bool addSucc( unsigned char ch, Address aa)
	{
		if (val == 0)
		{
			val = NodeValue( aa, ch);
			return true;
		}
		return false;
	}
};

struct NodeSucc2
{
	NodeValue val[2];

	Address getsucc( unsigned char ch)
	{
		if (nodeChar( val[0]) == ch) return nodeAddress( val[0]);
		if (nodeChar( val[1]) == ch) return nodeAddress( val[1]);
		return 0;
	}

	bool addSucc( unsigned char ch, Address aa)
	{
		if (val[0] == 0)
		{
			val[0] = NodeValue( aa, ch);
			return true;
		}
		if (val[1] == 0)
		{
			val[1] = NodeValue( aa, ch);
			return true;
		}
		return false;
	}
};

struct NodeSucc7
{
	char match[8];
	Address adr[7];

	Address getsucc( unsigned char ch)
	{
		//TODO use boost::SIMD operations
		const char* cc = std::strchr( match, ch);
		if (!cc) return false;
		return adr[ (cc - match)];
	}

	bool addSucc( unsigned char ch, Address aa)
	{
		//TODO use boost::SIMD operations
		const char* cc = std::strchr( match, '\0');
		if (ch - match >= 7) return false;
		adr[ ch - match] = aa;
		return true;
	}
};

struct NodeSucc14
{
	char match[16];
	Address adr[14];

	Address getsucc( unsigned char ch)
	{
		//TODO use boost::SIMD operations
		const char* cc = std::strchr( match, ch);
		if (!cc) return false;
		return adr[ (cc - match)];
	}

	bool addSucc( unsigned char ch, Address aa)
	{
		//TODO use boost::SIMD operations
		const char* cc = std::strchr( match, '\0');
		if (ch - match >= 14) return false;
		adr[ ch - match] = aa;
		return true;
	}
};

struct NodeSucc256
{
	Address adr[256];

	Address getsucc( unsigned char ch)
	{
		return adr[ ch];
	}

	bool addSucc( unsigned char ch, Address aa)
	{
		adr[ ch] = aa;
	}
};


template <class NodeType_, enum NodeClass_>
class NodeStruct
{
private:
	NodeClass_* ar;
	std::size_t arsize;
	std::size_t arpos;
public:
	!!! HIER WEITER (no freelist because of shared concurrent reader - element are freed when writing dictionary to disc)
};

struct Dictionary::Impl
{
	Impl(){}
	Impl( const Impl& o){}
	~Impl();

	void insert( const std::string& key, const Index& value);
	Index find( const std::string& key);

private:
	NodeValue* arv;
	std::size_t sizev;
	Index freev;

	NodeSucc1* ar1;
	std::size_t size1;
	Index free1;

	NodeSucc7* ar7;
	std::size_t size7;
	Index free7;

	NodeSucc14* ar14;
	std::size_t size256;
	Index free14;

	NodeSucc256* ar256;
	std::size_t size256;
	Index free256;
};

Dictionary::Dictionary()
{
	new Impl;
}

Dictionary::Dictionary( const Dictionary& o)
	:m_impl(new Impl(*o.m_impl))
{}

Dictionary::~Dictionary()
{
	delete m_impl;
}

void Dictionary::insert( const std::string& key, const Index& value)
{
	return m_impl->insert( key, value);
}

Index find( const std::string& key)
{
	return m_impl->find( key);
}


