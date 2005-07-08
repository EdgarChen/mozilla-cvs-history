#!c:\perl\bin\perl
# 
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1998-1999
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Sean Su <ssu@netscape.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either of the GNU General Public License Version 2 or later (the "GPL"),
# or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

#
# This perl script builds the xpi, config.ini, and js files.
#

use Cwd;
use File::Copy;
use File::Path;
use File::Basename;

$DEPTH = "../../..";
$topsrcdir = GetTopSrcDir();

# ensure that Packager.pm is in @INC, since we might not be called from
# mozilla/xpinstall/packager
push(@INC, "$topsrcdir/xpinstall/packager");
require StageUtils;
require "$topsrcdir/config/zipcfunc.pl";

$seiFileNameGeneric       = "stubinstall.exe";
$seiFileNameSpecific      = "seamonkey-os2-installer.exe";
$seiStubRootName          = "seamonkey-os2-stub-installer";
$seiFileNameSpecificStub  = "$seiStubRootName.exe";
$seuFileNameSpecific      = "seamonkeyUninstall.exe";
$seuzFileNameSpecific     = "seamonkeyuninstall.zip";

$seiFileNameGenericRes       = "stubinstall.res";
$seiFileNameSpecificRes  = "seamonkey-os2-installer.res";
$seiFileNameSpecificRC  = "seamonkey-os2-installer.rc";
$seiFileNameSpecificStubRC = "$seiStubRootName.rc";
$seiFileNameSpecificStubRes  = "$seiStubRootName.res";
$seuFileNameSpecificRes   = "SeaMonkeyUninstall.res";
$seuFileNameSpecificRC  = "SeaMonkeyUninstall.rc";

ParseArgv(@ARGV);

$topobjdir                = "$topsrcdir"                 if !defined($topobjdir);
$inStagePath              = "$topobjdir/stage"           if !defined($inStagePath);
$inDistPath               = "$topobjdir/dist"            if !defined($inDistPath);
$inXpiURL                 = "ftp://not.supplied.invalid" if !defined($inXpiURL);
$inRedirIniURL            = $inXpiURL                    if !defined($inRedirIniURL);

if(defined($ENV{DEBUG_INSTALLER_BUILD}))
{
  print " os2/makeall.pl\n";
  print "   topobjdir  : $topobjdir\n";
  print "   topsrcdir  : $topsrcdir\n";
  print "   inStagePath: $inStagePath\n";
  print "   inDistPath : $inDistPath\n";
}

$gDefaultProductVersion   = StageUtils::GetProductY2KVersion($topobjdir, $topsrcdir, $topsrcdir);

print "\n";
print " Building SeaMonkey\n";
print "  Raw version id   : $gDefaultProductVersion\n";

# $gDefaultProductVersion has the form maj.min.release.bld where maj, min, release
#   and bld are numerics representing version information.
# Other variables need to use parts of the version info also so we'll
#   split out the dot separated values into the array @versionParts
#   such that:
#
#   $versionParts[0] = maj
#   $versionParts[1] = min
#   $versionParts[2] = release
#   $versionParts[3] = bld
@versionParts = split /\./, $gDefaultProductVersion;

# We allow non-numeric characters to be included as the last 
#   characters in fields of $gDefaultProductVersion for display purposes (mostly to
#   show that we have moved past a certain version by adding a '+'
#   character).  Non-numerics must be stripped out of $gDefaultProductVersion,
#   however, since this variable is used to identify the the product 
#   for comparison with other installations, so the values in each field 
#   must be numeric only:
$gDefaultProductVersion =~ s/[^0-9.][^.]*//g;

# set environment vars for use by other .pl scripts called from this script.
if($versionParts[2] eq "0")
{
  $versionMain = "$versionParts[0].$versionParts[1]";
}
else
{
  $versionMain = "$versionParts[0].$versionParts[1].$versionParts[2]";
}

print "  Display version  : $versionMain\n";
print "  Xpinstall version: $gDefaultProductVersion\n";
print "\n";

$gDirPackager         = "$topsrcdir/xpinstall/packager";
$gDirStageProduct     = "$inStagePath/mozilla";
$gDirDistInstall      = "$inDistPath/install";

if(defined($ENV{DEBUG_INSTALLER_BUILD}))
{
  print " back in os2/makeall.pl\n";
  print "   inStagePath: $inStagePath\n";
  print "   inDistPath : $inDistPath\n";
}

# Create the stage area here.
# If -sd is not used, the default stage dir will be: $topobjdir/stage
if(system("perl \"$gDirPackager/make_stage.pl\" -pn mozilla -os os2 -sd \"$inStagePath\" -dd \"$inDistPath\""))
{
  die "\n Error: perl \"$gDirPackager/make_stage.pl\" -pn mozilla -os os2 -sd \"$inStagePath\" -dd \"$inDistPath\"\n";
}

$versionLanguage               = "en";
$ENV{WIZ_nameCompany}          = "mozilla.org";
$ENV{WIZ_nameProduct}          = "SeaMonkey";
$ENV{WIZ_nameProductInternal}  = "SeaMonkey"; # product name without the version string
$ENV{WIZ_fileMainExe}          = "seamonkey.exe";
$ENV{WIZ_fileUninstall}        = $seuFileNameSpecific;
$ENV{WIZ_fileUninstallZip}     = $seuzFileNameSpecific;
# The following variables are for displaying version info in the 
# the installer.
$ENV{WIZ_userAgent}            = "$versionMain ($versionLanguage)";
$ENV{WIZ_userAgentShort}       = "$versionMain";
$ENV{WIZ_xpinstallVersion}     = "$gDefaultProductVersion";
$ENV{WIZ_distInstallPath}      = "$gDirDistInstall";

# GetProductBuildID() will return the build id for GRE located here:
#      NS_BUILD_ID in nsBuildID.h: 2003030610
$ENV{WIZ_greBuildID}       = StageUtils::GetProductBuildID("$inDistPath/include/nsBuildID.h", "NS_BUILD_ID");

# GetGreFileVersion() will return the actual version of xpcom.dll used by GRE.
#  ie:
#      given milestone.txt : 1.4a
#      given nsBuildID.h   : 2003030610
#      gre version would be: 1.4.20030.30610
$ENV{WIZ_greFileVersion}       = StageUtils::GetGreFileVersion($topobjdir, $topsrcdir);

# GetGreSpecialID() will return the GRE ID to be used in the windows registry.
# This ID is also the same one being querried for by the seamonkey glue code.
#  ie:
#      given milestone.txt    : 1.4a
#      given nsBuildID.h      : 2003030610
#      gre special ID would be: 1.4a_2003030610
$ENV{WIZ_greUniqueID}          = StageUtils::GetGreSpecialID($topobjdir);

print "\n";
print " GRE build id       : $ENV{WIZ_greBuildID}\n";
print " GRE file version   : $ENV{WIZ_greFileVersion}\n";
print " GRE special version: $ENV{WIZ_greUniqueID}\n";
print "\n";
print " Building $ENV{WIZ_nameProduct} $ENV{WIZ_userAgent}...\n";
print "\n";

# Check for existence of staging path
if(!(-d "$gDirStageProduct"))
{
  die "\n Invalid path: $gDirStageProduct\n";
}

# List of components for to create xpi files from
@gComponentList = ("xpcom",
                   "browser",
                   "mail",
                   "spellcheck",
                   "psm",
                   "chatzilla",
                   "deflenus",
                   "langenus",
                   "regus",
                   "venkman",
                   "inspector",
                   "reporter");

if(VerifyComponents()) # return value of 0 means no errors encountered
{
  exit(1);
}

# Make sure gDirDistInstall exists
if(!(-d "$gDirDistInstall"))
{
  mkdir ("$gDirDistInstall",0775);
}

if(-d "$gDirDistInstall/xpi")
{
  unlink <$gDirDistInstall/xpi/*>;
}
else
{
  mkdir ("$gDirDistInstall/xpi",0775);
}

if(-d "$gDirDistInstall/uninstall")
{
  unlink <$gDirDistInstall/uninstall/*>;
}
else
{
  mkdir ("$gDirDistInstall/uninstall",0775);
}

if(-d "$gDirDistInstall/setup")
{
  unlink <$gDirDistInstall/setup/*>;
}
else
{
  mkdir ("$gDirDistInstall/setup",0775);
}

if(MakeXpiFile())
{
  exit(1);
}
if(MakeUninstall())
{
  exit(1);
}
if(MakeConfigFile())
{
  exit(1);
}

# Copy the setup files to the dist setup directory.
os2copy("install.ini", "$gDirDistInstall") || 
  die "copy install.ini $gDirDistInstall: $!\n";
os2copy("install.ini", "$gDirDistInstall/setup") ||
  die "copy install.ini $gDirDistInstall/setup: $!\n";
os2copy("config.ini", "$gDirDistInstall") ||
  die "copy config.ini $gDirDistInstall: $!\n";
os2copy("config.ini", "$gDirDistInstall/setup") ||
  die "copy config.ini $gDirDistInstall/setup: $!\n";
os2copy("$gDirDistInstall/setup.exe", "$gDirDistInstall/setup") ||
  die "copy $gDirDistInstall/setup.exe $gDirDistInstall/setup: $!\n";
os2copy("$gDirDistInstall/setuprsc.dll", "$gDirDistInstall/setup") ||
  die "copy $gDirDistInstall/setuprsc.dll $gDirDistInstall/setup: $!\n";

# copy license file for the installer
os2copy("$topsrcdir/LICENSE", "$gDirDistInstall/license.txt") ||
  die "copy $topsrcdir/LICENSE $gDirDistInstall/license.txt: $!\n";
os2copy("$topsrcdir/LICENSE", "$gDirDistInstall/setup/license.txt") ||
  die "copy $topsrcdir/LICENSE $gDirDistInstall/setup/license.txt: $!\n";


# copy the lean installer to stub\ dir
print "\n****************************\n";
print "*                          *\n";
print "*  creating Stub files...  *\n";
print "*                          *\n";
print "****************************\n";
print "\n $gDirDistInstall/stub/$seiFileNameSpecificStub\n";

# build the self-extracting .exe (installer) file.
os2copy("$gDirDistInstall/$seiFileNameGeneric", "$gDirDistInstall/$seiFileNameSpecificStub") ||
  die "copy $gDirDistInstall/$seiFileNameGeneric $gDirDistInstall/$seiFileNameSpecificStub: $!\n";

os2copy("$gDirDistInstall/$seiFileNameGenericRes", "$gDirDistInstall/$seiFileNameSpecificStubRes") ||
  die "copy $gDirDistInstall/$seiFileNameGenericRes $gDirDistInstall/$seiFileNameSpecificStubRes: $!\n";

@stubFiles = <$gDirDistInstall/setup/*.*>;

$size = (-s "$gDirDistInstall/$seiFileNameSpecificStubRes");
truncate("$gDirDistInstall/$seiFileNameSpecificStubRes", "$size-1");
open(OUTPUTFILE, ">$gDirDistInstall/$seiFileNameSpecificStubRC");
print OUTPUTFILE "#include <os2.h>\n";
print OUTPUTFILE "STRINGTABLE DISCARDABLE\n";
print OUTPUTFILE "BEGIN\n";
$currentResourceID = 10000+1;
foreach $entry ( @stubFiles ) 
{
  $filename = basename($entry);
  print OUTPUTFILE "$currentResourceID \"$filename\"\n";
  $currentResourceID++;
}
print OUTPUTFILE "END\n";
$currentResourceID = 10000+1;
foreach $entry ( @stubFiles ) 
{
  print OUTPUTFILE "RESOURCE RT_RCDATA $currentResourceID \"$entry\"\n";
  $currentResourceID++;
}
close(OUTPUTFILE);
if(defined($ENV{DEBUG_INSTALLER_BUILD}))
{
  print "rc -r $gDirDistInstall/$seiFileNameSpecificStubRC $gDirDistInstall/temp.res\n";
}
system("rc -r $gDirDistInstall/$seiFileNameSpecificStubRC $gDirDistInstall/temp.res");
if(defined($ENV{DEBUG_INSTALLER_BUILD}))
{
  print "cat $gDirDistInstall/$seiFileNameSpecificStubRes $gDirDistInstall/temp.res > $gDirDistInstall/new.res\n";
}
system("cat $gDirDistInstall/$seiFileNameSpecificStubRes $gDirDistInstall/temp.res > $gDirDistInstall/new.res");
unlink("$gDirDistInstall/$seiFileNameSpecificStubRes");
rename("$gDirDistInstall/new.res", "$gDirDistInstall/$seiFileNameSpecificStubRes");
unlink("$gDirDistInstall/temp.res");
system("rc $gDirDistInstall/$seiFileNameSpecificStubRes $gDirDistInstall/$seiFileNameSpecificStub");

if(-d "$gDirDistInstall/stub")
{
  unlink <$gDirDistInstall/stub/*>;
}
else
{
  mkdir ("$gDirDistInstall/stub",0775);
}
os2copy("$gDirDistInstall/$seiFileNameSpecificStub", "$gDirDistInstall/stub") ||
  die "copy $gDirDistInstall/$seiFileNameSpecificStub $gDirDistInstall/stub: $!\n";

# create the xpi for launching the stub installer
print "\n************************************\n";
print "*                                  *\n";
print "*  creating stub installer xpi...  *\n";
print "*                                  *\n";
print "************************************\n";
print "\n $gDirDistInstall/$seiStubRootName.xpi\n\n";

if(-d "$gDirStageProduct/$seiStubRootName")
{
  unlink <$gDirStageProduct/$seiStubRootName/*>;
}
else
{
  mkdir ("$gDirStageProduct/$seiStubRootName",0775);
}
os2copy("$gDirDistInstall/stub/$seiFileNameSpecificStub", "$gDirStageProduct/$seiStubRootName") ||
  die "copy $gDirDistInstall/stub/$seiFileNameSpecificStub $gDirStageProduct/$seiStubRootName: $!\n";

# Make .js files
if(MakeJsFile($seiStubRootName))
{
  return(1);
}

# Make .xpi file
if(system("perl makexpi.pl $seiStubRootName $gDirStageProduct $gDirDistInstall"))
{
  print "\n Error: perl makexpi.pl $seiStubRootName $gDirStageProduct $gDirDistInstall\n";
  return(1);
}

# group files for CD
print "\n************************************\n";
print "*                                  *\n";
print "*  creating Compact Disk files...  *\n";
print "*                                  *\n";
print "************************************\n";
print "\n $gDirDistInstall/cd\n";

if(-d "$gDirDistInstall/cd")
{
  unlink <$gDirDistInstall/cd/*>;
}
else
{
  mkdir ("$gDirDistInstall/cd",0775);
}

os2copy("$gDirDistInstall/$seiFileNameSpecificStub", "$gDirDistInstall/cd") ||
  die "copy $gDirDistInstall/$seiFileNameSpecificStub $gDirDistInstall/cd: $!\n";

StageUtils::CopyFiles("$gDirDistInstall/xpi", "$gDirDistInstall/cd");

# create the big self extracting .exe installer
print "\n**************************************************************\n";
print "*                                                            *\n";
print "*  creating Self Extracting Executable Full Install file...  *\n";
print "*                                                            *\n";
print "**************************************************************\n";
print "\n $gDirDistInstall/$seiFileNameSpecific\n";

if(-d "$gDirDistInstall/sea")
{
  unlink <$gDirDistInstall/sea/*>;
}
else
{
  mkdir ("$gDirDistInstall/sea",0775);
}
os2copy("$gDirDistInstall/$seiFileNameGeneric", "$gDirDistInstall/$seiFileNameSpecific") ||
  die "copy $gDirDistInstall/$seiFileNameGeneric $gDirDistInstall/$seiFileNameSpecific: $!\n";

os2copy("$gDirDistInstall/$seiFileNameGenericRes", "$gDirDistInstall/$seiFileNameSpecificRes") ||
  die "copy $gDirDistInstall/$seiFileNameGenericRes $gDirDistInstall/$seiFileNameSpecificRes: $!\n";

@stubFiles = <$gDirDistInstall/setup/*.*>;
@xpiFiles = <$gDirDistInstall/xpi/*.*>;

$size = (-s "$gDirDistInstall/$seiFileNameSpecificRes");
truncate("$gDirDistInstall/$seiFileNameSpecificRes", "$size-1");
open(OUTPUTFILE, ">$gDirDistInstall/$seiFileNameSpecificRC");
print OUTPUTFILE "#include <os2.h>\n";
print OUTPUTFILE "STRINGTABLE DISCARDABLE\n";
print OUTPUTFILE "BEGIN\n";
$currentResourceID = 10000+1;
foreach $entry ( @stubFiles ) 
{
  $filename = basename($entry);
  print OUTPUTFILE "$currentResourceID \"$filename\"\n";
  $currentResourceID++;
}
foreach $entry ( @xpiFiles ) 
{
  $filename = basename($entry);
  print OUTPUTFILE "$currentResourceID \"$filename\"\n";
  $currentResourceID++;
}
print OUTPUTFILE "END\n";
$currentResourceID = 10000+1;
foreach $entry ( @stubFiles ) 
{
  print OUTPUTFILE "RESOURCE RT_RCDATA $currentResourceID \"$entry\"\n";
  $currentResourceID++;
}
foreach $entry ( @xpiFiles ) 
{
  print OUTPUTFILE "RESOURCE RT_RCDATA $currentResourceID \"$entry\"\n";
  $currentResourceID++;
}
close(OUTPUTFILE);
system("rc -r $gDirDistInstall/$seiFileNameSpecificRC $gDirDistInstall/temp.res");
system("cat $gDirDistInstall/$seiFileNameSpecificRes $gDirDistInstall/temp.res > $gDirDistInstall/new.res");
unlink("$gDirDistInstall/$seiFileNameSpecificRes");
rename("$gDirDistInstall/new.res", "$gDirDistInstall/$seiFileNameSpecificRes");
unlink("$gDirDistInstall/temp.res");
system("rc $gDirDistInstall/$seiFileNameSpecificRes $gDirDistInstall/$seiFileNameSpecific");

os2copy("$gDirDistInstall/$seiFileNameSpecific", "$gDirDistInstall/sea") ||
  die "copy $gDirDistInstall/$seiFileNameSpecific $gDirDistInstall/sea: $!\n";

unlink <$gDirDistInstall/$seiFileNameSpecificStub>;

print " done!\n\n";

# end of script
exit(0);

sub MakeExeZip
{
  my($aSrcDir, $aExeFile, $aZipFile) = @_;
  my($saveCwdir);

  $saveCwdir = cwd();
  chdir($aSrcDir);
  if(system("zip $gDirDistInstall/xpi/$aZipFile $aExeFile"))
  {
    chdir($saveCwdir);
    die "\n Error: zip $gDirDistInstall/xpi/$aZipFile $aExeFile";
  }
  chdir($saveCwdir);
}

sub PrintUsage
{
  die "usage: $0 [options]

       options include:

           -objDir <path>            : path to the objdir.  default is topsrcdir

           -stagePath <staging path> : full path to where the seamonkey components are staged at
                                       Default stage path, if this is not set, is:
                                         [seamonkey]/stage

           -distPath <dist path>     : full path to where the seamonkey dist dir is at.
                                       Default stage path, if this is not set, is:
                                         [seamonkey]/dist

           -aurl <archive url>       : either ftp:// or http:// url to where the
                                       archives (.xpi, .exe, .zip, etc...) reside

           -rurl <redirect.ini url>  : either ftp:// or http:// url to where the
                                       redirec.ini resides.  If not supplied, it
                                       will be assumed to be the same as archive
                                       url.
       \n";
}

sub ParseArgv
{
  my(@myArgv) = @_;
  my($counter);

  for($counter = 0; $counter <= $#myArgv; $counter++)
  {
    if($myArgv[$counter] =~ /^[-,\/]h$/i)
    {
      PrintUsage();
    }
    elsif($myArgv[$counter] =~ /^[-,\/]objDir$/i)
    {
      if($#myArgv >= ($counter + 1))
      {
        ++$counter;
        $topobjdir = $myArgv[$counter];
        $topobjdir =~ s/\\/\//g;
      }
    }
    elsif($myArgv[$counter] =~ /^[-,\/]stagePath$/i)
    {
      if($#myArgv >= ($counter + 1))
      {
        ++$counter;
        $inStagePath = $myArgv[$counter];
        $inStagePath =~ s/\\/\//g;
      }
    }
    elsif($myArgv[$counter] =~ /^[-,\/]distPath$/i)
    {
      if($#myArgv >= ($counter + 1))
      {
        ++$counter;
        $inDistPath = $myArgv[$counter];
        $inDistPath =~ s/\\/\//g;
      }
    }
    elsif($myArgv[$counter] =~ /^[-,\/]aurl$/i)
    {
      if($#myArgv >= ($counter + 1))
      {
        ++$counter;
        $inXpiURL = $myArgv[$counter];
        $inRedirIniURL = $inXpiURL;
      }
    }
    elsif($myArgv[$counter] =~ /^[-,\/]rurl$/i)
    {
      if($#myArgv >= ($counter + 1))
      {
        ++$counter;
        $inRedirIniURL = $myArgv[$counter];
      }
    }
  }
}

sub MakeConfigFile
{
  chdir("$gDirPackager/os2");
  # Make config.ini file
  if(system("perl makecfgini.pl config.it $gDefaultProductVersion $gDirStageProduct $gDirDistInstall/xpi $inRedirIniURL $inXpiURL"))
  {
    print "\n Error: perl makecfgini.pl config.it $gDefaultProductVersion $gDirStageProduct $gDirDistInstall/xpi $inRedirIniURL $inXpiURL\n";
    return(1);
  }

  # Make install.ini file
  if(system("perl makecfgini.pl install.it $gDefaultProductVersion $gDirStageProduct $gDirDistInstall/xpi $inRedirIniURL $inXpiURL"))
  {
    print "\n Error: perl makecfgini.pl install.it $gDefaultProductVersion $gDirStageProduct $gDirDistInstall/xpi $inRedirIniURL $inXpiURL\n";
    return(1);
  }
  return(0);
}

sub MakeUninstall
{
  chdir("$gDirPackager/os2");
  if(MakeUninstallIniFile())
  {
    return(1);
  }

  # Copy the uninstall files to the dist uninstall directory.
  os2copy("uninstall.ini", "$gDirDistInstall") ||
    die "copy uninstall.ini $gDirDistInstall: $!\n";
  os2copy("uninstall.ini", "$gDirDistInstall/uninstall") ||
    die "copy uninstall.ini $gDirDistInstall/uninstall: $!\n";
  os2copy("$gDirDistInstall/uninstall.exe", "$gDirDistInstall/uninstall") ||
    die "copy $gDirDistInstall/uninstall.exe $gDirDistInstall/uninstall: $!\n";

  # build the self-extracting .exe (uninstaller) file.
  print "\nbuilding self-extracting uninstaller ($seuFileNameSpecific)...\n";
  os2copy("$gDirDistInstall/$seiFileNameGeneric", "$gDirDistInstall/$seuFileNameSpecific") ||
    die "copy $gDirDistInstall/$seiFileNameGeneric $gDirDistInstall/$seuFileNameSpecific: $!\n";

  print "copy $gDirDistInstall/$seiFileNameGeneric $gDirDistInstall/$seuFileNameSpecific: $!\n";
  print "copy $gDirDistInstall/$seiFileNameGeneric $gDirDistInstall/$seuFileNameSpecific: $!\n";
  print "copy $gDirDistInstall/$seiFileNameGeneric $gDirDistInstall/$seuFileNameSpecific: $!\n";

  os2copy("$gDirDistInstall/$seiFileNameGenericRes", "$gDirDistInstall/$seuFileNameSpecificRes") ||
    die "copy $gDirDistInstall/$seiFileNameGenericRes $gDirDistInstall/$seuFileNameSpecificRes: $!\n";
  print "copy $gDirDistInstall/$seiFileNameGenericRes $gDirDistInstall/$seuFileNameSpecificRes: $!\n";
  print "copy $gDirDistInstall/$seiFileNameGenericRes $gDirDistInstall/$seuFileNameSpecificRes: $!\n";
  print "copy $gDirDistInstall/$seiFileNameGenericRes $gDirDistInstall/$seuFileNameSpecificRes: $!\n";

  @stubFiles = <$gDirDistInstall/uninstall/*.*>;

  $size = (-s "$gDirDistInstall/$seuFileNameSpecificRes");
  truncate("$gDirDistInstall/$seuFileNameSpecificRes", "$size-1");
  open(OUTPUTFILE, ">$gDirDistInstall/$seuFileNameSpecificRC");
  print OUTPUTFILE "#include <os2.h>\n";
  print OUTPUTFILE "STRINGTABLE DISCARDABLE\n";
  print OUTPUTFILE "BEGIN\n";
  $currentResourceID = 10000+1;
  foreach $entry ( @stubFiles ) 
  {
    $filename = basename($entry);
    print OUTPUTFILE "$currentResourceID \"$filename\"\n";
    $currentResourceID++;
  }
  print OUTPUTFILE "END\n";
  $currentResourceID = 10000+1;
  foreach $entry ( @stubFiles ) 
  {
    print OUTPUTFILE "RESOURCE RT_RCDATA $currentResourceID \"$entry\"\n";
    $currentResourceID++;
  }
  close(OUTPUTFILE);
  system("rc -r $gDirDistInstall/$seuFileNameSpecificRC $gDirDistInstall/temp.res");
  system("cat $gDirDistInstall/$seuFileNameSpecificRes $gDirDistInstall/temp.res > $gDirDistInstall/new.res");
  unlink("$gDirDistInstall/$seuFileNameSpecificRes");
  rename("$gDirDistInstall/new.res", "$gDirDistInstall/$seuFileNameSpecificRes");
  unlink("$gDirDistInstall/temp.res");
  system("rc $gDirDistInstall/$seuFileNameSpecificRes $gDirDistInstall/$seuFileNameSpecific");

  MakeExeZip($gDirDistInstall, $seuFileNameSpecific, $seuzFileNameSpecific);
  unlink <$gDirDistInstall/$seuFileNameSpecific>;
  return(0);
}

sub MakeUninstallIniFile
{
  # Make config.ini file
  if(system("perl makeuninstallini.pl uninstall.it $gDefaultProductVersion"))
  {
    print "\n Error: perl makeuninstallini.pl uninstall.it $gDefaultProductVersion\n";
    return(1);
  }
  return(0);
}

sub MakeJsFile
{
  my($mComponent) = @_;

  # Make .js file
  chdir("$gDirPackager/os2");
  if(system("perl makejs.pl $mComponent.jst $gDefaultProductVersion $gDirStageProduct/$mComponent"))
  {
    print "\n Error: perl makejs.pl $mComponent.jst $gDefaultProductVersion $gDirStageProduct/$mComponent\n";
    return(1);
  }
  return(0);
}

sub MakeXpiFile
{
  my($mComponent);

  chdir("$gDirPackager/os2");
  foreach $mComponent (@gComponentList)
  {
    # Make .js files
    if(MakeJsFile($mComponent))
    {
      return(1);
    }

    # Make .xpi file
    if(system("perl makexpi.pl $mComponent $gDirStageProduct $gDirDistInstall/xpi"))
    {
      print "\n Error: perl makexpi.pl $mComponent $gDirStageProduct $gDirDistInstall/xpi\n";
      return(1);
    }
  }
  return(0);
}

sub VerifyComponents()
{
  my($mComponent);
  my($mError) = 0;

  print "\n Verifying existence of required components...\n";
  foreach $mComponent (@gComponentList)
  {
    if($mComponent =~ /talkback/i)
    {
      print " place holder: $gDirStageProduct/$mComponent\n";
      mkdir("$gDirStageProduct/$mComponent", 0775);
    }
    elsif(-d "$gDirStageProduct/$mComponent")
    {
      print "           ok: $gDirStageProduct/$mComponent\n";
    }
    else
    {
      print "        Error: $gDirStageProduct/$mComponent does not exist!\n";
      $mError = 1;
    }
  }
  print "\n";
  return($mError);
}

sub GetTopSrcDir
{
  my($rootDir) = dirname($0) . "/$DEPTH";
  my($savedCwdDir) = cwd();

  chdir($rootDir);
  $rootDir = cwd();
  chdir($savedCwdDir);
  return($rootDir);
}

sub os2copy
{
  my($source, $dest) = @_;
  system("cp $source $dest");
  return 1;
}
