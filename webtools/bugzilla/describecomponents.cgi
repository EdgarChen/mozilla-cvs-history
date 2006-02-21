#!/usr/bin/perl -wT
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
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>

use strict;
use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
require "globals.pl";

use vars qw(@legal_product);

my $user = Bugzilla->login();

GetVersionTable();

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
my $vars = {};
my $product = trim($cgi->param('product') || '');
my $product_id = get_product_id($product);

if (!$product_id || !$user->can_enter_product($product)) {
    # Products which the user is allowed to see.
    my @products = @{$user->get_enterable_products()};

    if (scalar(@products) == 0) {
        ThrowUserError("no_products");
    }
    elsif (scalar(@products) > 1) {
        # XXX - For backwards-compatibility with old template
        # interfaces, we now create a proddesc hash. This can go away
        # once we update the templates.
        my %product_desc;
        foreach my $product (@products) {
            $product_desc{$product->name} = $product->description;
        }
        $vars->{'proddesc'} = \%product_desc;
        $vars->{'target'} = "describecomponents.cgi";
        # If an invalid product name is given, or the user is not
        # allowed to access that product, a message is displayed
        # with a list of the products the user can choose from.
        if ($product) {
            $vars->{'message'} = "product_invalid";
            $vars->{'product'} = $product;
        }

        print $cgi->header();
        $template->process("global/choose-product.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }

    # Else, if there is only one product:
    $product = $products[0]->name;
    $product_id = $products[0]->id;
}

######################################################################
# End Data/Security Validation
######################################################################

my @components;
my $comps = $dbh->selectall_arrayref(
                  q{SELECT name, initialowner, initialqacontact, description
                      FROM components
                     WHERE product_id = ?
                  ORDER BY name}, undef, $product_id);
foreach my $comp (@$comps) {
    my ($name, $initialowner, $initialqacontact, $description) = @$comp;
    my %component;

    $component{'name'} = $name;
    $component{'initialowner'} = $initialowner ?
      DBID_to_name($initialowner) : '';
    $component{'initialqacontact'} = $initialqacontact ?
      DBID_to_name($initialqacontact) : '';
    $component{'description'} = $description;

    push @components, \%component;
}

$vars->{'product'} = $product;
$vars->{'components'} = \@components;

print $cgi->header();
$template->process("reports/components.html.tmpl", $vars)
  || ThrowTemplateError($template->error());
