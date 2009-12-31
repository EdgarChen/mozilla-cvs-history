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
# Contributor(s): Joel Peshkin <bugreport@peshkin.net>
#                 Erik Stambaugh <erik@dasbistro.com>
#                 Tiago R. Mello <timello@async.com.br>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

use strict;

package Bugzilla::Group;

use base qw(Bugzilla::Object);

use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Config qw(:admin);

###############################
##### Module Initialization ###
###############################

use constant DB_COLUMNS => qw(
    groups.id
    groups.name
    groups.description
    groups.isbuggroup
    groups.userregexp
    groups.isactive
    groups.icon_url
);

use constant DB_TABLE => 'groups';

use constant LIST_ORDER => 'isbuggroup, name';

use constant VALIDATORS => {
    name        => \&_check_name,
    description => \&_check_description,
    userregexp  => \&_check_user_regexp,
    isactive    => \&_check_is_active,
    isbuggroup  => \&_check_is_bug_group,
    icon_url    => \&_check_icon_url,
};

use constant REQUIRED_CREATE_FIELDS => qw(name description isbuggroup);

use constant UPDATE_COLUMNS => qw(
    name
    description
    userregexp
    isactive
    icon_url
);

# Parameters that are lists of groups.
use constant GROUP_PARAMS => qw(chartgroup insidergroup timetrackinggroup
                                querysharegroup);

###############################
####      Accessors      ######
###############################

sub description  { return $_[0]->{'description'};  }
sub is_bug_group { return $_[0]->{'isbuggroup'};   }
sub user_regexp  { return $_[0]->{'userregexp'};   }
sub is_active    { return $_[0]->{'isactive'};     }
sub icon_url     { return $_[0]->{'icon_url'};     }

sub bugs {
    my $self = shift;
    return $self->{bugs} if exists $self->{bugs};
    my $bug_ids = Bugzilla->dbh->selectcol_arrayref(
        'SELECT bug_id FROM bug_group_map WHERE group_id = ?',
        undef, $self->id);
    require Bugzilla::Bug;
    $self->{bugs} = Bugzilla::Bug->new_from_list($bug_ids);
    return $self->{bugs};
}

sub members_direct {
    my ($self) = @_;
    $self->{members_direct} ||= $self->_get_members(GRANT_DIRECT);
    return $self->{members_direct};
}

sub members_non_inherited {
    my ($self) = @_;
    $self->{members_non_inherited} ||= $self->_get_members();
    return $self->{members_non_inherited};
}

# A helper for members_direct and members_non_inherited
sub _get_members {
    my ($self, $grant_type) = @_;
    my $dbh = Bugzilla->dbh;
    my $grant_clause = $grant_type ? "AND grant_type = $grant_type" : "";
    my $user_ids = $dbh->selectcol_arrayref(
        "SELECT DISTINCT user_id
           FROM user_group_map
          WHERE isbless = 0 $grant_clause AND group_id = ?", undef, $self->id);
    require Bugzilla::User;
    return Bugzilla::User->new_from_list($user_ids);
}

sub flag_types {
    my $self = shift;
    require Bugzilla::FlagType;
    $self->{flag_types} ||= Bugzilla::FlagType::match({ group => $self->id });
    return $self->{flag_types};
}

sub grant_direct {
    my ($self, $type) = @_;
    $self->{grant_direct} ||= {};
    return $self->{grant_direct}->{$type} 
        if defined $self->{grant_direct}->{$type};
    my $dbh = Bugzilla->dbh;

    my $ids = $dbh->selectcol_arrayref(
      "SELECT member_id FROM group_group_map
        WHERE grantor_id = ? AND grant_type = $type", 
      undef, $self->id) || [];

    $self->{grant_direct}->{$type} = $self->new_from_list($ids);
    return $self->{grant_direct}->{$type};
}

sub granted_by_direct {
    my ($self, $type) = @_;
    $self->{granted_by_direct} ||= {};
    return $self->{granted_by_direct}->{$type}
         if defined $self->{granted_by_direct}->{$type};
    my $dbh = Bugzilla->dbh;

    my $ids = $dbh->selectcol_arrayref(
      "SELECT grantor_id FROM group_group_map
        WHERE member_id = ? AND grant_type = $type",
      undef, $self->id) || [];

    $self->{granted_by_direct}->{$type} = $self->new_from_list($ids);
    return $self->{granted_by_direct}->{$type};
}

sub products {
    my $self = shift;
    return $self->{products} if exists $self->{products};
    my $product_data = Bugzilla->dbh->selectall_arrayref(
        'SELECT product_id, entry, membercontrol, othercontrol,
                canedit, editcomponents, editbugs, canconfirm
          FROM  group_control_map WHERE group_id = ?', {Slice=>{}},
        $self->id);
    my @ids = map { $_->{product_id} } @$product_data;
    require Bugzilla::Product;
    my $products = Bugzilla::Product->new_from_list(\@ids); 
    my %data_map = map { $_->{product_id} => $_ } @$product_data;
    my @retval;
    foreach my $product (@$products) {
        # Data doesn't need to contain product_id--we already have
        # the product object.
        delete $data_map{$product->id}->{product_id};
        push(@retval, { controls => $data_map{$product->id},
                        product  => $product });
    }
    $self->{products} = \@retval;
    return $self->{products};
}

###############################
####        Methods        ####
###############################

sub set_description { $_[0]->set('description', $_[1]); }
sub set_is_active   { $_[0]->set('isactive', $_[1]);    }
sub set_name        { $_[0]->set('name', $_[1]);        }
sub set_user_regexp { $_[0]->set('userregexp', $_[1]);  }
sub set_icon_url    { $_[0]->set('icon_url', $_[1]);    }

sub update {
    my $self = shift;
    my $changes = $self->SUPER::update(@_);

    if (exists $changes->{name}) {
        my ($old_name, $new_name) = @{$changes->{name}};
        my $update_params;
        foreach my $group (GROUP_PARAMS) {
            if ($old_name eq Bugzilla->params->{$group}) {
                SetParam($group, $new_name);
                $update_params = 1;
            }
        }
        write_params() if $update_params;
    }

    # If we've changed this group to be active, fix any Mandatory groups.
    $self->_enforce_mandatory if (exists $changes->{isactive} 
                                  && $changes->{isactive}->[1]);

    $self->_rederive_regexp() if exists $changes->{userregexp};
    return $changes;
}

sub check_remove {
    my ($self, $params) = @_;

    # System groups cannot be deleted!
    if (!$self->is_bug_group) {
        ThrowUserError("system_group_not_deletable", { name => $self->name });
    }

    # Groups having a special role cannot be deleted.
    my @special_groups;
    foreach my $special_group (GROUP_PARAMS) {
        if ($self->name eq Bugzilla->params->{$special_group}) {
            push(@special_groups, $special_group);
        }
    }
    if (scalar(@special_groups)) {
        ThrowUserError('group_has_special_role',
                       { name   => $self->name,
                         groups => \@special_groups });
    }

    return if $params->{'test_only'};

    my $cantdelete = 0;

    my $users = $self->members_non_inherited;
    if (scalar(@$users) && !$params->{'remove_from_users'}) {
        $cantdelete = 1;
    }

    my $bugs = $self->bugs;
    if (scalar(@$bugs) && !$params->{'remove_from_bugs'}) {
        $cantdelete = 1;
    }
    
    my $products = $self->products;
    if (scalar(@$products) && !$params->{'remove_from_products'}) {
        $cantdelete = 1;
    }

    my $flag_types = $self->flag_types;
    if (scalar(@$flag_types) && !$params->{'remove_from_flags'}) {
        $cantdelete = 1;
    }

    ThrowUserError('group_cannot_delete', { group => $self }) if $cantdelete;
}

sub remove_from_db {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    $self->check_remove(@_);
    $dbh->do('DELETE FROM whine_schedules
               WHERE mailto_type = ? AND mailto = ?',
              undef, MAILTO_GROUP, $self->id);
    # All the other tables will be handled by foreign keys when we
    # drop the main "groups" row.
    $self->SUPER::remove_from_db(@_);
}

# Add missing entries in bug_group_map for bugs created while
# a mandatory group was disabled and which is now enabled again.
sub _enforce_mandatory {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    my $gid = $self->id;

    my $bug_ids =
      $dbh->selectcol_arrayref('SELECT bugs.bug_id
                                  FROM bugs
                            INNER JOIN group_control_map
                                    ON group_control_map.product_id = bugs.product_id
                             LEFT JOIN bug_group_map
                                    ON bug_group_map.bug_id = bugs.bug_id
                                   AND bug_group_map.group_id = group_control_map.group_id
                                 WHERE group_control_map.group_id = ?
                                   AND group_control_map.membercontrol = ?
                                   AND bug_group_map.group_id IS NULL',
                                 undef, ($gid, CONTROLMAPMANDATORY));

    my $sth = $dbh->prepare('INSERT INTO bug_group_map (bug_id, group_id) VALUES (?, ?)');
    foreach my $bug_id (@$bug_ids) {
        $sth->execute($bug_id, $gid);
    }
}

sub is_active_bug_group {
    my $self = shift;
    return $self->is_active && $self->is_bug_group;
}

sub _rederive_regexp {
    my ($self) = @_;

    my $dbh = Bugzilla->dbh;
    my $sth = $dbh->prepare("SELECT userid, login_name, group_id
                               FROM profiles
                          LEFT JOIN user_group_map
                                 ON user_group_map.user_id = profiles.userid
                                    AND group_id = ?
                                    AND grant_type = ?
                                    AND isbless = 0");
    my $sthadd = $dbh->prepare("INSERT INTO user_group_map
                                 (user_id, group_id, grant_type, isbless)
                                 VALUES (?, ?, ?, 0)");
    my $sthdel = $dbh->prepare("DELETE FROM user_group_map
                                 WHERE user_id = ? AND group_id = ?
                                 AND grant_type = ? and isbless = 0");
    $sth->execute($self->id, GRANT_REGEXP);
    my $regexp = $self->user_regexp;
    while (my ($uid, $login, $present) = $sth->fetchrow_array) {
        if ($regexp ne '' and $login =~ /$regexp/i) {
            $sthadd->execute($uid, $self->id, GRANT_REGEXP) unless $present;
        } else {
            $sthdel->execute($uid, $self->id, GRANT_REGEXP) if $present;
        }
    }
}

sub flatten_group_membership {
    my ($self, @groups) = @_;

    my $dbh = Bugzilla->dbh;
    my $sth;
    my @groupidstocheck = @groups;
    my %groupidschecked = ();
    $sth = $dbh->prepare("SELECT member_id FROM group_group_map
                             WHERE grantor_id = ? 
                               AND grant_type = " . GROUP_MEMBERSHIP);
    while (my $node = shift @groupidstocheck) {
        $sth->execute($node);
        my $member;
        while (($member) = $sth->fetchrow_array) {
            if (!$groupidschecked{$member}) {
                $groupidschecked{$member} = 1;
                push @groupidstocheck, $member;
                push @groups, $member unless grep $_ == $member, @groups;
            }
        }
    }
    return \@groups;
}




################################
#####  Module Subroutines    ###
################################

sub create {
    my $class = shift;
    my ($params) = @_;
    my $dbh = Bugzilla->dbh;

    print get_text('install_group_create', { name => $params->{name} }) . "\n" 
        if Bugzilla->usage_mode == USAGE_MODE_CMDLINE;

    my $group = $class->SUPER::create(@_);

    # Since we created a new group, give the "admin" group all privileges
    # initially.
    my $admin = new Bugzilla::Group({name => 'admin'});
    # This function is also used to create the "admin" group itself,
    # so there's a chance it won't exist yet.
    if ($admin) {
        my $sth = $dbh->prepare('INSERT INTO group_group_map
                                 (member_id, grantor_id, grant_type)
                                 VALUES (?, ?, ?)');
        $sth->execute($admin->id, $group->id, GROUP_MEMBERSHIP);
        $sth->execute($admin->id, $group->id, GROUP_BLESS);
        $sth->execute($admin->id, $group->id, GROUP_VISIBLE);
    }

    $group->_rederive_regexp() if $group->user_regexp;
    return $group;
}

sub ValidateGroupName {
    my ($name, @users) = (@_);
    my $dbh = Bugzilla->dbh;
    my $query = "SELECT id FROM groups " .
                "WHERE name = ?";
    if (Bugzilla->params->{'usevisibilitygroups'}) {
        my @visible = (-1);
        foreach my $user (@users) {
            $user && push @visible, @{$user->visible_groups_direct};
        }
        my $visible = join(', ', @visible);
        $query .= " AND id IN($visible)";
    }
    my $sth = $dbh->prepare($query);
    $sth->execute($name);
    my ($ret) = $sth->fetchrow_array();
    return $ret;
}

###############################
###       Validators        ###
###############################

sub _check_name {
    my ($invocant, $name) = @_;
    $name = trim($name);
    $name || ThrowUserError("empty_group_name");
    # If we're creating a Group or changing the name...
    if (!ref($invocant) || $invocant->name ne $name) {
        my $exists = new Bugzilla::Group({name => $name });
        ThrowUserError("group_exists", { name => $name }) if $exists;
    }
    return $name;
}

sub _check_description {
    my ($invocant, $desc) = @_;
    $desc = trim($desc);
    $desc || ThrowUserError("empty_group_description");
    return $desc;
}

sub _check_user_regexp {
    my ($invocant, $regex) = @_;
    $regex = trim($regex) || '';
    ThrowUserError("invalid_regexp") unless (eval {qr/$regex/});
    return $regex;
}

sub _check_is_active { return $_[1] ? 1 : 0; }
sub _check_is_bug_group {
    return $_[1] ? 1 : 0;
}

sub _check_icon_url { return $_[1] ? clean_text($_[1]) : undef; }

1;

__END__

=head1 NAME

Bugzilla::Group - Bugzilla group class.

=head1 SYNOPSIS

    use Bugzilla::Group;

    my $group = new Bugzilla::Group(1);
    my $group = new Bugzilla::Group({name => 'AcmeGroup'});

    my $id           = $group->id;
    my $name         = $group->name;
    my $description  = $group->description;
    my $user_reg_exp = $group->user_reg_exp;
    my $is_active    = $group->is_active;
    my $icon_url     = $group->icon_url;
    my $is_active_bug_group = $group->is_active_bug_group;

    my $group_id = Bugzilla::Group::ValidateGroupName('admin', @users);
    my @groups   = Bugzilla::Group->get_all;

=head1 DESCRIPTION

Group.pm represents a Bugzilla Group object. It is an implementation
of L<Bugzilla::Object>, and thus has all the methods that L<Bugzilla::Object>
provides, in addition to any methods documented below.

=head1 SUBROUTINES

=over

=item C<create>

Note that in addition to what L<Bugzilla::Object/create($params)>
normally does, this function also makes the new group be inherited
by the C<admin> group. That is, the C<admin> group will automatically
be a member of this group.

=item C<ValidateGroupName($name, @users)>

 Description: ValidateGroupName checks to see if ANY of the users
              in the provided list of user objects can see the
              named group.

 Params:      $name - String with the group name.
              @users - An array with Bugzilla::User objects.

 Returns:     It returns the group id if successful
              and undef otherwise.

=back

=head1 METHODS

=over

=item C<check_remove>

=over

=item B<Description>

Determines whether it's OK to remove this group from the database, and
throws an error if it's not OK.

=item B<Params>

=over

=item C<test_only>

C<boolean> If you want to only check if the group can be deleted I<at all>,
under any circumstances, specify C<test_only> to just do the most basic tests
(the other parameters will be ignored in this situation, as those tests won't
be run).

=item C<remove_from_users>

C<boolean> True if it would be OK to remove all users who are in this group
from this group.

=item C<remove_from_bugs>

C<boolean> True if it would be OK to remove all bugs that are in this group
from this group.

=item C<remove_from_flags>

C<boolean> True if it would be OK to stop all flagtypes that reference
this group from referencing this group (e.g., as their grantgroup or
requestgroup).

=item C<remove_from_products>

C<boolean> True if it would be OK to remove this group from all group controls
on products.

=back

=item B<Returns> (nothing)

=back

=item C<members_non_inherited>

Returns an arrayref of L<Bugzilla::User> objects representing people who are
"directly" in this group, meaning that they're in it because they match
the group regular expression, or they have been actually added to the
group manually.

=item C<flatten_group_membership>

Accepts a list of groups and returns a list of all the groups whose members 
inherit membership in any group on the list.  So, we can determine if a user
is in any of the groups input to flatten_group_membership by querying the
user_group_map for any user with DIRECT or REGEXP membership IN() the list
of groups returned.

=back
