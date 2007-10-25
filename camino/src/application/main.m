/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Simon Fraser <sfraser@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#import <Cocoa/Cocoa.h>

#include <sys/resource.h>
#include <sys/types.h>

static void SetupRuntimeOptions(int argc, const char *argv[])
{
  if (getenv("MOZ_UNBUFFERED_STDIO")) {
    printf("stdout and stderr unbuffered\n");
    setbuf(stdout, 0);
    setbuf(stderr, 0);
  }
}

static void SetMaxFileDescriptors(int target)
{
  struct rlimit rl;
  if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
    if (rl.rlim_max > 0 && rl.rlim_max < target)
      target = rl.rlim_max;
    if (target > rl.rlim_cur) {
      rl.rlim_cur = target;
      setrlimit(RLIMIT_NOFILE, &rl);
    }
  }
}

int main(int argc, const char *argv[])
{
  SetupRuntimeOptions(argc, argv);

  // Because of a nasty file descriptor leak when viewing flash
  // (bug 397053), bump up our limit up to 1024 so that it takes longer for
  // the world to end.
  SetMaxFileDescriptors(1024);

  return NSApplicationMain(argc, argv);
}
