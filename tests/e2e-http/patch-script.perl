#!/usr/bin/perl

$input = shift;
$output = shift;
$http = shift;
$dummynode = shift;
$switch = shift;
$massloader = shift;

open $in, '<', $input || die("Can't open $input: $!");
open $out, '>', $output || die("Can't open $output: $!");

while (<$in>) {
    $_ =~ s{ASEBAHTTP}{$http}ge;
    $_ =~ s{ASEBADUMMYNODE}{$dummynode}ge;
    $_ =~ s{ASEBASWITCH}{$switch}ge;
    $_ =~ s{ASEBAMASSLOADER}{$massloader}ge;
    print $out $_;
}
close $out;
close $in;
