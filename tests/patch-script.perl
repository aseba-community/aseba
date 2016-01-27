#!/usr/bin/perl

$input = shift;
$output = shift;

%subs;

while (my $arg = shift) {
  my ($cmd,$path) = ($arg =~ m/(\w+)=(.*)/);
  $subs{$cmd} = $path;
}

open $in, '<', $input || die("Can't open $input: $!");
open $out, '>', $output || die("Can't open $output: $!");

while (<$in>) {
  foreach my $cmd (keys %subs) {
    $_ =~ s{$cmd}{$subs{$cmd}}ge;
  }
  print $out $_;
}
close $out;
close $in;
