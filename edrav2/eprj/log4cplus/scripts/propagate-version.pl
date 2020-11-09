#!perl

# Enable in-place editing
BEGIN
{
    $^I = '.bak';
}

use strict;
use feature 'unicode_strings';

# parse line from include/log4cplus/version.h

open (my $fh, "<", "include/log4cplus/version.h")
    or die $!;

my ($major, $minor, $point, $version);
while (my $line = <$fh>)
{
    if ($line =~ m/\s* # \s* define \s+ LOG4CPLUS_VERSION \s+
          LOG4CPLUS_MAKE_VERSION \s* \(
          \s* (\d+) \s* , \s* (\d+) \s* , \s* (\d+) \s* \)/x)
    {
        ($major, $minor, $point) = ($1, $2, $3);
        $version = "$major.$minor.$point";
        print "version: ", $version, "\n";
        last;
    }
}

close $fh;

# parse SO version from configure.ac

open (my $fh2, "<", "configure.ac")
    or die $!;

my ($so_current, $so_revision, $so_age, $so_current_adjusted);
while (my $line = <$fh2>)
{
    if ($line =~ m/\s* LT_VERSION= \s*
          (\d+) \s* : \s* (\d+) \s* : \s* (\d+) \s*/x)
    {
        ($so_current, $so_revision, $so_age) = ($1, $2, $3);
        print +("SO version: ", $so_current, ".", $so_revision, ".", $so_age,
                "\n");
        $so_current_adjusted = $so_current - $so_age;
        print +("MingGW/Cygwin version: ", $major, "-", $minor, "-",
                $so_current_adjusted, "\n");
        last;
    }
}

close $fh2;

# edit configure.ac

{
    local $^I = ".bak";
    local @ARGV = ("configure.ac");
    while (my $line = <>)
    {
        $line =~ s/(.*AC_INIT\(.*\[)(\d+(?:\.\d+(?:\.\d+)?)?)(\].*)/$1$version$3/x;
        $line =~ s/(.*LT_RELEASE=)(.*)/$1$major.$minor/x;
        print $line;
    }

    local @ARGV = ("docs/doxygen.config");
    while (my $line = <>)
    {
        $line =~ s/(\s* PROJECT_NUMBER \s* = \s*)(.*)/$1$version/x;
        $line =~ s/(\s* OUTPUT_DIRECTORY \s* = \s*)(.*)/$1log4cplus-$version\/docs/x;
        print $line;
    }

    local @ARGV = ("docs/webpage_doxygen.config");
    while (my $line = <>)
    {
        $line =~ s/(\s* PROJECT_NUMBER \s* = \s*)(.*)/$1$version/x;
        $line =~ s/(\s* OUTPUT_DIRECTORY \s* = \s*)(.*)/$1webpage_docs-$version/x;
        print $line;
    }

    local @ARGV = ("log4cplus.spec", "mingw-log4cplus.spec");
    while (my $line = <>)
    {
        $line =~ s/(Version: \s*)(.*)/$1$version/x;
        if ($line =~ /Source\d*:/x)
        {
            $line =~ s/(\d+\.\d+\.\d+)/$version/gx;
        }
        print $line;
    }

    local @ARGV = ("cygport/log4cplus.cygport");
    while (my $line = <>)
    {
        $line =~ s/(\s* VERSION \s* = \s*)(\d+\.\d+\.\d+)(-.+)?/$1$version$3/x
            || $line =~ s/\d+ ([._\-]) \d+ ([._\-]) \d+
                         /$major$1$minor$2$so_current_adjusted/gx;
        print $line;
    }
}
