#!/usr/bin/perl
use strict;
use warnings;
use 5.014;
use HTML::Entities;

sub printProgramDescription
{
	my ($prgname,$description,$project) = @_;
	my $doc = `$prgname -h`;
	$doc =~ s/[\&]/&amp;/g;
	$doc =~ s/[\<]/&lt;/g;
	$doc =~ s/[\>]/&gt;/g;
	$doc =~ s/description:[^\n]*\n//;

	print '<h3>' . $prgname . '</h3>' . "\n";
	print '<p class="description">' . $description . '<br\>' . "\n";
	print "(implemented in the project <a href=\"https://github.com/patrickfrey/$project\">$project</a>)\n</p>\n";
	print '<h4>Usage</h4>' . "\n";
	print '<pre>' . "\n";
	print "$doc\n";
	print '</pre>' . "\n";
}

sub processFile
{
	my ($sourcefile,$project) = @_;
	open( SRCFILE, "<$sourcefile") or die "Couldn't open file $sourcefile for reading, $!";
	my $programName = '';
	my $description = '';
	while (my $row = <SRCFILE>)
	{
		chomp $row;
		if ($row =~ m/^[\#]{3}[ ]*(.*)[ ]*$/)
		{
			if ($programName ne '')
			{
				printProgramDescription( $programName, $description,$project);
			}
			$programName = $1;
			$description = ''; 
		}
		elsif ($row =~ m/\S/)
		{
			$description .= $row . "\n"; 
		}
	}
	if ($programName ne '')
	{
		printProgramDescription( $programName, $description,$project);
	}
	close( SRCFILE);
}

print <<EOF;
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 2.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
	<link rel="icon" type="image/ico" href="images/strus.ico" />
	<meta http-equiv="content-type" content="text/html; charset=utf-8" />
	<meta name="description" content="Description of the command line utility programs of strus, a collection of C++ libraries for building a full-text search engine." />
	<meta name="keywords" content="fulltext search engine C++" />
	<meta name="author" content="Patrick Frey &lt;patrickpfrey (a) yahoo (dt) com&gt;" />
	<link rel="stylesheet" type="text/css" href="text-profile.css" title="Text Profile" media="all" />
	<title>Strus utility programs</title>
</head>
<body>
<script>
  (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
  (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
  m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
  })(window,document,'script','//www.google-analytics.com/analytics.js','ga');

  ga('create', 'UA-63809026-12', 'auto');
  ga('send', 'pageview');
</script>
<div id="wrap">
	<div id="content">
		<h1>Command line utility programs</h1>
		<h2>Languages used by the utility programs</h2>
		<p class="description">
		Some of the utility programs are based on source files with a proprietary grammar.
		But the functionality expressed with these domain specific languages is not a parallel
		universe. All languages map to calls of the strus core and analyzer API.
		All loading of programs is implemented as calls of
		<a href="http://patrickfrey.github.io/strusUtilities/doc/doxygen/html/index.html">the program loader interface</a>.
		</p>
		<h3>Analyzer program</h3>
		<p class="description">The grammar of the sources referred to as analyzer programs 
		by some utility programs are defined <a href="grammar_analyzerprg.htm">here (analyzer program grammar)</a>.
		</p>
		<h3>Query evaluation program</h3>
		<p class="description">The grammar of the sources referred to as query evaluation programs 
		by some utility programs are defined <a href="grammar_queryevalprg.htm">here (query evaluation program grammar)</a>.
		</p>

		<h2>List of utility programs</h2>
EOF

processFile( "/home/patrick/Projects/github/strusUtilities/README.md", "strusUtilities");
printProgramDescription( "strusRpcServer", "Start a server processing requests from strus RPC clients", "strusRpc");

print <<EOF;
	</div>
</div>
</body>
EOF

