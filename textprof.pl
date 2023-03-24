#!/usr/intel/bin/perl

#
# textprof - text segment profile viewer
#

# SimpleScalar(TM) Tool Suite
# Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
# All Rights Reserved. 
#
# THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
# YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
#
# No portion of this work may be used by any commercial entity, or for any
# commercial purpose, without the prior, written permission of SimpleScalar,
# LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
# as described below.
#
# 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
# or implied. The user of the program accepts full responsibility for the
# application of the program and the use of any results.
#
# 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
# downloaded, compiled, executed, copied, and modified solely for nonprofit,
# educational, noncommercial research, and noncommercial scholarship
# purposes provided that this notice in its entirety accompanies all copies.
# Copies of the modified software can be delivered to persons who use it
# solely for nonprofit, educational, noncommercial research, and
# noncommercial scholarship purposes provided that this notice in its
# entirety accompanies all copies.
#
# 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
# PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
#
# 4. No nonprofit user may place any restrictions on the use of this software,
# including as modified by the user, by any other authorized user.
#
# 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
# in compiled or executable form as set forth in Section 2, provided that
# either: (A) it is accompanied by the corresponding machine-readable source
# code, or (B) it is accompanied by a written offer, with no time limit, to
# give anyone a machine-readable copy of the corresponding source code in
# return for reimbursement of the cost of distribution. This written offer
# must permit verbatim duplication by anyone, or (C) it is distributed by
# someone who received only the executable form, and is accompanied by a
# copy of the written offer of source code.
#
# 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
# currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
# 2395 Timbercrest Court, Ann Arbor, MI 48105.
#
# Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
#


#
# config parms
#
$gzip_cmd = "gzip -dc";

#
# parse commands
#
if (@ARGV != 3)
  {
     print STDERR
"Usage: textprof <disassembly_file> <simulator_output> <stat_name>\n".
"\n".
"         where <disassembly_file> is a disassembly file, generated with\n".
"         the command \"objdump -d <binary>\", <simulator_output> is the\n".
"         simulator output containing a text-based profile, and <stat_name>\n".
"         is the name of the text-based profile to be viewed.  Inputs may\n".
"         be gzip\'ed (.gz) or compress\'ed (.Z).\n".
"\n".
"         Example usage:\n".
"\n".
"           objdump -d test-math >&! test-math.dis\n".
"           sim-profile -pcstat sim_num_insn test-math >&! test-math.out\n".
"           textprof test-math.dis test-math.out sim_num_insn\n".
"\n";
     exit -1;
  }

$dis_file = $ARGV[0];
$sim_output = $ARGV[1];
$stat_name = $ARGV[2];

#
# parse out the <stat_name> profile from <sim_output>
#
if (grep(/.gz$/, $sim_output) || grep(/.Z$/, $sim_output) )
  {
    open(SIM_OUTPUT, "$gzip_cmd $sim_output |")
	|| die "Cannot open simulator output file: $sim_output\n";
  }
else
  {
    open(SIM_OUTPUT, $sim_output)
	|| die "Cannot open simulator output file: $sim_output\n";
  }
$parse_mode = "FIND_START";
while (<SIM_OUTPUT>)
  {
    # detect start of stats
    if ($parse_mode eq "FIND_START" && (/^$stat_name\.start_dist$/))
      {
        $parse_mode = "FIND_END";
	next;
      }
    if ($parse_mode eq "FIND_END" && (/^$stat_name\.end_dist$/))
      {
	$parse_mode = "FOUND_STAT";
	last;
      }
    if ($parse_mode eq "FIND_END")
      {
	# parse a real histo line
	if (/^(0x[0-9a-fA-F]+)\s+(\d+)\s+([0-9.]+)$/)
	  {
	    $histo_exists{hex($1)} = 1;
	    $histo_freq{hex($1)} = $2;
	    $histo_frac{hex($1)} = $3;
	  }
	else
	  {
	    print "** WARNING ** could not parse line: `$_'\n";
	  }
      }
  }
if ($parse_mode ne "FOUND_STAT")
  {
    print STDERR "** FATAL ** couldn't find histogram `$stat_name' in `$sim_output'\n";
    exit -1;
  }
close(SIM_OUTPUT);

#
# consume the disassembly file, spew back out with simulator stats
#
if (grep(/.gz$/, $dis_file) || grep(/.Z$/, $dis_file) )
  {
    open(DIS_FILE, "$gzip_cmd $dis_file |")
	|| die "Cannot open disassembly file: $dis_file\n";
  }
else
  {
    open(DIS_FILE, $dis_file)
	|| die "Cannot open disassembly file: $dis_file\n";
  }

# print text profile header
print "Text profile view for statistic `$stat_name'.\n";
print "Statistics extracted from `$sim_output'.\n";
print "Statistics extracted bound to disassembly file `$dis_file'.\n";
print "Text statistics are shown in the form (<count>, <% of total>).\n";
print "\n";

while (<DIS_FILE>)
  {
    if (/^([0-9a-fA-F]+)\s+(.*)$/)
      {
	if ($histo_exists{hex($1)})
	  {
	    printf "%s:  (%11d, %6.2f): %s\n",
	      $1, $histo_freq{hex($1)}, $histo_frac{hex($1)}, $2;
	    $histo_probed{hex($1)} = 1;
	  }
	else
	  {
	    printf "%s:                       : %s\n", $1, $2;
	  }
      }
    else
      {
	# various other disassembly lines, just echo them out as is
	print;
      }
  }
close(DIS_FILE);

#
# check for unprobed EIP stats
#
$cnt = 0;
$unbnd = 0;
foreach $addr (keys %histo_exists)
  {
    if (!$histo_probed{$addr})
      {
	$cnt++;
	if ($cnt > 5)
	  {
	    $unbnd++;
	  }
	else
	  {
	    printf "** WARNING ** address 0x%08x was not bound to disassembly output.\n", $addr;
	  }
      }
  }
if ($unbnd)
  {
    print "** WARNING ** $unbnd more unbound addresses were detected.\n";
  }

exit 0;
