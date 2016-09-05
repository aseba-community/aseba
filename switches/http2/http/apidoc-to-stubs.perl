#!/usr/bin/perl -w

use JSON::PP;

my $coder = JSON::PP->new->ascii->pretty->allow_nonref;
my $apidoc = $coder->decode( join('',<>) );

my %groups;

foreach my $path (keys %{$apidoc->{paths}}) {
    my $group = camelcase( (split /[\/\#]/, $path)[1] );
    $groups{$group} ||= [];
    push $groups{$group}, $path;
}

foreach my $group (sort keys %groups) {
    print "\t//! Register all $group-related handlers\n\tvoid HttpDispatcher::register",ucfirst($group),"Handlers()\n\t{\n";

    foreach my $path (sort @{$groups{$group}}) {
	foreach my $method (grep { /get|put|post|delete|options/ }
			    keys %{$apidoc->{paths}->{$path}}) {
	    my $doc = $apidoc->{paths}->{$path}->{$method};
	    foreach my $response (keys %{$doc->{responses}}) {
		eval { delete $doc->{responses}->{$response}->{examples} };
	    }
	    eval { delete $doc->{parameters}->{schema}->{example} };

	    # (my $operation = join("", map { $_ =~ s{[\{\}]}{}g; ucfirst(lc($_)) } split(/[\/\#]/, $path))) =~ s{[^A-Za-z]}{}g;
	    my $operation = camelcase( split(/[\/\#]/, $path) );
	    my $handler = $method . $operation . "Handler";
	    my @path = split /\//, $path;
	    my $indented_json = join("\n",map { "\t\t\t$_" } split(/\n/,$coder->encode($doc)));

	    printf("\t\tREGISTER_HANDLER(%s, %s, { %s }, { R\"(\n",
		   $handler, uc($method),
		   join(", ",map { "\"$_\"" } @path[1..$#path]));
	    print $indented_json;
	    print ")\"_json });\n";
	}
    }

    print "\t}\n\n";

    foreach my $path (sort @{$groups{$group}}) {
	foreach my $method (grep { /get|put|post|delete|options/ }
			    keys %{$apidoc->{paths}->{$path}}) {
	    my $operation = camelcase( split(/[\/\#]/, $path) );
	    my $handler = $method . $operation . "Handler";
	    my ($produces) = @{$apidoc->{paths}->{$path}->{$method}->{produces}};
	    print "\t//! handler for ",uc($method)," $path -> $produces\n";
	    print "\tvoid HttpDispatcher::$handler(HandlerContext& context)\n\t\{\n";
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
    return sanitize(join("", map { $_ =~ s{[\{\}]}{}g; ucfirst(lc($_)) } @_));
}
