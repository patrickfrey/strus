strus	 {#mainpage}
=====

The project strus provides some libraries for building a search engine
for information retrieval. This engine is able to evaluate structured 
queries on unstructured text as well as implenting classical information
retrieval. It is independent from the key value store database impementation.
Current database implementation is based on <a href="http://leveldb.org">levelDB</a>.
The project is hosted at <a href="https://github.com/patrickfrey/strus">github</a>.

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
     2. Unary set construction operators like the successor set <i>A+</i> of <i>A</i> defined as {(<i>d</i>,<i>p</i>) | (<i>d</i>,<i>p</i>-1) element of <i>A</i>} and the predecessor set defined as {(<i>d</i>,<i>p</i>) | (<i>d</i>,<i>p</i>+1) element of <i>A</i> and <i>p</i> &ne; 0} .
     3. N-ary set selection operators that select postings with help of context information. For example <i>within_struct</i>: get the first element in an interval that is not bigger than the defined maximum range containing at least one element of each input set without overlapping a specified delimiter token.

* <b>Weighting</b> of documents based on the feature occurrencies
  Weighting defines how documents are ranked in a search result. It is defined by weighting functions, that take an iterator on the feature occurrencies and some numeric parameters as input to calculate the weight of a document. Currently there is only BM25 defined in the core, but it is possible to define other weighting functions.

* <b>Summarization</b> (extraction of content)
  Summarization is used to extract content from matching documents. With summarization you can do various things:
     1. Extract the best matching passages of the query in a document to present it as summary of the rank to the user.
     2. Extract features close to matching passages for feature selection, categorization, clustering, query answering, etc.


Architecture
-------------

The architecture defines four components that are implemented as libraries.

* [queryeval](@ref strus::QueryEvalInterface) Query evaluation: Interpretes the query and uses the operators defined in the query processor for its execution.
* [queryproc](@ref strus::QueryProcessorInterface) Query processor: Map to access functions by name, like the set operations on feature occurrencies, the weighting functions and the summarizers to augment the results.
* [storage](@ref strus::StorageInterface) Storage: Defines the storage where the all retrievable information is stored. Implements the access of statistics and the occurrencies of the basic terms.
* [database](@ref strus::DatabaseInterface)  Key/value store database: Implements the storing and retrieval of the storage data blocks. Currently there exists only one implementation based on <a href="http://leveldb.org">levelDB</a>.




