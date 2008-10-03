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
# The Initial Developer of the Original Code is NASA.
# Portions created by NASA are Copyright (C) 2006 San Jose State
# University Foundation. All Rights Reserved.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# Contributor(s): Max Kanat-Alexander <mkanat@bugzilla.org>

use strict;

package Bugzilla::Field::Choice;

use base qw(Bugzilla::Object);

use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Field;
use Bugzilla::Util qw(trim detaint_natural);

use Scalar::Util qw(blessed);

##################
# Initialization #
##################

use constant DB_COLUMNS => qw(
    id
    value
    sortkey
);

use constant NAME_FIELD => 'value';
use constant LIST_ORDER => 'sortkey, value';

use constant REQUIRED_CREATE_FIELDS => qw(value);

use constant VALIDATORS => {
    value   => \&_check_value,
    sortkey => \&_check_sortkey,
};

use constant CLASS_MAP => {
    bug_status => 'Bugzilla::Status',
};

#################
# Class Factory #
#################

# Bugzilla::Field::Choice is actually an abstract base class. Every field
# type has its own dynamically-generated class for its values. This allows
# certain fields to have special types, like how bug_status's values
# are Bugzilla::Status objects.

sub type {
    my ($class, $field) = @_;
    my $field_obj = blessed $field ? $field : Bugzilla::Field->check($field);
    my $field_name = $field_obj->name;

    if ($class->CLASS_MAP->{$field_name}) {
        return $class->CLASS_MAP->{$field_name};
    }

    # For generic classes, we use a lowercase class name, so as
    # not to interfere with any real subclasses we might make some day.
    my $package = "Bugzilla::Field::Choice::$field_name";

    # We check defined so that the package and the stored field are only
    # created once globally (at least per request). We prefix it with
    # field_ (instead of suffixing it) so that we don't somehow conflict
    # with the names of custom fields.
    if (!defined Bugzilla->request_cache->{"field_$package"}) {
        eval <<EOC;
            package $package;
            use base qw(Bugzilla::Field::Choice);
            use constant DB_TABLE => '$field_name';
EOC
        Bugzilla->request_cache->{"field_$package"} = $field_obj;
    }

    return $package;
}

################
# Constructors #
################

# We just make new() enforce this, which should give developers 
# the understanding that you can't use Bugzilla::Field::Choice
# without calling type().
sub new {
    my $class = shift;
    if ($class eq 'Bugzilla::Field::Choice') {
        ThrowCodeError('field_choice_must_use_type');
    }
    $class->SUPER::new(@_);
}

#########################
# Database Manipulation #
#########################

# Our subclasses can take more arguments than we normally accept.
# So, we override create() to remove arguments that aren't valid
# columns. (Normally Bugzilla::Object dies if you pass arguments
# that aren't valid columns.)
sub create {
    my $class = shift;
    my ($params) = @_;
    foreach my $key (keys %$params) {
        if (!grep {$_ eq $key} $class->DB_COLUMNS) {
            delete $params->{$key};
        }
    }
    return $class->SUPER::create(@_);
}

#############
# Accessors #
#############

sub sortkey { return $_[0]->{'sortkey'}; }
sub field {
    my $invocant = shift;
    my $class = ref $invocant || $invocant;
    my $cache = Bugzilla->request_cache;
    # This is just to make life easier for subclasses. Our auto-generated
    # subclasses from type() already have this set.
    $cache->{"field_$class"} ||=  
        new Bugzilla::Field({ name => $class->DB_TABLE });
    return $cache->{"field_$class"};
}

##############
# Validators #
##############

sub _check_value {
    my ($invocant, $value) = @_;

    my $field = $invocant->field;

    $value = trim($value);
    ThrowUserError('fieldvalue_undefined') if !defined $value || $value eq "";
    ThrowUserError('fieldvalue_name_too_long', { value => $value })
        if length($value) > MAX_FIELD_VALUE_SIZE;

    my $exists = $invocant->type($field)->new({ name => $value });
    if ($exists) {
        ThrowUserError('fieldvalue_already_exists', 
                       { field => $field, value => $value });
    }

    return $value;
}

sub _check_sortkey {
    my ($invocant, $value) = @_;
    $value = trim($value);
    return 0 if !$value;
    # Store for the error message in case detaint_natural clears it.
    my $orig_value = $value;
    detaint_natural($value)
        || ThrowUserError('fieldvalue_sortkey_invalid',
                          { sortkey => $orig_value,
                            field   => $invocant->field });
    return $value;
}


1;

__END__

=head1 NAME

Bugzilla::Field::Choice - A legal value for a <select>-type field.

=head1 SYNOPSIS

 my $field = new Bugzilla::Field({name => 'bug_status'});

 my $choice = new Bugzilla::Field::Choice->type($field)->new(1);

 my $choices = Bugzilla::Field::Choice->type($field)->new_from_list([1,2,3]);
 my $choices = Bugzilla::Field::Choice->type($field)->get_all();
 my $choices = Bugzilla::Field::Choice->type($field->match({ sortkey => 10 }); 

=head1 DESCRIPTION

This is an implementation of L<Bugzilla::Object>, but with a twist.
You can't call any class methods (such as C<new>, C<create>, etc.) 
directly on C<Bugzilla::Field::Choice> itself. Instead, you have to
call C<Bugzilla::Field::Choice-E<gt>type($field)> to get the class
you're going to instantiate, and then you call the methods on that.

We do that because each field has its own database table for its values, so
each value type needs its own class.

See the L</SYNOPSIS> for examples of how this works.

=head1 METHODS

=head2 Class Factory

In object-oriented design, a "class factory" is a method that picks
and returns the right class for you, based on an argument that you pass.

=over

=item C<type>

Takes a single argument, which is either the name of a field from the
C<fielddefs> table, or a L<Bugzilla::Field> object representing a field.

Returns an appropriate subclass of C<Bugzilla::Field::Choice> that you
can now call class methods on (like C<new>, C<create>, C<match>, etc.)

B<NOTE>: YOU CANNOT CALL CLASS METHODS ON C<Bugzilla::Field::Choice>. You
must call C<type> to get a class you can call methods on.

=back

=head2 Accessors

These are in addition to the standard L<Bugzilla::Object> accessors.

=over

=item C<sortkey>

The key that determines the sort order of this item.

=item C<field>

The L<Bugzilla::Field> object that this field value belongs to.

=back
