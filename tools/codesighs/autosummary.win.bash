#!/bin/bash
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is autosummary.win.bash code, released
# Oct 3, 2002.
#
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 2002 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s):
#    Garrett Arch Blythe, 03-October-2002
#
# Alternatively, the contents of this file may be used under the
# terms of the GNU Public License (the "GPL"), in which case the
# provisions of the GPL are applicable instead of those above.
# If you wish to allow use of your version of this file only
# under the terms of the GPL and not to allow others to use your
# version of this file under the MPL, indicate your decision by
# deleting the provisions above and replace them with the notice
# and other provisions required by the GPL.  If you do not delete
# the provisions above, a recipient may use your version of this
# file under either the MPL or the GPL.
#

#
#   A little help for my friends.
#
if [ "-h" == "$1" ];then 
    SHOWHELP="1"
fi
if [ "--help" == "$1" ];then 
    SHOWHELP="1"
fi
if [ "" == "$1" ]; then
    SHOWHELP="1"
fi
if [ "" == "$2" ]; then
    SHOWHELP="1"
fi
if [ "" == "$3" ]; then
    SHOWHELP="1"
fi


#
#   Show the help if required.
#
if [ $SHOWHELP ]; then
    echo "usage: $0 <save_results> <old_results> <summary>"
    echo "  <save_results> is a file that will receive the results of this run."
    echo "    This file can be used in a future run as the old results."
    echo "  <old_results> is a results file from a previous run."
    echo "    It is used to diff with current results and come up with a summary"
    echo "      of changes."
    echo "    It is OK if the file does not exist, just supply the argument."
    echo "  <summary> is a file which will contain a human readable report."
    echo "    This file is most useful by providing more information than the"
    echo "      normally single digit output of this script."
    echo ""
    echo "Run this command from the parent directory of the mozilla tree."
    echo ""
    echo "This command will output two numbers to stdout that will represent"
    echo "  the total size of all code and data, and a delta from the prior."
    echo "  the old results."
    echo "For much more detail on size drifts refer to the summary report."
    exit
fi


#
#   Exclude certain path patterns.
#   Be sure to modify the grep command below as well.
#
EXCLUDE_PATTERN_01="test"
EXCLUDE_PATTERN_02="tools"
EXCLUDE_PATTERN_03="dbg"
EXCLUDE_PATTERN_04="sample"

EXCLUDE_NAME_01="mkdepend.map"
EXCLUDE_NAME_02="IBMNEC.map"


#
#   Stash our arguments away.
#
COPYSORTTSV="$1"
OLDTSVFILE="$2"
SUMMARYFILE="$3"


#
#   Create our temporary directory.
#
MYTMPDIR=`mktemp -d ./codesighs.tmp.XXXXXXXX`


#
#   Find all map files.
#
ALLMAPSFILE="$MYTMPDIR/allmaps.list"
find ./mozilla -type f -name *.map > $ALLMAPSFILE


#
#   Reduce the map files to a revelant set.
#
NOPATMAPSFILE="$MYTMPDIR/nopatmaps.list"
grep -vi $EXCLUDE_PATTERN_01 < $ALLMAPSFILE | grep -vi $EXCLUDE_PATTERN_02 | grep -vi $EXCLUDE_PATTERN_03 | grep -vi $EXCLUDE_PATTERN_04 > $NOPATMAPSFILE
MAPSFILE="$MYTMPDIR/maps.list"
grep -vi $EXCLUDE_NAME_01 < $NOPATMAPSFILE | grep -vi $EXCLUDE_NAME_02 > $MAPSFILE


#
#   Produce the TSV output.
#
RAWTSVFILE="$MYTMPDIR/raw.tsv"
xargs -n 1 ./mozilla/dist/bin/msmap2tsv --input < $MAPSFILE > $RAWTSVFILE


#
#   Sort the TSV output for useful diffing and eyeballing in general.
#
sort -r $RAWTSVFILE > $COPYSORTTSV


#
#   If a historical file was specified, diff it with our sorted tsv values.
#   Run it through a tool to summaries the diffs to the module
#       level report.
#
rm -f $SUMMARYFILE
DIFFFILE="$MYTMPDIR/diff.txt"
if [ -e $OLDTSVFILE ]; then
  diff $OLDTSVFILE $COPYSORTTSV > $DIFFFILE
  ./mozilla/dist/bin/maptsvdifftool --input $DIFFFILE >> $SUMMARYFILE
  echo "" >>  $SUMMARYFILE
  echo "" >>  $SUMMARYFILE
fi


#
#   Generate the module level report from our new data.
#
./mozilla/dist/bin/codesighs --modules --input $COPYSORTTSV >> $SUMMARYFILE


#
#   Output our numbers, that will let tinderbox specify everything all
#       at once.
#   First number is in fact the total size of all code and data in the map
#       files parsed.
#   Second number, if present, is growth/shrinkage.
#
./mozilla/dist/bin/codesighs --totalonly --input $COPYSORTTSV
if [ -e $DIFFFILE ]; then
    ./mozilla/dist/bin/maptsvdifftool --summary --input $DIFFFILE
fi

#
#   Remove our temporary directory.
#
rm -rf $MYTMPDIR
