#!/usr/bonsaitools/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-
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
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Stephan Niemz  <st.n@gmx.net>
#                 Christopher Aillon <christopher@aillon.com>

use diagnostics;
use strict;

require "CGI.pl";

ConnectToDatabase();

if (defined $::FORM{'voteon'} || (!defined $::FORM{'bug_id'} &&
                                  !defined $::FORM{'user'})) {
    confirm_login();
    $::FORM{'user'} = DBNameToIdAndCheck($::COOKIE{'Bugzilla_login'});
} else {
  # Check whether or not the user is currently logged in without throwing
  # an error if the user is not logged in. This function sets the value 
  # of $::usergroupset, the binary number that records the set of groups 
  # to which the user belongs and which gets used in ValidateBugID below
  # to determine whether or not the user is authorized to access the bug
  # whose votes are being shown or which is being voted on.
  quietly_check_login();
}

################################################################################
# Begin Data/Security Validation
################################################################################

# Make sure the bug ID is a positive integer representing an existing
# bug that the user is authorized to access.
if (defined $::FORM{'bug_id'}) {
  ValidateBugID($::FORM{'bug_id'});
}

# Make sure the bug ID being voted on is a positive integer representing 
# an existing bug that the user is authorized to access.
if (defined $::FORM{'voteon'}) {
  ValidateBugID($::FORM{'voteon'});
}

# Make sure the user ID is a positive integer representing an existing user.
if (defined $::FORM{'user'}) {
  $::FORM{'user'} =~ /^([1-9][0-9]*)$/
    || DisplayError("The user number is invalid.") 
    && exit;
  SendSQL("SELECT 1 FROM profiles WHERE userid = $::FORM{'user'}");
  FetchSQLData() 
    || DisplayError("User #$::FORM{'user'} does not exist.") 
    && exit;
}

################################################################################
# End Data/Security Validation
################################################################################

print "Content-type: text/html\n\n";

if (defined $::FORM{'bug_id'}) {
    my $id = $::FORM{'bug_id'};
    my $linkedid = qq{<a href="show_bug.cgi?id=$id">$id</a>};
    PutHeader("Show votes", "Show votes", "Bug $linkedid");
    SendSQL("select profiles.login_name, votes.who, votes.count from votes, profiles where votes.bug_id = " . SqlQuote($id) . " and profiles.userid = votes.who");
    print "<table>\n";
    print "<tr><th>Who</th><th>Number of votes</th></tr>\n";
    my $sum = 0;
    while (MoreSQLData()) {
        my ($name, $userid, $count) = (FetchSQLData());
        print qq{<tr><td><a href="showvotes.cgi?user=$userid">$name</a></td><td align=right>$count</td></tr>\n};
        $sum += $count;
    }
    print "</table>";
    print "<p>Total votes: $sum<p>\n";
} elsif (defined $::FORM{'user'}) {
    quietly_check_login();
    GetVersionTable();
    my $who = $::FORM{'user'};
    my $name = DBID_to_name($who);
    PutHeader("Show votes", "Show votes", $name);
    print qq{<form action="doeditvotes.cgi">\n};
    print "<table><tr><td></td><th>Bug \#</th><th>Summary</th><th>Votes</th></tr>\n";
    SendSQL("lock tables bugs read, products read, votes write");
    if (defined($::FORM{'voteon'})) {
        # Oh, boy, what a hack.  Make sure there is an entry for this bug
        # in the vote table, just so that things display right.
        # Yuck yuck yuck.###
        SendSQL("select votes.count from votes where votes.bug_id = $::FORM{'voteon'} and votes.who = $who");
        if (!MoreSQLData()) {
            SendSQL("insert into votes (who, bug_id, count) values ($who, $::FORM{'voteon'}, 0)");
        }
    }
    my $canedit = (defined $::COOKIE{'Bugzilla_login'} &&
                   $::COOKIE{'Bugzilla_login'} eq $name);
    my %maxvotesperbug;
    if( $canedit ) {
        SendSQL("SELECT products.product, products.maxvotesperbug FROM products");
        while (MoreSQLData()) {
            my ($prod, $max) = (FetchSQLData());
            $maxvotesperbug{$prod}= $max;
        }
    }
    foreach my $product (sort(keys(%::prodmaxvotes))) {
        if ($::prodmaxvotes{$product} <= 0) {
            next;
        }
        my $qprod = value_quote($product);
        SendSQL("select votes.bug_id, votes.count, bugs.short_desc, bugs.bug_status from votes, bugs where votes.who = $who and votes.bug_id = bugs.bug_id and bugs.product = " . SqlQuote($product) . "order by votes.bug_id");
        next if !MoreSQLData(); # don't show products without any votes
        my $sum = 0;
        print "<tr><th>$product</th></tr>";
        while (MoreSQLData()) {
            my ($id, $count, $summary, $status) = (FetchSQLData());
            if (!defined $status) {
                next;
            }
            my $opened = IsOpenedState($status);
            my $strike = $opened ? "" : "<strike>";
            my $endstrike = $opened ? "" : "</strike>";
            $summary = html_quote($summary);
            $sum += $count;
            if ($canedit) {
                my $min = min($::prodmaxvotes{$product}, $maxvotesperbug{$product});
                if ($min < 2) { # checkbox
                    my $checked = $count ? ' checked="checked"' : '';
                    $count = qq{<input type="checkbox" name="$id" value="1"$checked>};
                }
                else { # text input
                    my $maxlength = length $min;
                    $count = qq{<input name="$id" value="$count" size="$maxlength" maxlength="$maxlength">};
                }
            }
            print qq{
<tr>
<td></td>
<td>$strike<a href="showvotes.cgi?bug_id=$id">$id</a>$endstrike</td>
<td><a href="show_bug.cgi?id=$id">$summary</a></td>
<td align="right">$count</td>
</tr>
};
        }
        my $plural = (($sum == 1) ? "" : "s");
        print "<td colspan=3>$sum vote$plural used out of\n";
        print "$::prodmaxvotes{$product} allowed.</td>\n";
    }
    print "</table>\n";
    if ($canedit) {
        print qq{<input type=submit value="Submit">\n};
        print "<br>To change your votes, type in new numbers (using zero to\n";
        print "mean no votes) or change the checkbox, and then click <b>Submit</b>.\n";
    }
    print "<input type=hidden name=who value=$who>";
    print "</form>\n";
    SendSQL("delete from votes where count <= 0");
    SendSQL("unlock tables");
}

print qq{<a href="votehelp.html">Help!  I don't understand this voting stuff</a>};

PutFooter();
