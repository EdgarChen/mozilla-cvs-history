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
# Contributor(s):    Myk Melez <myk@mozilla.org>

################################################################################
# Module Initialization
################################################################################

# Make it harder for us to do dangerous things in Perl.
use strict;

# Bundle the functions in this file together into the "Bugzilla::Token" package.
package Bugzilla::Token;

use Bugzilla::Config;
use Bugzilla::Error;
use Bugzilla::Mailer;
use Bugzilla::Util;

use Date::Format;
use Date::Parse;

################################################################################
# Constants
################################################################################

# The maximum number of days a token will remain valid.
my $maxtokenage = 3;

################################################################################
# Public Functions
################################################################################

sub IssueEmailChangeToken {
    my ($userid, $old_email, $new_email) = @_;

    my ($token, $token_ts) = _create_token($userid, 'emailold', $old_email . ":" . $new_email);

    my $newtoken = _create_token($userid, 'emailnew', $old_email . ":" . $new_email);

    # Mail the user the token along with instructions for using it.

    my $template = Bugzilla->template;
    my $vars = {};

    $vars->{'oldemailaddress'} = $old_email . Param('emailsuffix');
    $vars->{'newemailaddress'} = $new_email . Param('emailsuffix');
    
    $vars->{'max_token_age'} = $maxtokenage;
    $vars->{'token_ts'} = $token_ts;

    $vars->{'token'} = $token;
    $vars->{'emailaddress'} = $old_email . Param('emailsuffix');

    my $message;
    $template->process("account/email/change-old.txt.tmpl", $vars, \$message)
      || ThrowTemplateError($template->error());

    MessageToMTA($message);

    $vars->{'token'} = $newtoken;
    $vars->{'emailaddress'} = $new_email . Param('emailsuffix');

    $message = "";
    $template->process("account/email/change-new.txt.tmpl", $vars, \$message)
      || ThrowTemplateError($template->error());

    MessageToMTA($message);
}

# Generates a random token, adds it to the tokens table, and sends it
# to the user with instructions for using it to change their password.
sub IssuePasswordToken {
    my $loginname = shift;
    my $dbh = Bugzilla->dbh;
    my $template = Bugzilla->template;
    my $vars = {};

    # Retrieve the user's ID from the database.
    trick_taint($loginname);
    my ($userid, $too_soon) =
        $dbh->selectrow_array('SELECT profiles.userid, tokens.issuedate
                                 FROM profiles
                            LEFT JOIN tokens
                                   ON tokens.userid = profiles.userid
                                  AND tokens.tokentype = ?
                                  AND tokens.issuedate > NOW() - ' .
                                      $dbh->sql_interval(10, 'MINUTE') . '
                                WHERE ' . $dbh->sql_istrcmp('login_name', '?'),
                                undef, ('password', $loginname));

    ThrowUserError('too_soon_for_new_token') if $too_soon;

    my ($token, $token_ts) = _create_token($userid, 'password', $::ENV{'REMOTE_ADDR'});

    # Mail the user the token along with instructions for using it.
    $vars->{'token'} = $token;
    $vars->{'emailaddress'} = $loginname . Param('emailsuffix');

    $vars->{'max_token_age'} = $maxtokenage;
    $vars->{'token_ts'} = $token_ts;

    my $message = "";
    $template->process("account/password/forgotten-password.txt.tmpl", 
                                                               $vars, \$message)
      || ThrowTemplateError($template->error());

    MessageToMTA($message);
}

sub IssueSessionToken {
    # Generates a random token, adds it to the tokens table, and returns
    # the token to the caller.

    my $data = shift;
    return _create_token(Bugzilla->user->id, 'session', $data);
}

sub CleanTokenTable {
    my $dbh = Bugzilla->dbh;
    $dbh->bz_lock_tables('tokens WRITE');
    $dbh->do('DELETE FROM tokens
              WHERE ' . $dbh->sql_to_days('NOW()') . ' - ' .
                        $dbh->sql_to_days('issuedate') . ' >= ?',
              undef, $maxtokenage);
    $dbh->bz_unlock_tables();
}

sub GenerateUniqueToken {
    # Generates a unique random token.  Uses generate_random_password 
    # for the tokens themselves and checks uniqueness by searching for
    # the token in the "tokens" table.  Gives up if it can't come up
    # with a token after about one hundred tries.
    my ($table, $column) = @_;

    my $token;
    my $duplicate = 1;
    my $tries = 0;
    $table ||= "tokens";
    $column ||= "token";

    my $dbh = Bugzilla->dbh;
    my $sth = $dbh->prepare("SELECT userid FROM $table WHERE $column = ?");

    while ($duplicate) {
        ++$tries;
        if ($tries > 100) {
            ThrowCodeError("token_generation_error");
        }
        $token = generate_random_password();
        $sth->execute($token);
        $duplicate = $sth->fetchrow_array;
    }
    return $token;
}

# Cancels a previously issued token and notifies the system administrator.
# This should only happen when the user accidentally makes a token request
# or when a malicious hacker makes a token request on behalf of a user.
sub Cancel {
    my ($token, $cancelaction, $vars) = @_;
    my $dbh = Bugzilla->dbh;
    $vars ||= {};

    # Get information about the token being cancelled.
    trick_taint($token);
    my ($issuedate, $tokentype, $eventdata, $loginname, $realname) =
        $dbh->selectrow_array('SELECT ' . $dbh->sql_date_format('issuedate') . ',
                                      tokentype, eventdata, login_name, realname
                                 FROM tokens
                           INNER JOIN profiles
                                   ON tokens.userid = profiles.userid
                                WHERE token = ?',
                                undef, $token);

    # Get the email address of the Bugzilla maintainer.
    my $maintainer = Param('maintainer');

    my $template = Bugzilla->template;

    $vars->{'emailaddress'} = $loginname . Param('emailsuffix');
    $vars->{'maintainer'} = $maintainer;
    $vars->{'remoteaddress'} = $::ENV{'REMOTE_ADDR'};
    $vars->{'token'} = $token;
    $vars->{'tokentype'} = $tokentype;
    $vars->{'issuedate'} = $issuedate;
    $vars->{'eventdata'} = $eventdata;
    $vars->{'cancelaction'} = $cancelaction;

    # Notify the user via email about the cancellation.

    my $message;
    $template->process("account/cancel-token.txt.tmpl", $vars, \$message)
      || ThrowTemplateError($template->error());

    MessageToMTA($message);

    # Delete the token from the database.
    DeleteToken($token);
}

sub DeletePasswordTokens {
    my ($userid, $reason) = @_;
    my $dbh = Bugzilla->dbh;

    detaint_natural($userid);
    my $tokens = $dbh->selectcol_arrayref('SELECT token FROM tokens
                                           WHERE userid = ? AND tokentype = ?',
                                           undef, ($userid, 'password'));

    foreach my $token (@$tokens) {
        Bugzilla::Token::Cancel($token, $reason);
    }
}

# Returns an email change token if the user has one. 
sub HasEmailChangeToken {
    my $userid = shift;
    my $dbh = Bugzilla->dbh;

    my $token = $dbh->selectrow_array('SELECT token FROM tokens
                                       WHERE userid = ?
                                       AND (tokentype = ? OR tokentype = ?) ' .
                                       $dbh->sql_limit(1),
                                       undef, ($userid, 'emailnew', 'emailold'));
    return $token;
}

# Returns the userid, issuedate and eventdata for the specified token
sub GetTokenData {
    my ($token) = @_;
    my $dbh = Bugzilla->dbh;

    return unless defined $token;
    trick_taint($token);

    return $dbh->selectrow_array(
        "SELECT userid, " . $dbh->sql_date_format('issuedate') . ", eventdata 
         FROM   tokens 
         WHERE  token = ?", undef, $token);
}

# Deletes specified token
sub DeleteToken {
    my ($token) = @_;
    my $dbh = Bugzilla->dbh;

    return unless defined $token;
    trick_taint($token);

    $dbh->bz_lock_tables('tokens WRITE');
    $dbh->do("DELETE FROM tokens WHERE token = ?", undef, $token);
    $dbh->bz_unlock_tables();
}

################################################################################
# Internal Functions
################################################################################

# Generates a unique token and inserts it into the database
# Returns the token and the token timestamp
sub _create_token {
    my ($userid, $tokentype, $eventdata) = @_;
    my $dbh = Bugzilla->dbh;

    detaint_natural($userid);
    trick_taint($tokentype);
    trick_taint($eventdata);

    $dbh->bz_lock_tables('tokens WRITE');

    my $token = GenerateUniqueToken();

    $dbh->do("INSERT INTO tokens (userid, issuedate, token, tokentype, eventdata)
        VALUES (?, NOW(), ?, ?, ?)", undef, ($userid, $token, $tokentype, $eventdata));

    $dbh->bz_unlock_tables();

    if (wantarray) {
        my (undef, $token_ts, undef) = GetTokenData($token);
        $token_ts = str2time($token_ts);
        return ($token, $token_ts);
    } else {
        return $token;
    }
}

1;
