#!/usr/bin/perl
use strict;
use warnings;
use Path::Tiny;
my $targetBin = shift(@ARGV);
my $targetSize = shift(@ARGV);
my $data = pack('C*', ((0x00) x (1024 * 1024 * $targetSize)));
my %things = @ARGV;
foreach my $addr (sort { hex($a) <=> hex($b) } keys %things)
{
    my $offset = hex($addr);
    my $bin = path($things{$addr})->slurp_raw();
    my $size = length($bin);
    #printf("offset %d size %d / %d\n", $offset, $size, length($data));
    substr($data, $offset, $size, $bin);
}
path($targetBin)->spew_raw($data);
