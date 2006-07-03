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
# Contributor(s): Terry Weissman <terry@mozilla.org>,
#                 Bryce Nesbitt <bryce-mozilla@nextbus.com>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Alan Raetz <al_raetz@yahoo.com>
#                 Jacob Steenhagen <jake@actex.net>
#                 Matthew Tuck <matty@chariot.net.au>
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 J. Paul Reed <preed@sigkill.com>
#                 Gervase Markham <gerv@gerv.net>
#                 Byron Jones <bugzilla@glob.com.au>

use strict;

package Bugzilla::BugMail;

use Bugzilla::Error;
use Bugzilla::User;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Bug;
use Bugzilla::Product;
use Bugzilla::Component;
use Bugzilla::Mailer;

use Date::Parse;
use Date::Format;

use constant BIT_DIRECT    => 1;
use constant BIT_WATCHING  => 2;

# We need these strings for the X-Bugzilla-Reasons header
# Note: this hash uses "," rather than "=>" to avoid auto-quoting of the LHS.
my %rel_names = (REL_ASSIGNEE          , "AssignedTo", 
                 REL_REPORTER          , "Reporter",
                 REL_QA                , "QAcontact",
                 REL_CC                , "CC",
                 REL_VOTER             , "Voter");

# This code is really ugly. It was a commandline interface, then it was moved.
# This really needs to be cleaned at some point.

my %nomail;

# This is run when we load the package
if (open(NOMAIL, '<', bz_locations->{'datadir'} . "/nomail")) {
    while (<NOMAIL>) {
        $nomail{trim($_)} = 1;
    }
    close(NOMAIL);
}

sub FormatTriple {
    my ($a, $b, $c) = (@_);
    $^A = "";
    my $temp = formline << 'END', $a, $b, $c;
^>>>>>>>>>>>>>>>>>>|^<<<<<<<<<<<<<<<<<<<<<<<<<<<|^<<<<<<<<<<<<<<<<<<<<<<<<<<<~~
END
    ; # This semicolon appeases my emacs editor macros. :-)
    return $^A;
}
    
sub FormatDouble {
    my ($a, $b) = (@_);
    $a .= ":";
    $^A = "";
    my $temp = formline << 'END', $a, $b;
^>>>>>>>>>>>>>>>>>> ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<~~
END
    ; # This semicolon appeases my emacs editor macros. :-)
    return $^A;
}

# This is a bit of a hack, basically keeping the old system()
# cmd line interface. Should clean this up at some point.
#
# args: bug_id, and an optional hash ref which may have keys for:
# changer, owner, qa, reporter, cc
# Optional hash contains values of people which will be forced to those
# roles when the email is sent.
# All the names are email addresses, not userids
# values are scalars, except for cc, which is a list
# This hash usually comes from the "mailrecipients" var in a template call.
sub Send {
    my ($id, $forced) = (@_);
    return ProcessOneBug($id, $forced);
}

sub ProcessOneBug {
    my ($id, $forced) = (@_);

    my @headerlist;
    my %defmailhead;
    my %fielddescription;

    my $msg = "";

    my $dbh = Bugzilla->dbh;
    
    my $fields = $dbh->selectall_arrayref('SELECT name, description, mailhead 
                                           FROM fielddefs ORDER BY sortkey');

    foreach my $fielddef (@$fields) {
        my ($field, $description, $mailhead) = @$fielddef;
        push(@headerlist, $field);
        $defmailhead{$field} = $mailhead;
        $fielddescription{$field} = $description;
    }

    my %values = %{$dbh->selectrow_hashref(
        'SELECT ' . join(',', editable_bug_fields()) . ',
                lastdiffed AS start, LOCALTIMESTAMP(0) AS end
           FROM bugs WHERE bug_id = ?',
        undef, $id)};

    my $product = new Bugzilla::Product($values{product_id});
    $values{product} = $product->name;
    my $component = new Bugzilla::Component($values{component_id});
    $values{component} = $component->name;

    my ($start, $end) = ($values{start}, $values{end});

    # User IDs of people in various roles. More than one person can 'have' a 
    # role, if the person in that role has changed, or people are watching.
    my $reporter = $values{'reporter'};
    my @assignees = ($values{'assigned_to'});
    my @qa_contacts = ($values{'qa_contact'});

    my $cc_users = $dbh->selectall_arrayref(
           "SELECT cc.who, profiles.login_name
              FROM cc
        INNER JOIN profiles
                ON cc.who = profiles.userid
             WHERE bug_id = ?",
           undef, $id);

    my (@ccs, @cc_login_names);
    foreach my $cc_user (@$cc_users) {
        my ($user_id, $user_login) = @$cc_user;
        push (@ccs, $user_id);
        push (@cc_login_names, $user_login);
    }

    # Include the people passed in as being in particular roles.
    # This can include people who used to hold those roles.
    # At this point, we don't care if there are duplicates in these arrays.
    my $changer = $forced->{'changer'};
    if ($forced->{'owner'}) {
        push (@assignees, login_to_id($forced->{'owner'}, THROW_ERROR));
    }
    
    if ($forced->{'qacontact'}) {
        push (@qa_contacts, login_to_id($forced->{'qacontact'}, THROW_ERROR));
    }
    
    if ($forced->{'cc'}) {
        foreach my $cc (@{$forced->{'cc'}}) {
            push(@ccs, login_to_id($cc, THROW_ERROR));
        }
    }
    
    # Convert to names, for later display
    $values{'changer'} = $changer;
    $values{'changername'} = Bugzilla::User->new_from_login($changer)->name;
    $values{'assigned_to'} = user_id_to_login($values{'assigned_to'});
    $values{'reporter'} = user_id_to_login($values{'reporter'});
    if ($values{'qa_contact'}) {
        $values{'qa_contact'} = user_id_to_login($values{'qa_contact'});
    }
    $values{'cc'} = join(', ', @cc_login_names);
    $values{'estimated_time'} = format_time_decimal($values{'estimated_time'});

    if ($values{'deadline'}) {
        $values{'deadline'} = time2str("%Y-%m-%d", str2time($values{'deadline'}));
    }

    my $dependslist = $dbh->selectcol_arrayref(
        'SELECT dependson FROM dependencies
         WHERE blocked = ? ORDER BY dependson',
        undef, ($id));

    $values{'dependson'} = join(",", @$dependslist);

    my $blockedlist = $dbh->selectcol_arrayref(
        'SELECT blocked FROM dependencies
         WHERE dependson = ? ORDER BY blocked',
        undef, ($id));

    $values{'blocked'} = join(",", @$blockedlist);

    my @args = ($id);

    # If lastdiffed is NULL, then we don't limit the search on time.
    my $when_restriction = '';
    if ($start) {
        $when_restriction = ' AND bug_when > ? AND bug_when <= ?';
        push @args, ($start, $end);
    }
    
    my $diffs = $dbh->selectall_arrayref(
           "SELECT profiles.login_name, profiles.realname, fielddefs.description,
                   bugs_activity.bug_when, bugs_activity.removed, 
                   bugs_activity.added, bugs_activity.attach_id, fielddefs.name
              FROM bugs_activity
        INNER JOIN fielddefs
                ON fielddefs.fieldid = bugs_activity.fieldid
        INNER JOIN profiles
                ON profiles.userid = bugs_activity.who
             WHERE bugs_activity.bug_id = ?
                   $when_restriction
          ORDER BY bugs_activity.bug_when", undef, @args);

    my $difftext = "";
    my $diffheader = "";
    my @diffparts;
    my $lastwho = "";
    my @changedfields;
    foreach my $ref (@$diffs) {
        my ($who, $whoname, $what, $when, $old, $new, $attachid, $fieldname) = (@$ref);
        my $diffpart = {};
        if ($who ne $lastwho) {
            $lastwho = $who;
            $diffheader = "\n$whoname <$who" . Bugzilla->params->{'emailsuffix'}
                          . "> changed:\n\n";
            $diffheader .= FormatTriple("What    ", "Removed", "Added");
            $diffheader .= ('-' x 76) . "\n";
        }
        $what =~ s/^(Attachment )?/Attachment #$attachid / if $attachid;
        if( $fieldname eq 'estimated_time' ||
            $fieldname eq 'remaining_time' ) {
            $old = format_time_decimal($old);
            $new = format_time_decimal($new);
        }
        if ($attachid) {
            ($diffpart->{'isprivate'}) = $dbh->selectrow_array(
                'SELECT isprivate FROM attachments WHERE attach_id = ?',
                undef, ($attachid));
        }
        $difftext = FormatTriple($what, $old, $new);
        $diffpart->{'header'} = $diffheader;
        $diffpart->{'fieldname'} = $fieldname;
        $diffpart->{'text'} = $difftext;
        push(@diffparts, $diffpart);
        push(@changedfields, $what);
    }
    $values{'changed_fields'} = join(' ', @changedfields);

    my $deptext = "";

    my $dependency_diffs = $dbh->selectall_arrayref(
           "SELECT bugs_activity.bug_id, bugs.short_desc, fielddefs.name, 
                   bugs_activity.removed, bugs_activity.added
              FROM bugs_activity
        INNER JOIN bugs
                ON bugs.bug_id = bugs_activity.bug_id
        INNER JOIN dependencies
                ON bugs_activity.bug_id = dependencies.dependson
        INNER JOIN fielddefs
                ON fielddefs.fieldid = bugs_activity.fieldid
             WHERE dependencies.blocked = ?
               AND (fielddefs.name = 'bug_status'
                    OR fielddefs.name = 'resolution')
                   $when_restriction
          ORDER BY bugs_activity.bug_when, bugs.bug_id", undef, @args);

    my $thisdiff = "";
    my $lastbug = "";
    my $interestingchange = 0;
    my @depbugs;
    foreach my $dependency_diff (@$dependency_diffs) {
        my ($depbug, $summary, $what, $old, $new) = @$dependency_diff;

        if ($depbug ne $lastbug) {
            if ($interestingchange) {
                $deptext .= $thisdiff;
            }
            $lastbug = $depbug;
            my $urlbase = Bugzilla->params->{"urlbase"};
            $thisdiff =
              "\nBug $id depends on bug $depbug, which changed state.\n\n" . 
              "Bug $depbug Summary: $summary\n" . 
              "${urlbase}show_bug.cgi?id=$depbug\n\n"; 
            $thisdiff .= FormatTriple("What    ", "Old Value", "New Value");
            $thisdiff .= ('-' x 76) . "\n";
            $interestingchange = 0;
        }
        $thisdiff .= FormatTriple($fielddescription{$what}, $old, $new);
        if ($what eq 'bug_status'
            && Bugzilla::Bug::is_open_state($old) ne Bugzilla::Bug::is_open_state($new))
        {
            $interestingchange = 1;
        }
        
        push(@depbugs, $depbug);
    }
    
    if ($interestingchange) {
        $deptext .= $thisdiff;
    }

    $deptext = trim($deptext);

    if ($deptext) {
        my $diffpart = {};
        $diffpart->{'text'} = "\n" . trim("\n\n" . $deptext);
        push(@diffparts, $diffpart);
    }


    my ($newcomments, $anyprivate) = get_comments_by_bug($id, $start, $end);

    ###########################################################################
    # Start of email filtering code
    ###########################################################################
    
    # A user_id => roles hash to keep track of people.
    my %recipients;
    my %watching;
    
    # Now we work out all the people involved with this bug, and note all of
    # the relationships in a hash. The keys are userids, the values are an
    # array of role constants.
    
    # Voters
    my $voters = $dbh->selectcol_arrayref(
        "SELECT who FROM votes WHERE bug_id = ?", undef, ($id));
        
    $recipients{$_}->{+REL_VOTER} = BIT_DIRECT foreach (@$voters);

    # CCs
    $recipients{$_}->{+REL_CC} = BIT_DIRECT foreach (@ccs);
    
    # Reporter (there's only ever one)
    $recipients{$reporter}->{+REL_REPORTER} = BIT_DIRECT;
    
    # QA Contact
    if (Bugzilla->params->{'useqacontact'}) {
        foreach (@qa_contacts) {
            # QA Contact can be blank; ignore it if so.
            $recipients{$_}->{+REL_QA} = BIT_DIRECT if $_;
        }
    }

    # Assignee
    $recipients{$_}->{+REL_ASSIGNEE} = BIT_DIRECT foreach (@assignees);

    # The last relevant set of people are those who are being removed from 
    # their roles in this change. We get their names out of the diffs.
    foreach my $ref (@$diffs) {
        my ($who, $what, $when, $old, $new) = (@$ref);
        if ($old) {
            # You can't stop being the reporter, and mail isn't sent if you
            # remove your vote.
            # Ignore people whose user account has been deleted or renamed.
            if ($what eq "CC") {
                foreach my $cc_user (split(/[\s,]+/, $old)) {
                    my $uid = login_to_id($cc_user);
                    $recipients{$uid}->{+REL_CC} = BIT_DIRECT if $uid;
                }
            }
            elsif ($what eq "QAContact") {
                my $uid = login_to_id($old);
                $recipients{$uid}->{+REL_QA} = BIT_DIRECT if $uid;
            }
            elsif ($what eq "AssignedTo") {
                my $uid = login_to_id($old);
                $recipients{$uid}->{+REL_ASSIGNEE} = BIT_DIRECT if $uid;
            }
        }
    }
    
    if (Bugzilla->params->{"supportwatchers"}) {
        # Find all those user-watching anyone on the current list, who is not 
        # on it already themselves.
        my $involved = join(",", keys %recipients);

        my $userwatchers = 
            $dbh->selectall_arrayref("SELECT watcher, watched FROM watch 
                                      WHERE watched IN ($involved)");

        # Mark these people as having the role of the person they are watching
        foreach my $watch (@$userwatchers) {
            while (my ($role, $bits) = each %{$recipients{$watch->[1]}}) {
                $recipients{$watch->[0]}->{$role} |= BIT_WATCHING
                    if $bits & BIT_DIRECT;
            }
            push (@{$watching{$watch->[0]}}, $watch->[1]);
        }
    }
        
    # We now have a complete set of all the users, and their relationships to
    # the bug in question. However, we are not necessarily going to mail them
    # all - there are preferences, permissions checks and all sorts to do yet.
    my @sent;
    my @excluded;

    foreach my $user_id (keys %recipients) {
        my %rels_which_want;
        my $sent_mail = 0;

        my $user = new Bugzilla::User($user_id);
        # Deleted users must be excluded.
        next unless $user;

        if ($user->can_see_bug($id))
        {
            # Go through each role the user has and see if they want mail in
            # that role.
            foreach my $relationship (keys %{$recipients{$user_id}}) {
                if ($user->wants_bug_mail($id,
                                          $relationship, 
                                          $diffs, 
                                          $newcomments, 
                                          $changer))
                {
                    $rels_which_want{$relationship} = 
                        $recipients{$user_id}->{$relationship};
                }
            }
        }
        
        if (scalar(%rels_which_want)) {
            # So the user exists, can see the bug, and wants mail in at least
            # one role. But do we want to send it to them?

            # If we are using insiders, and the comment is private, only send 
            # to insiders
            my $insider_ok = 1;
            $insider_ok = 0 if (Bugzilla->params->{"insidergroup"} && 
                                ($anyprivate != 0) && 
                                (!$user->groups->{Bugzilla->params->{"insidergroup"}}));

            # We shouldn't send mail if this is a dependency mail (i.e. there 
            # is something in @depbugs), and any of the depending bugs are not 
            # visible to the user. This is to avoid leaking the summaries of 
            # confidential bugs.
            my $dep_ok = 1;
            foreach my $dep_id (@depbugs) {
                if (!$user->can_see_bug($dep_id)) {
                   $dep_ok = 0;
                   last;
                }
            }

            # Make sure the user isn't in the nomail list, and the insider and 
            # dep checks passed.
            if ((!$nomail{$user->login}) &&
                $insider_ok &&
                $dep_ok)
            {
                # OK, OK, if we must. Email the user.
                $sent_mail = sendMail($user, 
                                      \@headerlist,
                                      \%rels_which_want, 
                                      \%values,
                                      \%defmailhead, 
                                      \%fielddescription, 
                                      \@diffparts,
                                      $newcomments, 
                                      $anyprivate, 
                                      $start, 
                                      $id,
                                      exists $watching{$user_id} ?
                                             $watching{$user_id} : undef);
            }
        }
       
        if ($sent_mail) {
            push(@sent, $user->login); 
        } 
        else {
            push(@excluded, $user->login); 
        } 
    }
    
    $dbh->do('UPDATE bugs SET lastdiffed = ? WHERE bug_id = ?',
             undef, ($end, $id));

    return {'sent' => \@sent, 'excluded' => \@excluded};
}

sub sendMail {
    my ($user, $hlRef, $relRef, $valueRef, $dmhRef, $fdRef,  
        $diffRef, $newcomments, $anyprivate, $start, 
        $id, $watchingRef) = @_;

    my %values = %$valueRef;
    my @headerlist = @$hlRef;
    my %mailhead = %$dmhRef;
    my %fielddescription = %$fdRef;
    my @diffparts = @$diffRef;    
    my $head = "";
    
    foreach my $f (@headerlist) {
      if ($mailhead{$f}) {
        my $value = $values{$f};
        # If there isn't anything to show, don't include this header
        if (! $value) {
          next;
        }
        # Only send estimated_time if it is enabled and the user is in the group
        if (($f ne 'estimated_time' && $f ne 'deadline') ||
             $user->groups->{Bugzilla->params->{'timetrackinggroup'}}) {

            my $desc = $fielddescription{$f};
            $head .= FormatDouble($desc, $value);
        }
      }
    }

    # Build difftext (the actions) by verifying the user should see them
    my $difftext = "";
    my $diffheader = "";
    my $add_diff;

    foreach my $diff (@diffparts) {
        $add_diff = 0;
        
        if (exists($diff->{'fieldname'}) && 
            ($diff->{'fieldname'} eq 'estimated_time' ||
             $diff->{'fieldname'} eq 'remaining_time' ||
             $diff->{'fieldname'} eq 'work_time' ||
             $diff->{'fieldname'} eq 'deadline')){
            if ($user->groups->{Bugzilla->params->{"timetrackinggroup"}}) {
                $add_diff = 1;
            }
        } elsif (($diff->{'isprivate'}) 
                 && Bugzilla->params->{'insidergroup'}
                 && !($user->groups->{Bugzilla->params->{'insidergroup'}})
                ) {
            $add_diff = 0;
        } else {
            $add_diff = 1;
        }

        if ($add_diff) {
            if (exists($diff->{'header'}) && 
             ($diffheader ne $diff->{'header'})) {
                $diffheader = $diff->{'header'};
                $difftext .= $diffheader;
            }
            $difftext .= $diff->{'text'};
        }
    }
 
    if ($difftext eq "" && $newcomments eq "") {
      # Whoops, no differences!
      return 0;
    }
    
    # XXX: This needs making localisable, probably by passing the role to
    # the email template and letting it choose the text.
    my $reasonsbody = "------- You are receiving this mail because: -------\n";

    while (my ($relationship, $bits) = each %{$relRef}) {
        if ($relationship == REL_ASSIGNEE) {
            $reasonsbody .= "You are the assignee for the bug.\n"  if ($bits & BIT_DIRECT);
            $reasonsbody .= "You are watching the assignee for the bug.\n" if ($bits & BIT_WATCHING);
        } elsif ($relationship == REL_REPORTER) {
            $reasonsbody .= "You reported the bug.\n" if ($bits & BIT_DIRECT);
            $reasonsbody .= "You are watching the reporter.\n" if ($bits & BIT_WATCHING);
        } elsif ($relationship == REL_QA) {
            $reasonsbody .= "You are the QA contact for the bug.\n" if ($bits & BIT_DIRECT);
            $reasonsbody .= "You are watching the QA contact for the bug.\n" if ($bits & BIT_WATCHING);
        } elsif ($relationship == REL_CC) {
            $reasonsbody .= "You are on the CC list for the bug.\n" if ($bits & BIT_DIRECT);
            $reasonsbody .= "You are watching someone on the CC list for the bug.\n" if ($bits & BIT_WATCHING);
        } elsif ($relationship == REL_VOTER) {
            $reasonsbody .= "You are a voter for the bug.\n" if ($bits & BIT_DIRECT);
            $reasonsbody .= "You are watching a voter for the bug.\n" if ($bits & BIT_WATCHING);
        }
    }

    my $isnew = !$start;
    
    my %substs;

    # If an attachment was created, then add an URL. (Note: the 'g'lobal
    # replace should work with comments with multiple attachments.)

    if ( $newcomments =~ /Created an attachment \(/ ) {

        my $showattachurlbase =
            Bugzilla->params->{'urlbase'} . "attachment.cgi?id=";

        $newcomments =~ s/(Created an attachment \(id=([0-9]+)\))/$1\n --> \(${showattachurlbase}$2\)/g;
    }

    $substs{"neworchanged"} = $isnew ? 'New: ' : '';
    $substs{"to"} = $user->email;
    $substs{"cc"} = '';
    $substs{"bugid"} = $id;
    if ($isnew) {
      $substs{"diffs"} = $head . "\n\n" . $newcomments;
    } else {
      $substs{"diffs"} = $difftext . "\n\n" . $newcomments;
    }
    $substs{"product"} = $values{'product'};
    $substs{"component"} = $values{'component'};
    $substs{"keywords"} = $values{'keywords'};
    $substs{"severity"} = $values{'bug_severity'};
    $substs{"status"} = $values{'bug_status'};
    $substs{"priority"} = $values{'priority'};
    $substs{"assignedto"} = $values{'assigned_to'};
    $substs{"targetmilestone"} = $values{'target_milestone'};
    $substs{"changedfields"} = $values{'changed_fields'};
    $substs{"summary"} = $values{'short_desc'};
    my (@headerrel, @watchingrel);
    while (my ($rel, $bits) = each %{$relRef}) {
        push @headerrel, ($rel_names{$rel}) if ($bits & BIT_DIRECT);
        push @watchingrel, ($rel_names{$rel}) if ($bits & BIT_WATCHING);
    }
    push @headerrel, 'None' if !scalar(@headerrel);
    push @watchingrel, 'None' if !scalar(@watchingrel);
    push @watchingrel, map { user_id_to_login($_) } @$watchingRef;
    $substs{"reasonsheader"} = join(" ", @headerrel);
    $substs{"reasonswatchheader"} = join(" ", @watchingrel);

    $substs{"reasonsbody"} = $reasonsbody;
    $substs{"space"} = " ";
    $substs{"changer"} = $values{'changer'};
    $substs{"changername"} = $values{'changername'};

    my $sitespec = '@' . Bugzilla->params->{'urlbase'};
    $sitespec =~ s/:\/\//\./; # Make the protocol look like part of the domain
    $sitespec =~ s/^([^:\/]+):(\d+)/$1/; # Remove a port number, to relocate
    if ($2) {
        $sitespec = "-$2$sitespec"; # Put the port number back in, before the '@'
    }
    if ($isnew) {
        $substs{'threadingmarker'} = "Message-ID: <bug-$id-" . 
                                     $user->id . "$sitespec>";
    } else {
        $substs{'threadingmarker'} = "In-Reply-To: <bug-$id-" . 
                                     $user->id . "$sitespec>";
    }
    
    my $template = Bugzilla->params->{"newchangedmail"};
    
    my $msg = perform_substs($template, \%substs);

    MessageToMTA($msg);

    return 1;
}

# Send the login name and password of the newly created account to the user.
sub MailPassword {
    my ($login, $password) = (@_);
    my $template = Bugzilla->template;
    my $vars = {
      mailaddress => $login . Bugzilla->params->{'emailsuffix'},
      login => $login,
      password => $password };
    my $msg;
    $template->process("email/password.txt.tmpl", $vars, \$msg)
      || ThrowTemplateError($template->error());
    MessageToMTA($msg);
}

# Get bug comments for the given period and format them to be used in emails.
sub get_comments_by_bug {
    my ($id, $start, $end) = @_;
    my $dbh = Bugzilla->dbh;

    my $result = "";
    my $count = 0;
    my $anyprivate = 0;

    my $query = 'SELECT profiles.login_name, profiles.realname, ' .
                        $dbh->sql_date_format('longdescs.bug_when', '%Y.%m.%d %H:%i') . ',
                        longdescs.thetext, longdescs.isprivate,
                        longdescs.already_wrapped
                   FROM longdescs
             INNER JOIN profiles
                     ON profiles.userid = longdescs.who
                  WHERE longdescs.bug_id = ? ';

    my @args = ($id);

    # $start will be undef for new bugs, and defined for pre-existing bugs.
    if ($start) {
        # If $start is not NULL, obtain the count-index
        # of this comment for the leading "Comment #xxx" line.
        $count = $dbh->selectrow_array('SELECT COUNT(*) FROM longdescs
                                        WHERE bug_id = ? AND bug_when <= ?',
                                        undef, ($id, $start));

        $query .= ' AND longdescs.bug_when > ?
                    AND longdescs.bug_when <= ? ';
        push @args, ($start, $end);
    }

    $query .= ' ORDER BY longdescs.bug_when';
    my $comments = $dbh->selectall_arrayref($query, undef, @args);

    foreach (@$comments) {
        my ($who, $whoname, $when, $text, $isprivate, $already_wrapped) = @$_;
        if ($count) {
            $result .= "\n\n--- Comment #$count from $whoname <$who" .
                       Bugzilla->params->{'emailsuffix'}. ">  " 
                       . format_time($when) . " ---\n";
        }
        if ($isprivate > 0 && Bugzilla->params->{'insidergroup'}) {
            $anyprivate = 1;
        }
        $result .= ($already_wrapped ? $text : wrap_comment($text));
        $count++;
    }
    return ($result, $anyprivate);
}

1;
