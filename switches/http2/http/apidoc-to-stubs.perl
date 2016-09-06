#!/usr/bin/perl -w

use JSON::PP;

my $coder = JSON::PP->new->ascii->pretty->allow_nonref->canonical;
my $apidoc = $coder->decode( join('',<>) );

my %groups;

# Declare models
snip();
my $models = $apidoc->{definitions};
foreach my $model (sort keys %{$models}) {
	eval { delete $models->{$model}->{example}; }
}
print "\t\t//! Models shared by endpoints\n";
print "\t\tdefinitions( R\"(\n", indented_json($coder->encode($models)), ")\"_json });\n\n\n";

# Find endpoint groups
foreach my $path (keys %{$apidoc->{paths}}) {
	my $group = camelcase( (split /[\/\#]/, $path)[1] );
	$groups{$group} ||= [];
	push $groups{$group}, $path;
}

# For each group, for each endpoint, register endpoint and generate handler stub
foreach my $group (sort keys %groups) {
	snip();
	print "\t//! Register all $group-related handlers\n\tvoid HttpDispatcher::register",
		ucfirst($group),"Handlers()\n\t{\n";

	foreach my $path (sort @{$groups{$group}}) {
		foreach my $method (grep { /get|put|post|delete|options/ }
		keys %{$apidoc->{paths}->{$path}}) {
			my $doc = $apidoc->{paths}->{$path}->{$method};
			foreach my $response (keys %{$doc->{responses}}) {
				eval { delete $doc->{responses}->{$response}->{examples} };
				if (! $doc->{responses}->{$response}->{description} and
		  $doc->{responses}->{$response}->{schema}->{description}) {
			  $doc->{responses}->{$response}->{description} = $doc->{responses}->{$response}->{schema}->{description};
		  }
			}
			eval { delete $doc->{parameters}->{schema}->{example} };

			my @path = split /\//, $path;

			printf("\t\tREGISTER_HANDLER(%s, %s, { %s }, { R\"(\n",
			       camelcase($doc->{operationId})."Handler", uc($method),
			join(", ",map { "\"$_\"" } @path[1..$#path]));
			print indented_json($coder->encode($doc));
			print ")\"_json });\n";
		}
	}

	print "\t}\n\n";

	foreach my $path (sort @{$groups{$group}}) {
		foreach my $method (grep { /get|put|post|delete|options/ }
		keys %{$apidoc->{paths}->{$path}}) {
			my ($produces) = @{$apidoc->{paths}->{$path}->{$method}->{produces}};
			print "\t//! handler for ",uc($method)," $path ".($produces ? "-> $produces" : "")."\n";
			printf("\tvoid HttpDispatcher::%s(HandlerContext& context)\n\t\{\n",
			       camelcase($apidoc->{paths}->{$path}->{$method}->{operationId})."Handler");
			if ($produces and $produces =~ /json/) {
				print "\t\tHttpResponse::fromJSON({}).send(context.stream);\n";
			} elsif ($produces and $produces =~ /text/) {
				print "\t\tHttpResponse::fromPlainString(\"\").send(context.stream);\n";
			} else {
				print "\t\tHttpResponse::fromStatus(HttpStatus::NO_CONTENT).send(context.stream);\n";
			}
			print "\t}\n\n";
		}
	}
}

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
