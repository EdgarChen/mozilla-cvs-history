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
# The Original Code is Litmus.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Zach Lipton <zach@zachlipton.com>

# Caching of product/group/subgroup data for litmusconfig.js

package Litmus::Cache;

use strict;

use Litmus;
use Data::JavaScript;
use Litmus::DB::Product;

our @ISA = qw(Exporter);
@Litmus::Cache::EXPORT = qw(
    rebuildCache
);

# generate a new litmusconfig.js file because something has updated:
sub rebuildCache {
    unless (-e "data") { system("mkdir", "data") }
    open(CACHE, ">data/litmusconfig.js.new");
    
    print CACHE "// Litmus configuration information\n";
    print CACHE "// Do not edit this file directly. It is autogenerated from the database\n";
    print CACHE "\n";
    
    # we build up %litmusconfig, a big perl data structure, and spit the 
    # whole thing out as javascript with Data::JavaScript
    my %litmusconfig;
    my @products = Litmus::DB::Product->retrieve_all();
    foreach my $curproduct (@products) {
        $litmusconfig{$curproduct->productid()} = {};
        my $prodrow = \%{$litmusconfig{$curproduct->productid()}};
        $prodrow->{"name"} = $curproduct->name();
        $prodrow->{"testgroups"} = {};
        
        my @testgroups = $curproduct->testgroups();
        foreach my $curgroup (@testgroups) {
            $prodrow->{"testgroups"}->{$curgroup->testgroupid()} = {};
            my $grouprow = \%{$prodrow->{"testgroups"}->{$curgroup->testgroupid()}};
            $grouprow->{"name"} = $curgroup->name();
            
            my @subgroups = $curgroup->subgroups();
            foreach my $cursubgroup (@subgroups) {
                $grouprow->{"subgroups"}->{$cursubgroup->subgroupid()} = {};
                my $subgrouprow = \%{$grouprow->{"subgroups"}->{$cursubgroup->subgroupid()}};
                $subgrouprow->{"name"} = $cursubgroup->name();
            }
        }
    }
        
    
    print CACHE jsdump('litmusconfig', \%litmusconfig);
    close(CACHE);
    system("mv", "data/litmusconfig.js.new", "data/litmusconfig.js");
    system("chmod", "-R", "755", "data/");
}

1;