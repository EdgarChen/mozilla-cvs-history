#!/usr/bonsaitools/bin/perl -wT
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
# Contributor(s): Myk Melez <myk@mozilla.org>

############################################################################
# Script Initialization
############################################################################

# Make it harder for us to do dangerous things in Perl.
use diagnostics;
use strict;

use lib qw(.);

use vars qw($template $vars);

# Include the Bugzilla CGI and general utility library.
require "CGI.pl";

# Establish a connection to the database backend.
ConnectToDatabase();

# Use the "Token" module that contains functions for doing various
# token-related tasks.
use Token;

################################################################################
# Data Validation / Security Authorization
################################################################################

# Throw an error if the form does not contain an "action" field specifying
# what the user wants to do.
$::FORM{'a'}
  || DisplayError("I could not figure out what you wanted to do.")
  && exit;

# Assign the action to a global variable.
$::action = $::FORM{'a'};

# If a token was submitted, make sure it is a valid token that exists in the
# database and is the correct type for the action being taken.
if ($::FORM{'t'}) {
  # Assign the token and its SQL quoted equivalent to global variables.
  $::token = $::FORM{'t'};
  $::quotedtoken = SqlQuote($::token);
  
  # Make sure the token contains only valid characters in the right amount.
  my $validationerror = ValidatePassword($::token);
  if ($validationerror) {
      DisplayError('The token you entered is invalid.');
      exit;
  }

  # Make sure the token exists in the database.
  SendSQL( "SELECT tokentype FROM tokens WHERE token = $::quotedtoken" );
  (my $tokentype = FetchSQLData())
    || DisplayError("The token you submitted does not exist.")
    && exit;

  # Make sure the token is the correct type for the action being taken.
  if ( grep($::action eq $_ , qw(cfmpw cxlpw chgpw)) && $tokentype ne 'password' ) {
    DisplayError("That token cannot be used to change your password.");
    Token::Cancel($::token, "user tried to use token to change password");
    exit;
  }
}

# If the user is requesting a password change, make sure they submitted
# their login name and it exists in the database.
if ( $::action eq 'reqpw' ) {
    defined $::FORM{'loginname'}
      || DisplayError("You must enter a login name when requesting to change your password.")
      && exit;

    # Make sure the login name looks like an email address.  This function
    # displays its own error and stops execution if the login name looks wrong.
    CheckEmailSyntax($::FORM{'loginname'});

    my $quotedloginname = SqlQuote($::FORM{'loginname'});
    SendSQL("SELECT userid FROM profiles WHERE login_name = $quotedloginname");
    FetchSQLData()
      || DisplayError("There is no Bugzilla account with that login name.")
      && exit;
}

# If the user is changing their password, make sure they submitted a new
# password and that the new password is valid.
if ( $::action eq 'chgpw' ) {
    defined $::FORM{'password'}
      && defined $::FORM{'matchpassword'}
      || DisplayError("You cannot change your password without submitting a new one.")
      && exit;

     my $passworderror = ValidatePassword($::FORM{'password'}, $::FORM{'matchpassword'});
     if ( $passworderror ) {
         DisplayError($passworderror);
         exit;
     }
}

################################################################################
# Main Body Execution
################################################################################

# All calls to this script should contain an "action" variable whose value
# determines what the user wants to do.  The code below checks the value of
# that variable and runs the appropriate code.

if ($::action eq 'reqpw') { 
    requestChangePassword(); 
} elsif ($::action eq 'cfmpw') { 
    confirmChangePassword(); 
} elsif ($::action eq 'cxlpw') { 
    cancelChangePassword(); 
} elsif ($::action eq 'chgpw') { 
    changePassword(); 
} else { 
    # If the action that the user wants to take (specified in the "a" form field)
    # is none of the above listed actions, display an error telling the user 
    # that we do not understand what they would like to do.
    DisplayError("I could not figure out what you wanted to do.");
}

exit;

################################################################################
# Functions
################################################################################

sub requestChangePassword {
    Token::IssuePasswordToken($::FORM{'loginname'});

    $vars->{'title'} = "Request to Change Password";
    $vars->{'message'} = "A token for changing your password has been 
                          emailed to you. Follow the instructions in 
                          that email to change your password.";

    print "Content-Type: text/html\n\n";
    $template->process("global/message.html.tmpl", $vars)
      || DisplayError("Template process failed: " . $template->error())
      && exit;
}

sub confirmChangePassword {
    $vars->{'title'} = "Change Password";
    $vars->{'token'} = $::token;
    
    print "Content-Type: text/html\n\n";
    $template->process("admin/change-password.html.tmpl", $vars)
      || DisplayError("Template process failed: " . $template->error())
      && exit;
}

sub cancelChangePassword {    
    Token::Cancel($::token, "user requested cancellation");

    $vars->{'title'} = "Cancel Request to Change Password";
    $vars->{'message'} = "Your request has been cancelled.";

    print "Content-Type: text/html\n\n";
    $template->process("global/message.html.tmpl", $vars)
      || DisplayError("Template process failed: " . $template->error())
      && exit;
}

sub changePassword {
    # Quote the password and token for inclusion into SQL statements.
    my $cryptedpassword = Crypt($::FORM{'password'});
    my $quotedpassword = SqlQuote($cryptedpassword);

    # Get the user's ID from the tokens table.
    SendSQL("SELECT userid FROM tokens WHERE token = $::quotedtoken");
    my $userid = FetchSQLData();
    
    # Update the user's password in the profiles table and delete the token
    # from the tokens table.
    SendSQL("LOCK TABLES profiles WRITE , tokens WRITE");
    SendSQL("UPDATE   profiles
             SET      cryptpassword = $quotedpassword
             WHERE    userid = $userid");
    SendSQL("DELETE FROM tokens WHERE token = $::quotedtoken");
    SendSQL("UNLOCK TABLES");

    InvalidateLogins($userid);

    $vars->{'title'} = "Password Changed";
    $vars->{'message'} = "Your password has been changed.";

    print "Content-Type: text/html\n\n";
    $template->process("global/message.html.tmpl", $vars)
      || DisplayError("Template process failed: " . $template->error())
      && exit;
}




