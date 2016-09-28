#!/usr/bin/perl -w

use JSON::PP;
use Data::Dumper;

my $coder = JSON::PP->new->ascii->pretty->allow_nonref->canonical;
my $apidoc = $coder->decode( join('',<>) );

my %groups;

# For each model, remove example
my $models = $apidoc->{definitions};
foreach my $model (sort keys %{$models}) {
  eval { delete $models->{$model}->{example}; }
}

# Find endpoint groups
foreach my $path (keys %{$apidoc->{paths}}) {
  my $group = camelcase( (split /[\/\#]/, $path)[1] );
  $groups{$group} ||= [];
  push $groups{$group}, $path;
}

# For each group, for each endpoint, move descriptions and remove example(s)
foreach my $group (sort keys %groups) {
  foreach my $path (sort @{$groups{$group}}) {
    foreach my $method (grep { /get|put|post|delete|options/ }
			keys %{$apidoc->{paths}->{$path}}) {
      my $doc = $apidoc->{paths}->{$path}->{$method};
      foreach my $response (keys %{$doc->{responses}}) {
	eval { delete $doc->{responses}->{$response}->{examples} };
	if ($doc->{responses}->{$response}->{schema}->{description}) {
	  $doc->{responses}->{$response}->{description} = $doc->{responses}->{$response}->{schema}->{description};
	  eval { delete $doc->{responses}->{$response}->{schema}->{description} };
	} else {
	  $doc->{responses}->{$response}->{description} = ($response =~ /^4/ ? "Error in " : "") .
	    $apidoc->{paths}->{$path}->{$method}->{summary};
	}
      }
      if ('HASH' eq ref($doc) && $doc->{parameters}) {
      	eval { delete $doc->{parameters}->{schema}->{example}; }
      }
    }
  }
}

$apidoc->{info}->{description} = "Aseba REST API v1";
print $coder->encode($apidoc);

sub sanitize {
  (my $id = $_[0]) =~ s{[^A-Za-z]}{}g;
  return $id;
}

sub camelcase {
  my @elements = map { $_ =~ s{[\{\}]}{}g; $_ } split(/-/, $_[0]);
  return sanitize( join("", lc($elements[0]), map { ucfirst(lc($_)) } @elements[1..$#elements]) );
}

sub indented_json {
  my ($object, $indent) = @_;
  return join("\n",map { $_=~s{   }{\t}g; "\t\t\t$_" } split(/\n/,$object));
}

sub snip {
  print "//---",("---snip---" x 7),"---\n\n";
}
