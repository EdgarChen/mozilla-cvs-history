#!/usr/bin/ksh -p
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
# The Original Code is Sun LDAP C SDK.
# 
# The Initial Developer of the Original Code is Sun Microsystems, 
# Inc. Portions created by Sun Microsystems, Inc are Copyright 
# (C) 2005 Sun Microsystems, Inc.  All Rights Reserved.
# 
# Contributor(s):
# 
# Alternatively, the contents of this file may be used under the
# terms of the GNU General Public License Version 2 or later (the
# "GPL"), in which case the provisions of the GPL are applicable 
# instead of those above.  If you wish to allow use of your 
# version of this file only under the terms of the GPL and not to
# allow others to use your version of this file under the MPL,
# indicate your decision by deleting the provisions above and
# replace them with the notice and other provisions required by
# the GPL.  If you do not delete the provisions above, a recipient
# may use your version of this file under either the MPL or the
# GPL.
# 
#ident	"$Id: bld_awk_pkginfo.ksh,v 1.1 2006/05/19 20:17:34 richm%stanfordalumni.org Exp $"
#
# Simple script which builds the awk_pkginfo awk script.  This awk script
# is used to convert the pkginfo.tmpl files into pkginfo files
# for the build.
#

usage()
{
   cat <<-EOF
usage: bld_awk_pkginfo -p <prodver> -m <mach> -o <awk_script> [-v <version>]
EOF
}

#
# Awk strings
#
# two VERSION patterns: one for Dewey decimal, one for Dewey plus ,REV=n
# the first has one '=' the second has two or more '='
#
VERSION1="VERSION=[^=]*$"
VERSION2="VERSION=[^=]*=.*$"
PRODVERS="^SUNW_PRODVERS="
ARCH='ARCH=\"ISA\"'

#
# parse command line
#
mach=""
prodver=""
awk_script=""
version="JSSVERS"

while getopts o:p:m:v: c
do
   case $c in
   o)
      awk_script=$OPTARG
      ;;
   m)
      mach=$OPTARG
      ;;
   p)
      prodver=$OPTARG
      ;;
   v)
      version=$OPTARG
      ;;
   \?)
      usage
      exit 1
      ;;
   esac
done

if [[ ( -z $prodver ) || ( -z $mach ) || ( -z $awk_script ) ]]
then
   usage
   exit 1
fi

if [[ -f $awk_script ]]
then
	rm -f $awk_script
fi

#
# Build REV= field based on date
#
if [ $mach = "sparc" ]
then
	rev="2005.11.29.16.11"
else
if [ $mach = "i386" ]
then
	rev="2005.11.29.16.11"
fi
fi
rev=$(date "+%Y.%m.%d.%H.%M")

#
# Build awk script which will process all the
# pkginfo.tmpl files.
#
# the first VERSION pattern is replaced with a leading quotation mark
#
rm -f $awk_script
cat << EOF > $awk_script
/$VERSION1/ {
      sub(/\=[^=]*$/,"=\"$rev\"")
      print
      next
   }
/$VERSION2/ {
      sub(/\=[^=]*$/,"=$rev\"")
      sub(/LDAPCSDKVERS/,"$version")
      print
      next
   }
/$PRODVERS/ { 
      printf "SUNW_PRODVERS=\"%s\"\n", "$prodver" 
      next
   }
/$ARCH/ {
      printf "ARCH=\"%s\"\n", "$mach"
      next
   }
{ print }
EOF
