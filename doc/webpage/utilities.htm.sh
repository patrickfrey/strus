#!/bin/sh
cat <<'!ENDBLOCK'
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 2.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
	<link rel="icon" type="image/ico" href="images/strus.ico" />
	<meta http-equiv="content-type" content="text/html; charset=utf-8" />
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<meta name="description" content="Description of the command line utility programs of strus, a collection of C++ libraries for building a full-text search engine." />
	<meta name="keywords" content="fulltext search engine C++" />
	<meta name="author" content="Patrick Frey &lt;patrickpfrey (a) yahoo (dt) com&gt;" />
	<link rel="stylesheet" type="text/css" href="text-profile.css" title="Text Profile" media="all" />
	<title>Strus utility programs</title>
</head>
<body>
<div id="wrap">
	<div id="content">
		<h1>Command line utility programs</h1>
		<p class="description">This document lists some utility programs with description,
		most of them are implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>.
		</p>
		<h2>Languages used by utility programs</h2>
		<p class="description">
		Some utility programs</a> are based on source files in a proprietary language.
		But the functionality expressed with these domain specific languages is 
		not a parallel universe. All languages map to calls of the strus core and 
		analyzer API. All loading of programs is implemented as calls of
		<a href="http://patrickfrey.github.io/strusUtilities/doc/doxygen/html/index.html">the program loader interface</a>.
		</p>
		<h3>Document analyzer program</h3>
		<p class="description">The grammar of the sources referred to as document analyzer programs 
		by some utility programs are defined <a href="grammar_analyzerprg_doc.htm">here (document analyzer program grammar)</a>.
		</p>
		<h3>Query analyzer program</h3>
		<p class="description">The grammar of the sources referred to as query analyzer programs 
		by some utility programs are defined <a href="grammar_analyzerprg_qry.htm">here (query analyzer program grammar)</a>.
		</p>
		<h3>Query evaluation program</h3>
		<p class="description">The grammar of the sources referred to as query evaluation programs 
		by some utility programs are defined <a href="grammar_queryevalprg.htm">here (query evaluation program grammar)</a>.
		</p>
		<h3>Query language</h3>
		<p class="description">The language used by utility programs for search queries
		is <a href="grammar_query.htm">here (query language grammar)</a>.
		</p>
		<h2>List of utility programs</h2>
<ul>
!ENDBLOCK
cat <<'!ENDBLOCK'
<a name="strusCreate"></a><li><b>strusCreate</b>
<br/>Create a strus storage. (implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusCreate_usage" type="checkbox" >
<label for="strusCreate_usage">Usage</label>
<pre>
!ENDBLOCK
strusCreate -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusDestroy"></a><li><b>strusDestroy</b>
<br/>Remove a strus storage and all its files. (implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusDestroy_usage" type="checkbox" >
<label for="strusDestroy_usage">Usage</label>
<pre>
!ENDBLOCK
strusDestroy -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusInspect"></a><li><b>strusInspect</b>
<br/>Inspect elements of items inserted in a strus storage. (implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusInspect_usage" type="checkbox" >
<label for="strusInspect_usage">Usage</label>
<pre>
!ENDBLOCK
strusInspect -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusAnalyze"></a><li><b>strusAnalyze</b>
<br/>Dump the document analyze result without feeding the storage. This program can be used to check the result of the document analysis.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusAnalyze_usage" type="checkbox" >
<label for="strusAnalyze_usage">Usage</label>
<pre>
!ENDBLOCK
strusAnalyze -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusAnalyzePhrase"></a><li><b>strusAnalyzePhrase</b>
<br/>Call the query analyzer with a phrase to analyze. This program can also be used to check details of the document analyzer as it tokenizes and normalizes a text segment with the tokenizer and normalizer specified.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusAnalyzePhrase_usage" type="checkbox" >
<label for="strusAnalyzePhrase_usage">Usage</label>
<pre>
!ENDBLOCK
strusAnalyzePhrase -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusAnalyzeQuery"></a><li><b>strusAnalyzeQuery</b>
<br/>Call the query analyzer with a query to analyze.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusAnalyzeQuery_usage" type="checkbox" >
<label for="strusAnalyzeQuery_usage">Usage</label>
<pre>
!ENDBLOCK
strusAnalyzeQuery -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusSegment"></a><li><b>strusSegment</b>
<br/>Call the segmenter with a document and one or more expressions to exract with the segmenter. Dump the resulting segments to stdout.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusSegment_usage" type="checkbox" >
<label for="strusSegment_usage">Usage</label>
<pre>
!ENDBLOCK
strusSegment -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>


<a name="strusPatternMatcher"></a><li><b>strusPatternMatcher</b>
<br/>Processes some documents mit a pattern matcher and output all matches found.<br\>
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>).
<input class="toggle-box" id="strusPatternMatcher_usage" type="checkbox" >
<label for="strusPatternMatcher_usage">Usage</label>
<pre>
!ENDBLOCK
strusPatternMatcher -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusPatternSerialize"></a><li><b>strusPatternSerialize</b>
<br/>Loads a pattern match program and outputs it in a serialized form that can be loaded by the analyzer.<br\>
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>).
<input class="toggle-box" id="strusPatternSerialize_usage" type="checkbox" >
<label for="strusPatternSerialize_usage">Usage</label>
<pre>
!ENDBLOCK
strusPatternSerialize -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusInsert"></a><li><b>strusInsert</b>
<br/>Insert a document or all files in a directory or in any descendant directory of it.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusInsert_usage" type="checkbox" >
<label for="strusInsert_usage">Usage</label>
<pre>
!ENDBLOCK
strusInsert -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusDeleteDocument"></a><li><b>strusDeleteDocument</b>
<br/>Deletes a list of documents referenced by document identifiers.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusDeleteDocument_usage" type="checkbox" >
<label for="strusDeleteDocument_usage">Usage</label>
<pre>
!ENDBLOCK
strusDeleteDocument -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusUpdateStorage"></a><li><b>strusUpdateStorage</b>
<br/>This program allows to update attributes, meta data and user access rights in a storage from a batch file.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusUpdateStorage_usage" type="checkbox" >
<label for="strusUpdateStorage_usage">Usage</label>
<pre>
!ENDBLOCK
strusUpdateStorage -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusCheckStorage"></a><li><b>strusCheckStorage</b>
<br/>This program checks a strus storage for corrupt data.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusCheckStorage_usage" type="checkbox" >
<label for="strusCheckStorage_usage">Usage</label>
<pre>
!ENDBLOCK
strusCheckStorage -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusCheckInsert"></a><li><b>strusCheckInsert</b>
<br/>Processes the documents the same way as strusInsert. But instead of inserting the documents,
it checks if the document representation in the storage is complete compared with the checked documents.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusCheckInsert_usage" type="checkbox" >
<label for="strusCheckInsert_usage">Usage</label>
<pre>
!ENDBLOCK
strusCheckInsert -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusQuery"></a><li><b>strusQuery</b>
<br/>Evaluate a query per command line.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusQuery_usage" type="checkbox" >
<label for="strusQuery_usage">Usage</label>
<pre>
!ENDBLOCK
strusQuery -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusAlterMetaData"></a><li><b>strusAlterMetaData</b>
<br/>Alter the table structure for document metadata of a storage.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusAlterMetaData_usage" type="checkbox" >
<label for="strusAlterMetaData_usage">Usage</label>
<pre>
!ENDBLOCK
strusAlterMetaData -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusGenerateKeyMap"></a><li><b>strusGenerateKeyMap</b>
<br/>Dumps a list of terms as result of document anaylsis of a file or directory.
The dump can be loaded by the storage on startup to create a map of frequently used terms.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusGenerateKeyMap_usage" type="checkbox" >
<label for="strusGenerateKeyMap_usage">Usage</label>
<pre>
!ENDBLOCK
strusGenerateKeyMap -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusDumpStatistics"></a><li><b>strusDumpStatistics</b>
<br/>Dumps the statisics that would be populated to in case of a distributed index to stout.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusDumpStatistics_usage" type="checkbox" >
<label for="strusDumpStatistics_usage">Usage</label>
<pre>
!ENDBLOCK
strusDumpStatistics -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusDumpStorage"></a><li><b>strusDumpStorage</b>
<br/>Dumps the statisics that would be populated to in case of a distributed index to stout.
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>)
<input class="toggle-box" id="strusDumpStorage_usage" type="checkbox" >
<label for="strusDumpStorage_usage">Usage</label>
<pre>
!ENDBLOCK
strusDumpStorage -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusResizeBlocks"></a><li><b>strusResizeBlocks</b>
<br/>Resize the blocks for a storage based on leveldb (leveldb only!).
(implemented in the project <a href="https://github.com/patrickfrey/strus">strus</a>)
<input class="toggle-box" id="strusResizeBlocks_usage" type="checkbox" >
<label for="strusResizeBlocks_usage">Usage</label>
<pre>
!ENDBLOCK
strusResizeBlocks -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusCreateVectorStorage"></a><li><b>strusCreateVectorStorage</b>
<br/>Creates a storage for vectors.<br\>
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>).
<input class="toggle-box" id="strusCreateVectorStorage_usage" type="checkbox" >
<label for="strusCreateVectorStorage_usage">Usage</label>
<pre>
!ENDBLOCK
strusCreateVectorStorage -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusBuildVectorStorage"></a><li><b>strusBuildVectorStorage</b>
<br/>Build relations describing some structures of the vectors inserted into a storge.<br\>
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>).
<input class="toggle-box" id="strusBuildVectorStorage_usage" type="checkbox" >
<label for="strusBuildVectorStorage_usage">Usage</label>
<pre>
!ENDBLOCK
strusBuildVectorStorage -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusInspectVectorStorage"></a><li><b>strusInspectVectorStorage</b>
<br/>Program to introspect a vector storage. Query for near vectors, for relations beween vectors, etc.<br\>
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>).
<input class="toggle-box" id="strusInspectVectorStorage_usage" type="checkbox" >
<label for="strusInspectVectorStorage_usage">Usage</label>
<pre>
!ENDBLOCK
strusInspectVectorStorage -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusRpcServer"></a><li><b>strusRpcServer</b>
<br/>Start a server processing requests from strus RPC clients<br\>
(implemented in the project <a href="https://github.com/patrickfrey/strusRpc">strusRpc</a>).
<input class="toggle-box" id="strusRpcServer_usage" type="checkbox" >
<label for="strusRpcServer_usage">Usage</label>
<pre>
!ENDBLOCK
strusRpcServer -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusHelp"></a><li><b>strusHelp</b>
<br/>Program to print descriptions of functions available to console.<br\>
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>).
<input class="toggle-box" id="strusHelp_usage" type="checkbox" >
<label for="strusHelp_usage">Usage</label>
<pre>
!ENDBLOCK
strusHelp -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusModuleInfo"></a><li><b>strusModuleInfo</b>
<br/>Program to print some information about a module in the module header (version, identifiers, etc.).<br\>
(implemented in the project <a href="https://github.com/patrickfrey/strusModule">strusModule</a>).
<input class="toggle-box" id="strusModuleInfo_usage" type="checkbox" >
<label for="strusModuleInfo_usage">Usage</label>
<pre>
!ENDBLOCK
strusModuleInfo -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusPageWeight"></a><li><b>strusPageWeight</b>
<br/>Calculate the weight of a page derived from its linkage to other pages.
The linkage info is fed in a proprietary text format as input.
If strusVector has been built with WITH_PAGERANK="YES" then the value calculated will
be the <a href="https://en.wikipedia.org/wiki/PageRank">pagerank</a> value 
(invented by Larry Page and patented in the USA as https://www.google.com/patents/US6285999).
If strusVector has been built without page rank support or taken from a standard strus package then
the calculated value will be derived from the number of links pointing to a document (non transitive).<br\>
(implemented in the project <a href="https://github.com/patrickfrey/strusVector">strusVector</a>).
<input class="toggle-box" id="strusPageWeight_usage" type="checkbox" >
<label for="strusPageWeight_usage">Usage</label>
<pre>
!ENDBLOCK
strusPageWeight -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

<a name="strusUpdateStorageCalcStatistics"></a><li><b>strusUpdateStorageCalcStatistics</b>
<br/>Program to calculate a formula for each document in the storages and update a metadata field with the result.<br\>
(implemented in the project <a href="https://github.com/patrickfrey/strusUtilities">strusUtilities</a>).
<input class="toggle-box" id="strusUpdateStorageCalcStatistics_usage" type="checkbox" >
<label for="strusUpdateStorageCalcStatistics_usage">Usage</label>
<pre>
!ENDBLOCK
strusUpdateStorageCalcStatistics -h 2>&1 | sed 's/[&]/\&amp;/g' | sed 's/[<]/\&lt;/g' | sed 's/[>]/\&gt;/g'
cat <<'!ENDBLOCK'
</pre>
</li>

</div>
</div>
</body>
</html>
!ENDBLOCK
