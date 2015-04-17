strus	 {#mainpage}
=====

The project strus provides some libraries for building a search engine
for information retrieval. This engine is able to evaluate structured 
queries on unstructured text as well as implenting classical information
retrieval. It is independent from the key value store database impementation.
Current database implementationis based on [LevelDB]: http://leveldb.org/.
The project is hosted at [strus]: https://github.com/patrickfrey/strus.

strus defines the evaluation of a query based on 3 types of operations:
* <b>Fetching and joining</b> of feature occurrencies.
  The feature ocurrencies, also referred to a postings are represented as sets of pairs
     {(<i>d</i>,<i>p</i>) |  <i>d</i> is a document number, <i>p</i> is a position }, where 
     <i>d</i> and  <i>p</i> are positive integer numbers.  <i>d</i> is a unique id of the document in
     the storage while <i>p</i> is representing the term position in the document.
     These sets are built from the basic feature occurrencies of terms
     stored in the storage (See [here](@ref strus::StorageClientInterface::createTermPostingIterator)).
     Together with the set join operators provided by the [query processor interface](@ref strus::QueryProcessorInterface),
     you can build representations of more complex structures.
     The basic set join operators are the following:
     1. Basic operators of the boolean algebra of sets of (<i>d</i>, <i>p</i>) pairs: intersection, union and relative complement.
     2. Unary set construction operators like the successor set <i>A+</i> of <i>A</i> defined as {(<i>d</i>,<i>p</i>) | (<i>d</i>,<i>p</i>-1) element of <i>A</i>} and the predecessor set defined accordingly.
     3. N-ary set selection operators that select elements based on context conditions. For example <i>within_struct</i>: get the first element of every interval of a maximum size (range) containing at least one element of each input set, but not containing a specific delimiter token (like for example punctuation to filter structures crossing sentence borders).

* <b>Weighting</b> of documents based on the feature occurrencies
  Weighting defines how documents are ranked in a search result. It is defined by weighting functions, that take an iterator on the feature occurrencies and some numeric parameters as input to calculate the weight of a document. Currently there is only BM25 defined in the core, but it is simple to define other weighting functions.
  Weighting functions can access the positions of query subexpressions too. This allows to implement query evaluation schemes that take relations of features like for example relative distance into account for weighting (proximity weighting scheme).

* <b>Summarization</b> as extraction of content based on the query matches
  Summarization is used to extract content from matching documents. With summarization you can do various things:
     1. Extract the best matching passages of the query in a document to present it as summary of the rank to the user.
     2. Extract features close to matching passages for automated query rewriting (relevance feedback).
     3. Extract data close to matching passages for categorization, clustering, query answering, etc.


Architecture
-------------

The architecture defines four components that are implemented as libraries.

* [queryeval](@ref strus::QueryEvalInterface) Query evaluation: Interpretes the query and uses the operators defined in the query processor for its execution.
* [queryproc](@ref strus::QueryProcessorInterface) Query processor: Defines the set operations on feature occurrencies, the weighting functions and the summarizers to augment the results.
* [storage](@ref strus::StorageInterface) Storage: Defines the storage where the all retrievable information is stored. Implements the retrieval of occurrencies of the basic terms and the weighting the terms in case of statistical IR.
* [database](@ref strus::DatabaseInterface)  Key/value store database: Implements the storing and retrieval of the storage data blocks. Currently there exists only one implementation based on [LevelDB]: http://leveldb.org/.




