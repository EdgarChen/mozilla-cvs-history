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
# The Original Code are the Bugzilla Tests.
# 
# The Initial Developer of the Original Code is Zach Lipton
# Portions created by Zach Lipton are 
# Copyright (C) 2002 Zach Lipton.  All
# Rights Reserved.
# 
# Contributor(s): Zach Lipton <zach@zachlipton.com>
# 
# Alternatively, the contents of this file may be used under the
# terms of the GNU General Public License Version 2 or later (the
# "GPL"), in which case the provisions of the GPL are applicable 
# instead of those above.  If you wish to allow use of your 
# version of this file only under the terms of the GPL and not to
# allow others to use your version of this file under the MPL,
# indicate your decision by deleting the provisions above and
# replace them with the notice and other provisions required by
# the GPL.  If you do not delete the provisions above, a recipient
# may use your version of this file under either the MPL or the
# GPL.
# 

#################
#Bugzilla Test 7#
#####Util.pm#####

use lib 't';
use Support::Files;

BEGIN { 
	use Test::More tests => 10;
	use_ok(Bugzilla::Util);
}

# we don't test the taint functions since that's going to take some more work.
# XXX: test taint functions

#html_quote():
is(html_quote("<lala&>"),"&lt;lala&amp;&gt;",'html_quote');

#url_quote():
is(url_quote("<lala&>gaa\"'[]{\\"),"%3Clala%26%3Egaa%22%27%5B%5D%7B%5C",'url_quote');

#value_quote():
is(value_quote("<lal\na&>g\naa\"'[\n]{\\"),"&lt;lal&#013;a&amp;&gt;g&#013;aa&quot;'[&#013;]{\\",'value_quote');

#lsearch():
my @list = ('apple','pear','plum','<"\\%');
is(lsearch(\@list,'pear'),1,'lsearch 1');
is(lsearch(\@list,'<"\\%'),3,'lsearch 2');
is(lsearch(\@list,'kiwi'),-1,'lsearch 3 (missing item)');

#max() and min():
my @list = (7,27,636,2);
is(max(@list),636,'max()');
is(min(@list),2,'min()');

#trim():
is(trim(" fg<*\$%>+=~~ "),'fg<*$%>+=~~','trim()');
