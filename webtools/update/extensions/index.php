<?php
// ***** BEGIN LICENSE BLOCK *****
// Version: MPL 1.1/GPL 2.0/LGPL 2.1
//
// The contents of this file are subject to the Mozilla Public License Version
// 1.1 (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
// http://www.mozilla.org/MPL/
//
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
// for the specific language governing rights and limitations under the
// License.
//
// The Original Code is Mozilla Update.
//
// The Initial Developer of the Original Code is
// Chris "Wolf" Crews.
// Portions created by the Initial Developer are Copyright (C) 2004
// the Initial Developer. All Rights Reserved.
//
// Contributor(s):
//   Chris "Wolf" Crews <psychoticwolf@carolina.rr.com>
//
// Alternatively, the contents of this file may be used under the terms of
// either the GNU General Public License Version 2 or later (the "GPL"), or
// the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
// in which case the provisions of the GPL or the LGPL are applicable instead
// of those above. If you wish to allow use of your version of this file only
// under the terms of either the GPL or the LGPL, and not to allow others to
// use your version of this file under the terms of the MPL, indicate your
// decision by deleting the provisions above and replace them with the notice
// and other provisions required by the GPL or the LGPL. If you do not delete
// the provisions above, a recipient may use your version of this file under
// the terms of any one of the MPL, the GPL or the LGPL.
//
// ***** END LICENSE BLOCK *****
?>
<?php
require"../core/config.php";
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html401/loose.dtd">
<html lang="EN" dir="ltr">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
    <meta http-equiv="Content-Language" content="en">
    <meta http-equiv="Content-Style-Type" content="text/css">

<TITLE>Mozilla Update :: Extensions - Add Features to Mozilla Software</TITLE>

<?php
include"$page_header";
?>

<div id="mBody">
<?php
$type = "E";
if ($_GET["application"]) {$application=$_GET["application"]; }

$index="yes";
include"inc_sidebar.php";
?>

<div id="mBody">
<h3>What is an Extension?</h3>
<p>Extensions are small add-ons that add new functionality to <?php print(ucwords($application)); ?>.
They can add anything from a toolbar button to a completely new feature. They allow the browser to be customized to fit the
personal needs of each user if they need addtional features<?php if ($application !=="mozilla") { ?>, while keeping <?php print(ucwords($application)); ?> small
to download <?php } ?>.</p>

<?php
//Temporary!! Current Version Array Code
$currentver_array = array("firefox"=>"1.0", "thunderbird"=>"0.9", "mozilla"=>"1.7");
$currentver_display_array = array("firefox"=>"1.0", "thunderbird"=>"0.9", "mozilla"=>"1.7.x");
$currentver = $currentver_array[$application];
$currentver_display = $currentver_display_array[$application];
?>

  <!-- Start News Columns -->
  <div class="frontcolumn">
<a href="http://www.mozilla.org/news.rdf"><img src="../images/rss.png" width="28" height="16" class="rss" alt="Mozilla News in RSS"></a><h2 style="margin-top: 0;"><a href="showlist.php?application=<?php echo uriparams(); ?>&category=Newest" title="New Extensions on Mozilla Update">New Additions</a></h2>
<span class="newsSubline">New and Updated Extensions</span>
<ul class="news">

<?php
$i=0;
$sql = "SELECT TM.ID, TV.vID, TM.Name, TV.Version, TV.DateAdded
FROM  `t_main` TM
INNER  JOIN t_version TV ON TM.ID = TV.ID
INNER  JOIN t_applications TA ON TV.AppID = TA.AppID
INNER  JOIN t_os TOS ON TV.OSID = TOS.OSID
WHERE  `Type`  =  '$type' AND `AppName` = '$application' AND (`OSName` = '$_SESSION[app_os]' OR `OSName` = 'ALL') AND `approved` = 'YES' ORDER BY `DateAdded` DESC ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
   $i++;
   $id = $row["ID"];
   $vid = $row["vID"];
   $name = $row["Name"];
   $version = $row["Version"];
   $dateadded = $row["DateAdded"];
//Create Customizeable Datestamp
    $timestamp = strtotime("$dateadded");
    $dateadded = gmdate("M d", $timestamp); //    $dateupdated = gmdate("F d, Y g:i:sa T", $timestamp);

if ($lastname == $name) {$i--; continue; }
  echo"<li>\n";
  echo"<div class=\"date\">$dateadded</div>\n";
  echo"<a href=\"moreinfo.php?".uriparams()."&id=$id\">$name $version</a><BR>\n";
  echo"</li>\n";

$lastname = $name;
if ($i >= "5") {break;}
}
?>
</ul>
</div>
<div class="frontcolumn">
<a href="http://planet.mozilla.org/rss10.xml"><img src="../images/rss.png" width="28" height="16" class="rss" alt="Mozilla Weblogs in RSS"></a><h2 style="margin-top: 0;"><a href="showlist.php?<?php echo uriparams(); ?>&category=Popular" title="Most Popular Extensions, based on Downloads over the last week">Most Popular</a></h2>
<span class="newsSubline">Downloads over the last week</span>
<ul class="news">

<?php
$i=0;
$sql = "SELECT TM.ID, TV.vID,TM.Name, TV.Version, TM.TotalDownloads, TM.downloadcount
FROM  `t_main` TM
INNER  JOIN t_version TV ON TM.ID = TV.ID
INNER  JOIN t_applications TA ON TV.AppID = TA.AppID
WHERE  `Type`  =  '$type' AND `AppName` = '$application' AND `minAppVer_int` <='$currentver' AND `maxAppVer_int` >= '$currentver' AND `downloadcount` > '0' AND `approved` = 'YES' ORDER BY `downloadcount` DESC ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
   $i++;
   $id = $row["ID"];
   $vid = $row["vID"];
   $name = $row["Name"];
   $version = $row["Version"];
   $downloadcount = $row["downloadcount"];
   $totaldownloads = $row["TotalDownloads"];
if ($lastname == $name) {$i--; continue; }
  echo"<li>\n";
  echo"<div class=\"date\">$i</div>\n";
  echo"<a href=\"moreinfo.php?".uriparams()."&id=$id\">$name</a><br>\n";
  echo"<span class=\"newsSubline\">($downloadcount downloads)</span>\n";
  echo"</li>\n";
$lastname = $name;
if ($i >= "5") {break;}
}
?>
</ul>
</div>

<div class="frontcolumn">
<a href="http://www.mozillazine.org/atom.xml"><img src="../images/rss.png" width="28" height="16" class="rss" alt="MozillaZine News in RSS"></a><h2 style="margin-top: 0;"><a href="showlist.php?<?php echo uriparams(); ?>&category=Top Rated" title="Highest Rated Extensions by the Community">Top Rated</a></h2>
<span class="newsSubline">Based on feedback from visitors</span>
<ul class="news">

<?php
$r=0;
$usednames = array();
$sql = "SELECT TM.ID, TV.vID, TM.Name, TV.Version, TM.Rating, TU.UserName
FROM  `t_main` TM
INNER  JOIN t_version TV ON TM.ID = TV.ID
INNER  JOIN t_applications TA ON TV.AppID = TA.AppID
INNER JOIN t_authorxref TAX ON TAX.ID = TM.ID
INNER JOIN t_userprofiles TU ON TU.UserID = TAX.UserID
WHERE  `Type`  =  '$type' AND `AppName` = '$application' AND `minAppVer_int` <='$currentver' AND `maxAppVer_int` >= '$currentver' AND `Rating` > '0' AND `approved` = 'YES' ORDER BY `Rating` DESC, `Name` ASC, `Version` DESC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
   $r++; $s++;
   $id = $row["ID"];
   $vid = $row["vID"];
   $name = $row["Name"];
   $version = $row["Version"];
   $rating = $row["Rating"];
  $arraysearch = array_search("$name", $usednames);
  if ($arraysearch !== false AND $usedversions[$arraysearch]['version']<$version) {$r--; continue; }

  echo"<li>\n";
  echo"<div class=\"date\">$rating stars</div>\n";
  echo"<a href=\"moreinfo.php?".uriparams()."&id=$id\">$name</a>\n";
  echo"</li>\n";


$usednames[$s] = $name;
$usedversions[$s] = $version;
if ($r >= "5") {break;}
}
unset($usednames, $usedversions, $r, $s, $i);
?>
</ul>
</div>

  <!-- End News Columns -->  
<br style="clear: both;">


</div>
</div>
<BR>
<?php
include"$page_footer";
?>
</BODY>
</HTML>