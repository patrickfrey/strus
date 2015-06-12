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

	print '<h2>' . $prgname . '</h2>' . "\n";
	print '<p>' . $description . '<br\>' . "\n";
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
	<meta name="description" content="Story of strus, a collection of C++ libraries for building a full-text search engine." />
	<meta name="keywords" content="fulltext search engine C++" />
	<meta name="author" content="Patrick Frey <patrickpfrey (a) yahoo (dt) com" />
	<link rel="stylesheet" type="text/css" href="text-profile.css" title="Text Profile" media="all" />
	<title>Strus utility programs</title>
</head>

<body>
<div id="wrap">
	<div id="content">
		<h1>Utility programs</h1>
EOF

processFile( "/home/patrick/Projects/github/strusUtilities/README.md", "strusUtilities");
printProgramDescription( "strusRpcServer", "Start a server processing requests from strus RPC clients", "strusRpc");

print <<EOF;
	</div>
</div>
</body>
EOF

