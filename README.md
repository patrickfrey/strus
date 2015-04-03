strus
=====

Library for building a search engine for information retrieval. 
Able to evaluate structured queries on unstructured text as well
as implenting classical information retrieval.
Independent from storage impementation. Current storage implementation
is based on LevelDB.

strus defines information retrieval as evaluation of a query based on 3
types of operations:
  * Fetching and joining of feature occurrencies (document/position number)
  * Weighting of documents based on the feature occurrencies
  * Summarization as extraction of content based on the query matches

Architecture
-------------

The architecture defines three components that are implemented as libraries.

  |--------------------|
  |     queryeval      |  Query evaluation: Interpretes the query
  |                    |  and uses the operators defined in the query
  |                    |  processor for its execution.
  |--------------------|
  |     queryproc      |  Query processor: Defines the set operations
  |                    |  on feature occurrencies, the weighting functions
  |                    |  and the summarizers to augment the results.
  |--------------------|
  |     storage        |  Storage: Defines the storage where the all
  |                    |  retrievable information is stored.
  |                    |  Implements the retrieval of occurrencies of
  |                    |  the atomic terms and the weighting the terms
  |                    |  in case of statistical IR.
  |--------------------|
  |     database       |  Key/value store database: Implements the storing
  |                    |  and retrieval of the storage data blocks.
  |                    |  Currently there exists only one implementation
  |                    |  based on LevelDB.
  |--------------------|




