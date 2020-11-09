#! env perl
use strict;
use IO::All;
use LWP::UserAgent;
use URI;
use Getopt::Long;
use Config::YAML;
use Pod::Usage;
use File::Basename;
use File::Touch;
use User;

# set up configuration store

my $user_config_file = User->Home . '/.log4cplus-upload_to_wiki';
if (! -e $user_config_file)
{
    touch($user_config_file);
}

my $config = Config::YAML->new(
    config => $user_config_file,
    output => $user_config_file,
    bearer_token => '',
    wiki_endpoint => 'https://sourceforge.net/rest/p/log4cplus/wiki/');

if (-r $user_config_file)
{
    $config->read($user_config_file);
}

my $wiki_endpoint = $config->{wiki_endpoint};

# parse script parameters

my $bearer_token = $config->get_bearer_token;
my $file_path = '';
my $help = 0;
my $remember = 0;

if (! GetOptions("token|t=s" => \$bearer_token,
                 "remember" => \$remember,
                 "file|f=s" => \$file_path,
                 "help|h" => \$help))
{
    pod2usage(2);
}

# evaluate script parameters

pod2usage(1) if $help;
pod2usage(-message => 'missing bearer token argument')
    if $bearer_token eq '';
pod2usage(-message => 'missing input file argument')
    if $file_path eq '';

# save configuration

if ($remember)
{
    $config->set_bearer_token($bearer_token);
    $config->write;
}

# slurp input file

print "reading file ", $file_path, "\n";
my $text = io($file_path)->utf8->slurp;

# add SF specific table of contents
$text = "[TOC]\n\n" . $text;

# construct URL for POST request

my $uri = URI->new($wiki_endpoint);
my ($file_name, undef, undef) = fileparse($file_path);
$uri->path($uri->path . $file_name);
$uri->query_form('access_token' => $bearer_token);

# get user agent

print "POSTing to ", $uri->as_string, "\n";
my $ua = LWP::UserAgent->new;
push @{ $ua->requests_redirectable }, 'POST';

# set up POST request

my $req = HTTP::Request->new(POST => $uri);
$req->content_type("application/x-www-form-urlencoded; charset='utf8'");
$req->accept_decodable;
my $content_enc = URI->new;
$content_enc->query_form('text' => $text, 'labels' => 'readme');
$req->content($content_enc->query);
print "POST body:\n", $req->as_string, "\n";

# do POST request

my $resp = $ua->request($req);

# process response

print "POST response: ";
if ($resp->is_success)
{
    print $resp->decoded_content, "\n";
}
else
{
    print $resp->decoded_content, "\n";
    die $resp->status_line;
}

__END__

=head1 NAME

upload_to_wiki.pl - uploads a file (usually a Markdown document) as wiki page
on log4cplus SourceForge wiki.

=head1 SYNOPSIS

upload_to_wiki.pl [options] --token <bearer token> --file <file>

 Options:
   --help|h                      this help
   --token|t <bearer token>      SourceForge bearer token used for
                                 authentication
   --remember                    remember bearer token in configuration file
   --file|f <file>               file to be uploaded

=cut
