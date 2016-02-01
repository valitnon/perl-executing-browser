#!/usr/bin/perl -w

use strict;
use warnings;
use POSIX qw (strftime);
use 5.010;
no lib ".";

BEGIN {
	##############################
	# BAN ALL PROHIBITED CORE FUNCTIONS:
	##############################
	no ops qw(:dangerous sysopen :subprocess :sys_db unlink);

	##############################
	# HANDLE ENVIRONMENT VARIABLES:
	##############################
	my $DOCUMENT_ROOT = $ENV{'DOCUMENT_ROOT'};

	# Generate random values in case
	# FOLDER_TO_OPEN and FILE_TO_OPEN
	# environment variables are not set.
	# This is necessary for the overriden core functions
	# 'opendir', 'chdir' and 'open'.
	my $FOLDER_TO_OPEN;
	if (defined($ENV{'FOLDER_TO_OPEN'})) {
		$FOLDER_TO_OPEN = $ENV{'FOLDER_TO_OPEN'};
	} else {
		$FOLDER_TO_OPEN = rand().strftime '%d-%m-%Y--%H-%M-%S', gmtime();
	}

	my $FILE_TO_OPEN;
	if (defined($ENV{'FILE_TO_OPEN'})) {
		$FILE_TO_OPEN = $ENV{'FILE_TO_OPEN'};
	} else {
		$FILE_TO_OPEN = rand().strftime '%d-%m-%Y--%H-%M-%S', gmtime();
	}

	##############################
	# OVERRIDE POTENTIALY DANGEROUS
	# CORE FUNCTIONS:
	##############################
	*CORE::GLOBAL::opendir = sub (*;$@) {
		(my $package, my $filename, my $line) = caller();
		my $dir = $_[1];
		if ($dir =~ $DOCUMENT_ROOT or $dir =~ $FOLDER_TO_OPEN) {
			return CORE::opendir $_[0], $_[1];
		} else {
			die "Intercepted insecure 'opendir' call from package '$package', line: $line.<br>Opening directory '$dir' is not allowed!\n";
		}
	};

	*CORE::GLOBAL::chdir = sub (*;$@) {
		(my $package, my $filename, my $line) = caller();
		my $dir = $_[0];
		if ($dir =~ $DOCUMENT_ROOT or $dir =~ $FOLDER_TO_OPEN) {
			CORE::chdir $_[0];
		} else {
			die "Intercepted insecure 'chdir' call from package '$package', line: $line.<br>Changing directory to '$dir' is not allowed!\n";
		}
	};

	*CORE::GLOBAL::open = sub (*;$@) {
		(my $package, my $filename, my $line) = caller();
		my $handle = shift;
		if (@_ == 1) {
			my $filepath = $_[0];
			if ($_[0] =~ $DOCUMENT_ROOT or $_[0] =~ $FOLDER_TO_OPEN or $_[0] =~ $FILE_TO_OPEN) {
				return CORE::open ($handle, $_[0]);
			} else {
				$filepath =~ s/(\<|\>)//;
				die "Intercepted insecure 'open' call from package '$package', line: $line.<br>Opening '$filepath' is not allowed!\n";
			}
		} elsif (@_ == 2) {
			my $filepath = $_[1];
			if ($_[1] =~ $DOCUMENT_ROOT or $_[1] =~ $FOLDER_TO_OPEN or $_[1] =~ $FILE_TO_OPEN) {
				return CORE::open ($handle, $_[1]);
			} else {
				die "Intercepted insecure 'open' call from package '$package', line: $line.<br>Opening '$filepath' is not allowed!\n";
			}
		} elsif (@_ == 3) {
			if (defined $_[2]) {
				my $filepath = $_[2];
				if ($_[2] =~ $DOCUMENT_ROOT or $_[2] =~ $FOLDER_TO_OPEN or $_[2] =~ $FILE_TO_OPEN) {
					CORE::open $handle, $_[1], $_[2];
				} else {
					die "Intercepted insecure 'open' call from package '$package', line: $line.<br>Opening '$filepath' is not allowed!\n";
				}
			} else {
				my $filepath = $_[1];
				if ($_[1] =~ $DOCUMENT_ROOT or $_[1] =~ $FOLDER_TO_OPEN or $_[1] =~ $FILE_TO_OPEN) {
					CORE::open $handle, $_[1], undef; # special case
				} else {
					die "Intercepted insecure 'open' call from package '$package', line: $line.<br>Opening '$filepath' is not allowed!\n";
				}
			}
		} else {
			my $filepath = $_[1];
			if ($_[1] =~ $DOCUMENT_ROOT or $_[1] =~ $FOLDER_TO_OPEN or $_[1] =~ $FILE_TO_OPEN or
				$_[2] =~ $DOCUMENT_ROOT or $_[2] =~ $FOLDER_TO_OPEN or $_[2] =~ $FILE_TO_OPEN) {
				CORE::open $handle, $_[1], $_[2], @_[3..$#_];
			} else {
				die "Intercepted insecure 'open' call from package '$package', line: $line.<br>Opening '$filepath' is not allowed!\n";
			}
		}
	};
}

##############################
# REDIRECT STDERR TO A VARIABLE:
##############################
CORE::open (my $saved_stderr_filehandle, '>&', \*STDERR)  or die "Can not duplicate STDERR: $!";
close STDERR;
my $stderr;
CORE::open (STDERR, '>', \$stderr) or die "Unable to open STDERR: $!";

##############################
# READ USER SCRIPT FROM
# THE FIRST COMMAND LINE ARGUMENT:
##############################
my $file = $ARGV[0];
CORE::open my $filehandle, '<', $file or die;
my @user_code = <$filehandle>;
close $filehandle;

##############################
# STATIC CODE ANALYSIS:
##############################
my %problematic_lines;
my $line_number;
foreach my $line (@user_code) {
	$line_number++;

	if ($line =~ m/CORE::/) {
		if ($line =~ m/#.*CORE::(opendir|chdir|open)/) {
			next;
		} else {
			$problematic_lines{"Line ".$line_number.": ".$line} = "Forbidden invocation of non-overriden core function detected!";
		}
	}

	if ($line =~ m/use lib/) {
		if ($line =~ m/#.*use lib/) {
			next;
		} else {
			$problematic_lines{"Line ".$line_number.": ".$line} = "Forbidden 'use lib' detected!";
		}
	}

	if ($line =~ m/unshift\s*\@INC/ or $line =~ m/push\s*\(\s*\@INC/ or $line =~ m/push\s*\@INC/) {
		if ($line =~ m/#.*unshift\s*\@INC/ or $line =~ m/#.*push\s*\(\s*\@INC/ or $line =~ m/#.*push\s*\@INC/) {
			next;
		} else {
			$problematic_lines{"Line ".$line_number.": ".$line} = "Forbidden \@INC array manipulation detected!";
		}
	}
}

##############################
# HTML HEADER AND FOOTER:
##############################
my $header = "<html>

	<head>
		<title>Perl Executing Browser - Errors</title>
		<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>
		<style type='text/css'>body {text-align: left}</style>\n
	</head>

	<body>";

my $footer = "</body>

</html>";

##############################
# EXECUTE USER CODE IN 'EVAL' STATEMENT:
##############################
if (scalar (keys %problematic_lines) == 0) {
	my $user_code = join ('', @user_code);
	eval ($user_code);

	close (STDERR) or die "Can not close STDERR: $!";
	CORE::open (STDERR, '>&', $saved_stderr_filehandle) or die "Can not restore STDERR: $!";
} elsif (scalar (keys %problematic_lines) > 0) {
	close (STDERR) or die "Can not close STDERR: $!";
	CORE::open (STDERR, '>&', $saved_stderr_filehandle) or die "Can not restore STDERR: $!";

	print STDERR $header;
	print STDERR "<p align='center'><font size='5' face='SansSerif'>";
	print STDERR "Script execution was not attempted due to security violation";
	if (scalar (keys %problematic_lines) == 1) {
		print STDERR ":</font></p>\n";
	} elsif (scalar (keys %problematic_lines) > 1) {
		print STDERR "s:</font></p>\n";
	}

	while ((my $line, my $explanation) = each (%problematic_lines)){
		print STDERR "<pre>";
		print STDERR "$line";
		print STDERR "$explanation";
		print STDERR "</pre>";
	}
	
	print STDERR $footer;
	exit;
}

##############################
# PRINT SAVED STDERR, IF ANY:
##############################
if ($@) {
	if ($@ =~ m/trapped/ or $@ =~ m/insecure/) {
		print STDERR $header;
		print STDERR "<p align='center'><font size='5' face='SansSerif'>Insecure code was blocked:</font></p>\n";
		print STDERR "<pre>$@</pre>";
		print STDERR $footer;
		exit;
	} else {
		print STDERR $header;
		print STDERR "<p align='center'><font size='5' face='SansSerif'>Errors were found during script execution:</font></p>\n";
		print STDERR "<pre>$@</pre>";
		print STDERR $footer;
		exit;
	}
}

if (defined($stderr)) {
	print STDERR $header;
	print STDERR "<p align='center'><font size='5' face='SansSerif'>Errors were found during script execution:</font></p>\n";
	print STDERR "<pre>$stderr</pre>";
	print STDERR $footer;
}
