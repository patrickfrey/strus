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
#include "queryEval.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/constants.hpp"
#include "parser/lexems.hpp"
#include "parser/selectorSet.hpp"
#include "postingIteratorReference.hpp"
#include "accumulator.hpp"
#include "ranker.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;
using namespace strus::parser;

static std::string errorPosition( const char* base, const char* itr)
{
	unsigned int line = 1;
	unsigned int col = 1;
	std::ostringstream msg;

	for (unsigned int ii=0,nn=itr-base; ii < nn; ++ii)
	{
		if (base[ii] == '\n')
		{
			col = 1;
			++line;
		}
		else
		{
			++col;
		}
	}
	msg << "at line " << line << " column " << col;
	return msg.str();
}



void QueryEval::parseJoinOperationDef( char const*& src)
{
	int result = m_setnamemap.get( parse_IDENTIFIER( src));
	if (!isAlpha( *src) || !isEqual( parse_IDENTIFIER( src), "FOREACH"))
	{
		throw std::runtime_error("FOREACH expected after INTO and the destination set identifier of the join");
	}
	int selector = SelectorExpression::parse( src, m_selectors, m_setnamemap);
	if (!isAlpha( *src) || !isEqual( parse_IDENTIFIER( src), "DO"))
	{
		throw std::runtime_error( "DO expected after FOREACH and the selector expression");
	}
	int function = JoinFunction::parse( src, m_functions);
	if (!isSemiColon(*src))
	{
		throw std::runtime_error( "semicolon expected after a feature join operation definition");
	}
	parse_OPERATOR( src);
	m_operations.push_back( JoinOperation( result, function, selector));
}

void QueryEval::parseAccumulatorDef( char const*& src)
{
	if (m_accumulateOperation.defined())
	{
		throw std::runtime_error("duplicate definition of accumulator in query evaluation program");
	}
	m_accumulateOperation.parse( src, m_setnamemap);
	if (!isSemiColon(*src))
	{
		throw std::runtime_error( "missing semicolon ';' after EVAL expression");
	}
	parse_OPERATOR( src);
}

void QueryEval::parseSummarizeDef( char const*& src)
{
	parser::SummarizeOperation summarizer;
	summarizer.parse( src, m_setnamemap);
	if (!isSemiColon(*src))
	{
		throw std::runtime_error( "missing semicolon ';' after SUMMARIZE expression");
	}
	parse_OPERATOR( src);
	m_summarizers.push_back( summarizer);
}

void QueryEval::parseTermDef( char const*& src)
{
	if (isAlpha(*src))
	{
		std::string termset = parse_IDENTIFIER( src);
		std::string termvalue;
		std::string termtype;

		if (isStringQuote( *src))
		{
			termvalue = parse_STRING( src);
		}
		else if (isAlpha( *src))
		{
			termvalue = parse_IDENTIFIER( src);
		}
		else
		{
			throw std::runtime_error( "term value (string,identifier,number) after the feature group identifier");
		}
		if (!isColon( *src))
		{
			throw std::runtime_error( "colon (':') expected after term value");
		}
		parse_OPERATOR(src);
		if (!isAlpha( *src))
		{
			throw std::runtime_error( "term type identifier expected after colon and term value");
		}
		termtype = parse_IDENTIFIER( src);
		if (!isSemiColon( *src))
		{
			throw std::runtime_error( "semicolon expected after a feature declaration in the query");
		}
		parse_OPERATOR( src);
		m_predefinedTerms.push_back( queryeval::Query::Term( termset, termtype, termvalue));
	}
	else
	{
		throw std::runtime_error( "feature set identifier expected as start of a term declaration in the query");
	}
}

QueryEval::QueryEval( const std::string& source)
{
	char const* src = source.c_str();
	enum StatementKeyword {e_INTO, e_EVAL, e_TERM, e_SUMMARIZE};
	std::string id;

	skipSpaces( src);
	try
	{
		while (*src)
		{
			switch ((StatementKeyword)parse_KEYWORD( src, 4, "INTO", "EVAL", "TERM", "SUMMARIZE"))
			{
				case e_TERM:
					parseTermDef( src);
					break;
				case e_INTO:
					parseJoinOperationDef( src);
					break;
				case e_EVAL:
					parseAccumulatorDef( src);
					break;
				case e_SUMMARIZE:
					parseSummarizeDef( src);
					break;
			}
		}
	}
	catch (const std::runtime_error& e)
	{
		throw std::runtime_error(
			std::string( "error in query evaluation program ")
			+ errorPosition( source.c_str(), src)
			+ ":" + e.what());
	}
}


class QueryStruct
{
public:
	explicit QueryStruct( const StringIndexMap* setnamemap_)
		:m_setnamemap(setnamemap_){}

	void pushFeature( int termset, const PostingIteratorReference& itr)
	{
		if (termset <= 0) throw std::runtime_error( "internal: term set identifier out of range");
		while ((std::size_t)termset > m_iteratorSets.size())
		{
			m_iteratorSets.push_back( std::vector< PostingIteratorReference >());
		}
		m_iteratorSets[ termset-1].push_back( itr);
		if ((m_setSizeMap[ termset] += 1) > (int)QueryEval::MaxSizeFeatureSet)
		{
			throw std::runtime_error( std::string( "query term set '") + m_setnamemap->name( termset) + "' is getting too complex");
		}
	}

	const PostingIteratorInterface* getFeature( int setIndex, std::size_t elemIndex)
	{
		const std::vector<PostingIteratorReference>& iset = getFeatureSet( setIndex);
		if (iset.size() <= elemIndex)
		{
			throw std::runtime_error( std::string( "referencing feature '") + m_setnamemap->name( setIndex) + "' not defined yet completely (features have to be defined completely before referencing them in the program)");
		}
		return iset[ elemIndex].get();
	}

	const std::vector<PostingIteratorReference>& getFeatureSet( int setIndex) const
	{
		if (setIndex <= 0)
		{
			throw std::runtime_error( "internal: feature address out of range");
		}
		if ((std::size_t)(unsigned int)setIndex > m_iteratorSets.size())
		{
			throw std::runtime_error( std::string( "referencing feature '") + m_setnamemap->name( setIndex) + "' not defined yet (features have to be defined before referencing them in the program)");
		}
		return m_iteratorSets[ setIndex-1];
	}

	static bool isRelevantSelectionFeature( const StorageInterface& storage, PostingIteratorInterface& itr)
	{
		float nofMatches = itr.documentFrequency();
		float nofCollectionDocuments = storage.nofDocumentsInserted();
	
		if (nofCollectionDocuments < 50 || nofCollectionDocuments > nofMatches * 2)
		{
			return true;
		}
		return false;
	}

	PostingIteratorReference getFeatureSetUnion( const StorageInterface& storage, const QueryProcessorInterface& processor, int setIndex, bool relevantOnly)
	{
		const PostingIteratorInterface* far[ (int)QueryEval::MaxSizeFeatureSet];

		if (setIndex <= 0)
		{
			throw std::runtime_error( "internal: feature address is NULL");
		}
		if (setIndex <= 0 || (std::size_t)(unsigned int)setIndex > m_iteratorSets.size())
		{
			throw std::runtime_error( std::string( "feature '") + m_setnamemap->name(setIndex) + "' is undefined");
		}
		std::vector<PostingIteratorReference>& feats = m_iteratorSets[ setIndex-1];
		if (feats.size() > (int)QueryEval::MaxSizeFeatureSet)
		{
			throw std::runtime_error( "number of features in selection set is too big");
		}
		std::vector<PostingIteratorReference>::const_iterator ai = feats.begin(), ae = feats.end();
		std::size_t aidx = 0;
		for (; ai != ae; ai++)
		{
			if (ai->get() && (!relevantOnly || isRelevantSelectionFeature( storage, **ai)))
			{
				far[ aidx++] = ai->get();
			}
		}
		if (aidx)
		{
			PostingIteratorReference rt(
				processor.createJoinPostingIterator(
					Constants::operator_set_union(), 0, aidx, far));
			return rt;
		}
		else
		{
			return PostingIteratorReference();
		}
	}

	const std::map<int,int> setSizeMap() const
	{
		return m_setSizeMap;
	}

	int setSize( int setIndex) const
	{
		std::map<int,int>::const_iterator gi = m_setSizeMap.find( setIndex);
		return (gi == m_setSizeMap.end())?0:gi->second;
	}

	void printFeatures( std::ostream& out)
	{
		std::vector< std::vector<PostingIteratorReference> >::const_iterator
			vi = m_iteratorSets.begin(), ve = m_iteratorSets.end();
		for (int vidx=0; vi != ve; ++vi,++vidx)
		{
			std::vector<PostingIteratorReference>::const_iterator fi = vi->begin(), fe = vi->end();
			for (; fi != fe; ++fi)
			{
				if (fi->get())
				{
					out << "[" << vidx << "] '" << (*fi)->featureid() << "'" << std::endl;
				}
			}
		}
	}

private:
	std::vector< std::vector<PostingIteratorReference> > m_iteratorSets;
	const StringIndexMap* m_setnamemap;
	std::map<int,int> m_setSizeMap;
};


void QueryEval::print( std::ostream& out) const
{
	std::vector<queryeval::Query::Term>::const_iterator ti = predefinedTerms().begin(), te = predefinedTerms().end();
	for (; ti != te; ++ti)
	{
		out << "TERM " << ti->set << ": " << ti->type << " '" << ti->value << "';" << std::endl;
	}
	std::vector<parser::JoinOperation>::const_iterator ji = m_operations.begin(), je = m_operations.end();
	for (; ji != je; ++ji)
	{
		out << "INTO " << m_setnamemap.name( ji->result())
			<< " FOREACH ";
		SelectorExpression::print( out, ji->selector(), m_selectors, m_setnamemap);

		out << " DO ";
		JoinFunction::print( out, ji->function(), m_functions);
		out << ";" << std::endl;
	}
	if (m_accumulateOperation.defined())
	{
		out << "EVAL ";
		m_accumulateOperation.print( out, m_setnamemap);
		out << ";" << std::endl;
	}
	std::vector<parser::SummarizeOperation>::const_iterator si = m_summarizers.begin(), se = m_summarizers.end();
	for (; si != se; ++si)
	{
		out << "SUMMARIZE ";
		si->print( out, m_setnamemap);
		out << ";" << std::endl;
	}
}


std::vector<queryeval::ResultDocument>
	QueryEval::getRankedDocumentList(
			Accumulator& accu,
			const std::vector<SummarizerDef>& summarizers,
			std::size_t firstRank,
			std::size_t maxNofRanks) const
{
	std::vector<queryeval::ResultDocument> rt;
	Ranker ranker( maxNofRanks);

	Index docno = 0;
	unsigned int state = 0;
	unsigned int prev_state = 0;
	float weight = 0.0;

	while (accu.nextRank( docno, state, weight))
	{
		ranker.insert( queryeval::WeightedDocument( docno, weight));
		if (state > prev_state && ranker.nofRanks() >= maxNofRanks)
		{
			break;
		}
		prev_state = state;
	}
	std::vector<queryeval::WeightedDocument> resultlist = ranker.result( firstRank);
	std::vector<queryeval::WeightedDocument>::const_iterator
		ri=resultlist.begin(),re=resultlist.end();

	for (; ri != re; ++ri)
	{
		std::vector<queryeval::ResultDocument::Attribute> attr;
		std::vector<SummarizerDef>::const_iterator
			si = summarizers.begin(), se = summarizers.end();
		for (; si != se; ++si)
		{
			std::vector<std::string> summary = si->second->getSummary( ri->docno());
			std::vector<std::string>::const_iterator
				ci = summary.begin(), ce = summary.end();
			for (; ci != ce; ++ci)
			{
				attr.push_back(
					queryeval::ResultDocument::Attribute(
						si->first, *ci));
			}
		}
		rt.push_back( queryeval::ResultDocument( *ri, attr));
	}
	return rt;
}

std::vector<queryeval::ResultDocument>
	QueryEval::getRankedDocumentList(
		const StorageInterface& storage,
		const QueryProcessorInterface& processor,
		const queryeval::Query& query_,
		std::size_t fromRank,
		std::size_t maxNofRanks) const
{
	QueryStruct query( &m_setnamemap);

#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "Start evaluating query:" << std::endl;
	print( std::cout);
	std::cout << std::endl;
#endif
	//[1] Create the initial feature sets:
	{
		// Process the query features (explicit join operations)
		typedef std::pair<std::string, PostingIteratorReference> FeatDef;
		std::vector<FeatDef> feats;

		std::vector<queryeval::Query::Term>::const_iterator ti = query_.termar().begin(), te = query_.termar().end();
		std::vector<queryeval::Query::JoinOp>::const_iterator ji = query_.joinar().begin(), je = query_.joinar().end();

		for (std::size_t tidx=0; ti != te; ++ti,++tidx)
		{
			feats.push_back( FeatDef( ti->set, processor.createTermPostingIterator( ti->type, ti->value)));

			for (; ji != je && ji->termcnt == tidx; ++ji)
			{
				enum {MaxNofJoinopArguments=256};
				if (ji->nofArgs > MaxNofJoinopArguments || ji->nofArgs > feats.size())
				{
					throw std::runtime_error( "number of arguments of explicit join in query out of range");
				}
				const PostingIteratorInterface* joinargs[ MaxNofJoinopArguments];
				std::size_t ii=0;
				for (;ii < ji->nofArgs; ++ii)
				{
					joinargs[ ji->nofArgs-ii-1] = feats[ feats.size()-ii-1].second.get();
				}
				PostingIteratorReference res(
					processor.createJoinPostingIterator(
						ji->opname, ji->range,
						ji->nofArgs, joinargs));

				feats.resize( feats.size() - ji->nofArgs);
				feats.push_back( FeatDef( ji->set, res));
			}
		}

		// Add add the processed query features to the initial feature set:
		std::vector<FeatDef>::const_iterator fi = feats.begin(), fe = feats.end();
		for (; fi != fe; ++fi)
		{
			StringIndexMap::const_iterator si = m_setnamemap.find( fi->first);
			if (si == m_setnamemap.end())
			{
				throw std::runtime_error( std::string( "term set identifier '") + fi->first + "' not used in this query program");
			}
			query.pushFeature( si->second, fi->second);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "Create query feature [" << si->second << "] " << fi->first << " :" << query.setSize(si->second) << std::endl;
#endif
		}

		// Add predefined terms to the initial feature set:
		std::vector<queryeval::Query::Term>::const_iterator pi = m_predefinedTerms.begin(), pe = m_predefinedTerms.end();
		for (; pi != pe; ++pi)
		{
			StringIndexMap::const_iterator si = m_setnamemap.find( pi->set);
			if (si != m_setnamemap.end())
			{
				query.pushFeature( si->second, processor.createTermPostingIterator( pi->type, pi->value));
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "create predefined feature [" << si->second << "] " << pi->set << " :" << query.setSize(si->second) << std::endl;
#endif
			}
		}
	}

	//[2] Iterate on all join operations and create the combined features:
	std::vector<parser::JoinOperation>::const_iterator ji = m_operations.begin(), je = m_operations.end();
	for (; ji != je; ++ji)
	{
		const parser::JoinFunction& function = functions()[ ji->function()-1];
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "calculate join ";
		function.print( std::cout, ji->function(), functions());
		std::cout << " of ";
		SelectorExpression::print( std::cout, ji->selector(), selectors(), m_setnamemap);
		std::cout << std::endl;
#endif
		SelectorSetR selset(
			SelectorSet::calculate(
				ji->selector(), selectors(), query.setSizeMap()));

		if (selset.get())
		{
			const std::vector<Selector>& selar = selset->ar();
			std::size_t ri = 0, re = selar.size(), ro = selset->rowsize();
			enum {MaxNofSelectorColumns=256};
			if (ro > MaxNofSelectorColumns)
			{
				throw std::runtime_error("query too complex (number of rows in selection has more than 256 elements");
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "create feature " << m_setnamemap.name( ji->result()) << " from " << selset->tostring() << std::endl;
#endif
			const PostingIteratorInterface* joinargs[ MaxNofSelectorColumns];
			for (; ri < re; ri += ro)
			{
				std::size_t ci = 0, ce = ro;
				for (; ci < ce; ++ci)
				{
					const Selector& sel = selar[ ri+ci];
					joinargs[ ci] = query.getFeature( sel.setIndex, sel.elemIndex);
				}
				PostingIteratorInterface* opres =
					processor.createJoinPostingIterator(
						function.name(), function.range(), ro, joinargs);
				query.pushFeature( ji->result(), opres);
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "feature " << m_setnamemap.name( ji->result()) << " has set size " << query.setSize(ji->result()) << std::endl;
#endif
		}
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "query features:" << std::endl;
	query.printFeatures( std::cout);
#endif

	//[3] Get the result accumulator and evaluate the results
	if (m_accumulateOperation.defined())
	{
		// Create the accumulator:
		Accumulator accumulator( &processor, maxNofRanks, storage.maxDocumentNumber());

		std::vector<WeightingFunction>::const_iterator
			gi = m_accumulateOperation.args().begin(),
			ge = m_accumulateOperation.args().end();
	
		for (std::size_t gidx=0; gi != ge; ++gi,++gidx)
		{
			const std::vector<PostingIteratorReference>& feats
				= query.getFeatureSet( gi->setIndex());
			std::vector<PostingIteratorReference>::const_iterator ai = feats.begin(), ae = feats.end();
			for (std::size_t aidx=0; ai != ae; ai++,aidx++)
			{
				if (ai->get())
				{
					accumulator.addRanker( gi->factor(), gi->function(), gi->params(), **ai);
				}
			}
		}
		std::vector<int>::const_iterator
			fi = m_accumulateOperation.featureSelectionSets().begin(),
			fe = m_accumulateOperation.featureSelectionSets().end();

		for (; fi != fe; ++fi)
		{
			PostingIteratorReference selection( query.getFeatureSetUnion( storage, processor, *fi, true));
			if (selection.get()) accumulator.addSelector( *selection);
		}
		// Get the summarizers:
		std::vector<SummarizerDef> summarizerdefs;
		std::vector<parser::SummarizeOperation>::const_iterator
			si = m_summarizers.begin(), se = m_summarizers.end();
		for (; si != se; ++si)
		{
			std::vector<PostingIteratorReference> far_ref;
			const PostingIteratorInterface* far[ MaxSizeFeatureSet];
			if (si->featureset().size() > MaxSizeFeatureSet)
			{
				throw std::runtime_error( "set of features in summarizer definition is too complex");
			}
			std::vector<int>::const_iterator ai = si->featureset().begin(), ae = si->featureset().end();
			for (std::size_t aidx=0; ai != ae; ++ai,++aidx)
			{
				PostingIteratorReference sumfeat( query.getFeatureSetUnion( storage, processor, *ai, true));
				far_ref.push_back( sumfeat); 
				far[ aidx] = far_ref.back().get();
			}
			PostingIteratorReference structfeat;
			if (si->structset())
			{
				structfeat = query.getFeatureSetUnion(
					storage, processor, si->structset(), false);
			}
			summarizerdefs.push_back(
				SummarizerDef(
					si->resultAttribute(),
					processor.createSummarizer(
						si->summarizerName(), si->type(),
						si->parameter(), structfeat.get(),
						far_ref.size(), far)));
		}
		// Calculate and return the ranklist:
		return getRankedDocumentList(
				accumulator, summarizerdefs,
				fromRank, maxNofRanks);
	}
	else
	{
		throw std::runtime_error("no accumulator defined (EVAL), cannot evaluate ranked document list");
	}
}

