/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code, released
 * Jan 28, 2003.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2003 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *    Garrett Arch Blythe, 28-January-2003
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 */

#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif


MOZCE_SHUNT_API int mozce_chmod(const char* inFilename, int inMode)
{
#ifdef DEBUG
    printf("mozce_chmod called\n");
#endif
    
    int retval = -1;
    
    if(NULL != inFilename)
    {
        unsigned short buffer[MAX_PATH];
        
        int convRes = a2w_buffer(inFilename, -1, buffer, sizeof(buffer) / sizeof(unsigned short));
        if(0 != convRes)
        {
            DWORD attribs = 0;
            
            attribs = GetFileAttributesW(buffer);
            if(0 != attribs)
            {
                if(0 != (_S_IWRITE & inMode))
                {
                    attribs |= FILE_ATTRIBUTE_READONLY;
                }
                else
                {
                    attribs &= ~FILE_ATTRIBUTE_READONLY;
                }
                
                BOOL setRes = SetFileAttributesW(buffer, attribs);
                if(FALSE != setRes)
                {
                    retval = 0;
                }
            }
        }
    }
    
    return retval;
}


MOZCE_SHUNT_API int mozce_isatty(int inHandle)
{
#ifdef DEBUG
    printf("mozce_isatty called\n");
#endif
    
    int retval = 0;
    
    return retval;
}


#if 0
{
#endif
} /* extern "C" */

