/*
The contents of this file are subject to the Mozilla Public License
Version 1.1 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations
under the License.

The Original Code is expat.

The Initial Developer of the Original Code is James Clark.
Portions created by James Clark are Copyright (C) 1998, 1999
James Clark. All Rights Reserved.

Contributor(s):

Alternatively, the contents of this file may be used under the terms
of the GNU General Public License (the "GPL"), in which case the
provisions of the GPL are applicable instead of those above.  If you
wish to allow use of your version of this file only under the terms of
the GPL and not to allow others to use your version of this file under
the MPL, indicate your decision by deleting the provisions above and
replace them with the notice and other provisions required by the
GPL. If you do not delete the provisions above, a recipient may use
your version of this file under either the MPL or the GPL.
*/

#include "codepage.h"

#ifdef WIN32
#define STRICT 1
#define WIN32_LEAN_AND_MEAN 1

#include <windows.h>

int codepageMap(int cp, int *map)
{
  int i;
  CPINFO info;
  if (!GetCPInfo(cp, &info) || info.MaxCharSize > 2)
    return 0;
  for (i = 0; i < 256; i++)
    map[i] = -1;
  if (info.MaxCharSize > 1) {
    for (i = 0; i < MAX_LEADBYTES; i++) {
      int j, lim;
      if (info.LeadByte[i] == 0 && info.LeadByte[i + 1] == 0)
        break;
      lim = info.LeadByte[i + 1];
      for (j = info.LeadByte[i]; j < lim; j++)
	map[j] = -2;
    }
  }
  for (i = 0; i < 256; i++) {
   if (map[i] == -1) {
     char c = i;
     unsigned short n;
     if (MultiByteToWideChar(cp, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
		             &c, 1, &n, 1) == 1)
       map[i] = n;
   }
  }
  return 1;
}

int codepageConvert(int cp, const char *p)
{
  unsigned short c;
  if (MultiByteToWideChar(cp, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
		          p, 2, &c, 1) == 1)
    return c;
  return -1;
}

#else /* not WIN32 */

int codepageMap(int cp, int *map)
{
  return 0;
}

int codepageConvert(int cp, const char *p)
{
  return -1;
}

#endif /* not WIN32 */
