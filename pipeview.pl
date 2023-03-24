#!/usr/bin/perl

#
# pipeview - pipeline trace pretty printer
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
# TODO: check live lengths...
#

if (@ARGV != 1)
  {
    print STDERR "Usage: pview <pipe_trace>\n";
    exit -1;
  }

open(TRC_FILE, $ARGV[0])
	|| die "Cannot open pipeline trace file `$ARGV[0]'";

$cycle = 0;

print "Instruction event legend:\n";
print "\n";
print "    * - cache miss\n";
print "    ! - TLB miss\n";
print "    / - branch misprediction\n";
print "    \\ - branch misprediction detected\n";
print "    + - address generation execution\n";
print "\n";

while (<TRC_FILE>)
  {
    # remove carriage return
    chop;

    # new instruction
    if (/^\+\s+(\d+)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(.*)\n?$/)
      {
	# register this instruction in the live instruction hash table
	$insts{$1} = 1;
	$insts_pc{$1} = $2;
	$insts_addr{$1} = $3;
	$insts_asm{$1} = $4;
	$insts_iid{$1} = 
	    chr ((ord("a") + (($1 / 26) % 26))) .
	    chr (ord("a") + ($1 % 26));

	# print instruction id and asm info
	print "$insts_iid{$1} = `$2: $4'\n";
      }
    # deleted instruction
    elsif (/^\-\s+(\d+)\n?$/)
      {
	# record deletion, these are processed after pipe info is printed
	@dead_pseqs = ($1, @dead_pseqs);
      }
    # new cycle
    elsif (/^@\s+(\d+)\n?$/)
      {
	# clear all instruction stage lists
	@if_iids = ();
	@da_iids = ();
	@ex_iids = ();
	@wb_iids = ();
	@ct_iids = ();

	# print last cycle pipeline state
	foreach $pseq (keys %insts)
	  {
	    # partition by instruction stage
	    $stage = $insts_stage{$pseq};

	    # decode the instruction events
	    $events = hex($insts_events{$pseq});
	    $evstr = "";
	    if ($events & 0x00000001)		# cache miss
	      {
		$evstr = $evstr . "*";
	      }
	    if (($events & 0x00000002) != 0)		# TLB miss
	      {
		$evstr = $evstr . "!";
	      }
	    if ($events & 0x00000004)		# mis-predict occurred
	      {
		$evstr = $evstr . "/";
	      }
	    if ($events & 0x00000008)		# mis-predict detected
	      {
		$evstr = $evstr . "\\";
	      }
	    if ($events & 0x00000010)		# addr generation
	      {
		$evstr = $evstr . "+";
	      }

	    if ($stage eq "IF")
	      {
		@if_iids = (@if_iids, $insts_iid{$pseq} . $evstr);
	      }
	    elsif ($stage eq "DA")
	      {
		@da_iids = (@da_iids, $insts_iid{$pseq} . $evstr);
	      }
	    elsif ($stage eq "EX")
	      {
		@ex_iids = (@ex_iids, $insts_iid{$pseq} . $evstr);
	      }
	    elsif ($stage eq "WB")
	      {
		@wb_iids = (@wb_iids, $insts_iid{$pseq} . $evstr);
	      }
	    elsif ($stage eq "CT")
	      {
		@ct_iids = (@ct_iids, $insts_iid{$pseq} . $evstr);
	      }
	    else
	      {
		print "warning: unknown stage\n";
	      }
	  }


#	print "** if_iids: ";
#	foreach $iid (@if_iids)
#	  {
#	    print "$iid ";
#         }
#	print "\n";

	# sort stage lists
	@if_iids = sort @if_iids;
	@da_iids = sort @da_iids;
	@ex_iids = sort @ex_iids;
	@wb_iids = sort @wb_iids;
	@ct_iids = sort @ct_iids;

	# print pipeline header
	if ((@if_iids + @da_iids + @ex_iids + @wb_iids + @ct_iids) > 0)
	  {
	    print "\n";
	    print " [IF]     ", " [DA]     ", " [EX]     ", " [WB]     ", " [CT]", "\n";
	  }

	# print stage data
	while ((@if_iids + @da_iids + @ex_iids + @wb_iids + @ct_iids) > 0)
	  {
	    if (@if_iids > 0)
	      {
		($iid, @if_iids) = @if_iids;
		printf "  %-6s  ", $iid;
	      }
	    else
	      {
		print "          ";
	      }

	    if (@da_iids > 0)
	      {
		($iid, @da_iids) = @da_iids;
		printf "  %-6s  ", $iid;
	      }
	    else
	      {
		print "          ";
	      }

	    if (@ex_iids > 0)
	      {
		($iid, @ex_iids) = @ex_iids;
		printf "  %-6s  ", $iid;
	      }
	    else
	      {
		print "          ";
	      }

	    if (@wb_iids > 0)
	      {
		($iid, @wb_iids) = @wb_iids;
		printf "  %-6s  ", $iid;
	      }
	    else
	      {
		print "          ";
	      }

	    if (@ct_iids > 0)
	      {
		($iid, @ct_iids) = @ct_iids;
		printf "  %-6s  ", $iid;
	      }
	    else
	      {
		print "          ";
	      }
	    print "\n";
	  }
	print "\n";

	# delete instruction that finished in this cycle
	foreach $pseq (@dead_pseqs)
	  {
	    delete $insts{$pseq};
	    delete $insts_pc{$pseq};
	    delete $insts_addr{$pseq};
	    delete $insts_asm{$pseq};
	    delete $insts_iid{$pseq};
	    delete $insts_stage{$pseq};
	    delete $insts_events{$pseq};
	  }
	@dead_pseqs = ();

        # go on to the next cycle
	$cycle = $1;
	print "@ $cycle\n";
      }
    # instruction transition to a new stage
    elsif (/^\*\s+(\d+)\s+(\w+)\s+(0x[0-9a-fA-F]+)\n?$/)
      {
	if ($insts{$1})
	  {
	    # this instruction is live...
	    $insts_stage{$1} = $2;
	    $insts_events{$1} = $3;
	  }
	else
	  {
	    # this instruction is dead...
	    # print STDERR "inst `$1' is dead...\n";
	  }
      }
    else
      {
	print "warning: could not parse line: `$_'\n";
      }
  }

close(TRC_FILE);

