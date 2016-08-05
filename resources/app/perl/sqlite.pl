#!/usr/bin/perl -w

# UTF-8 encoded file

use strict;
use warnings;
use utf8;
use open ':std', ':encoding(UTF-8)';
use Cwd;
use DBI;

print "
<!DOCTYPE html>
<html>

	<head>
		<title>Perl Executing Browser - SQLite Test</title>
		<meta name='viewport' content='width=device-width, initial-scale=1'>
		<meta charset='utf-8'>
		<link rel='stylesheet' type='text/css' href='http://local-pseudodomain/bootstrap/css/themes/darkly-theme.css' media='all'>
	</head>

	<body>

		<p>
			<font size='5'>
				SQLite Test
			</font>
		</p>

		<div>\n";

my $cwd = cwd();
my $database_relative_pathname = "/resources/data/test.db";
my $db = DBI->connect ("dbi:SQLite:$cwd$database_relative_pathname","","", {sqlite_unicode => 1}) or
	die "Could not connect to database";

$db->do ("CREATE TABLE IF NOT EXISTS user (id INTEGER PRIMARY KEY, name TEXT, surname TEXT)");

my $all_records = $db->selectall_arrayref ("SELECT * FROM USER");

if (scalar @$all_records < 4) {
	$db->do ("INSERT INTO user\(name, surname) VALUES ( 'Linus', 'Torvalds')");
	$db->do ("INSERT INTO user\(name, surname) VALUES ( 'Richard', 'Stallman')");
	$db->do ("INSERT INTO user\(name, surname) VALUES ( 'Линус', 'Торвалдс')");
	$db->do ("INSERT INTO user\(name, surname) VALUES ( 'Ричард', 'Столман')");
}

$all_records = $db->selectall_arrayref ("SELECT * FROM USER");

foreach my $row (@$all_records) {
	my ($id, $name, $surname) = @$row;
	print "$id $name $surname <br>\n";
}

$db->disconnect;

print "\n
		</div>

	</body>

</html>\n";
