# -*- Mode: perl; tab-width: 4; indent-tabs-mode: nil; -*-
#
# This file is MPL/GPL dual-licensed under the following terms:
# 
# The contents of this file are subject to the Mozilla Public License
# Version 1.1 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and
# limitations under the License.
#
# The Original Code is PLIF 1.0.
# The Initial Developer of the Original Code is Ian Hickson.
#
# Alternatively, the contents of this file may be used under the terms
# of the GNU General Public License Version 2 or later (the "GPL"), in
# which case the provisions of the GPL are applicable instead of those
# above. If you wish to allow use of your version of this file only
# under the terms of the GPL and not to allow others to use your
# version of this file under the MPL, indicate your decision by
# deleting the provisions above and replace them with the notice and
# other provisions required by the GPL. If you do not delete the
# provisions above, a recipient may use your version of this file
# under either the MPL or the GPL.

package PLIF::Service::AdminCommands;
use strict;
use vars qw(@ISA);
use PLIF::Service;
@ISA = qw(PLIF::Service);
1;

# Any application that uses PLIF::Service::AdminCommands must also
# implement "setupFailed($result)" and "setupSucceeded()" in any
# output services that might get called in a Setup context, as well as
# default string data sources for any protocols that might be used in
# a Setup context that use the Generic output module other than
# 'stdout' which is supported by this module itself.

sub provides {
    my $class = shift;
    my($service) = @_;
    return ($service eq 'input.verify' or 
            $service eq 'dispatcher.output.generic' or 
            $service eq 'dataSource.strings.default' or
            $class->SUPER::provides($service));
}

sub objectProvides {
    my $class = shift;
    my($service) = @_;
    return ($service eq 'dispatcher.commands' or 
            $class->SUPER::objectProvides($service));
}

# input.verify
sub verifyInput {
    my $self = shift;
    my($app) = @_;
    if ($app->input->isa('PLIF::Input::CommandLine')) {
        $app->addObject($self);
    }
    return;
}

# dispatcher.commands
sub cmdSetup {
    my $self = shift;
    my($app) = @_;
    my $result;
    $result = $app->getSelectingServiceList('setup.configure')->setupConfigure($app);
    if (not $result) {
        $result = $app->getSelectingServiceList('setup.install')->setupInstall($app);
    }
    if ($result) {
        $app->output->setupFailed($result);
    } else {
        $app->output->setupSucceeded();
    }
}

# dispatcher.output.generic
sub outputSetupSucceeded {
    my $self = shift;
    my($app, $output) = @_;
    $output->output(undef, 'setup', {
        'failed' => 0,
    });
}

# dispatcher.output.generic as well
sub outputSetupFailed {
    my $self = shift;
    my($app, $output, $result) = @_;
    $output->output(undef, 'setup', {
        'failed' => 1,
        'result' => $result,
    });
}

# dataSource.strings.default
sub getDefaultString {
    my $self = shift;
    my($app, $protocol, $string) = @_;
    if ($protocol eq 'stdout' and $string eq 'setup') {
        return '<text><if lvalue="(data.failed)" condition="=" rvalue="1">Failed with:<br/><text variable="(data.result)"/></if><else>Succeeded!</else><br/></text>';
    } else {
        return; # nope, sorry
    }
}

