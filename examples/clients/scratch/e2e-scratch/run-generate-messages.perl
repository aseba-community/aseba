#!/usr/bin/perl -w

my $BASE  = 'http://localhost:3002';
my $POLL  = $BASE . "/poll\n";
my $RESET = $BASE . "/reset_all\n";
my $NODE  = $BASE . '/nodes%2Fthymio-II%2F';

# Require that asebascratch from run-e2e-scratch.sh is still running
use LWP::Simple;
die "No response from $BASE/poll; perhaps you need to start\n".
    "asebascratch --http 3002 --aesl thymio_motion.aesl ser:name=Thymio\n"
    unless get("$BASE/poll");

# Siege can only use URLs from a file, so make one
use File::Temp qw(tempfile);
my ($urls,$url_file) = tempfile("urlsXXXXX", UNLINK => 1);

# Generate urls
print $urls $POLL x 5;
print $urls $RESET;
print $urls $POLL x 30;
print $urls $NODE . "scratch_set_dial/0\n";
print $urls $NODE . "scratch_clear_leds\n";

foreach my $job (1..4) {
    print $urls $NODE . "scratch_move/".($job)."/".(50+$job)."\n";
    print $urls $POLL x 3;
    print $urls $NODE . "scratch_turn/".($job+10)."/".(60+$job)."\n";
    print $urls $POLL x 10;
    print $urls $NODE . "scratch_next_dial\n";
    print $urls $POLL x 17;
    print $urls $NODE . "A_sound_system/5\n";
    print $urls $POLL x 30;
}
print $urls $NODE . "scratch_set_leds/0/7\n";
print $urls $POLL x 10;
print $urls $RESET;

# Send URLs to asebascratch at 30 ms intervals
system("siege --file=$url_file --reps=once --concurrent=1 --delay=0.030");
