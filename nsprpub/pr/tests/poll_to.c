/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is the Netscape Portable Runtime (NSPR).
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

/***********************************************************************
**
** Name: prpoll_to.c
**
** Description: This program tests PR_Poll with sockets.
**              Timeout operation is tested
**
** Modification History:
** 14-May-97 AGarcia- Converted the test to accomodate the debug_mode flag.
**	         The debug mode will print all of the printfs associated with this test.
**			 The regress mode will be the default mode. Since the regress tool limits
**           the output to a one line status:PASS or FAIL,all of the printf statements
**			 have been handled with an if (debug_mode) statement.
** 04-June-97 AGarcia removed the Test_Result function. Regress tool has been updated to
**			recognize the return code from tha main program.
***********************************************************************/

/***********************************************************************
** Includes
***********************************************************************/
/* Used to get the command line option */
#include "plgetopt.h"

#include "prinit.h"
#include "prio.h"
#include "prlog.h"
#include "prprf.h"
#include "prnetdb.h"

#ifndef XP_MAC
#include "private/pprio.h"
#else
#include "pprio.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nst_wince.h"

PRIntn failed_already=0;
PRIntn debug_mode;

int main(int argc, char **argv)
{
    PRFileDesc *listenSock1 = NULL, *listenSock2 = NULL;
    PRUint16 listenPort1, listenPort2;
    PRNetAddr addr;
    char buf[128];
    PRPollDesc pds0[10], pds1[10], *pds, *other_pds;
    PRIntn npds;
    PRInt32 retVal;

	/* The command line argument: -d is used to determine if the test is being run
	in debug mode. The regress tool requires only one line output:PASS or FAIL.
	All of the printfs associated with this test has been handled with a if (debug_mode)
	test.
	Usage: test_name -d
	*/
	PLOptStatus os;
	PLOptState *opt = PL_CreateOptState(argc, argv, "d:");
	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
		if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'd':  /* debug mode */
			debug_mode = 1;
            break;
         default:
            break;
        }
    }
	PL_DestroyOptState(opt);

 /* main test */
	
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

    if (debug_mode) {
		printf("This program tests PR_Poll with sockets.\n");
		printf("Timeout is tested.\n\n");
	}

    /* Create two listening sockets */
    if ((listenSock1 = PR_NewTCPSocket()) == NULL) {
	fprintf(stderr, "Can't create a new TCP socket\n");
	if (!debug_mode)  failed_already=1;
	goto exit_now;
    }
    memset(&addr, 0, sizeof(addr));
    addr.inet.family = PR_AF_INET;
    addr.inet.ip = PR_htonl(PR_INADDR_ANY);
    addr.inet.port = PR_htons(0);
    if (PR_Bind(listenSock1, &addr) == PR_FAILURE) {
	fprintf(stderr, "Can't bind socket\n");
	if (!debug_mode)  failed_already=1;
	goto exit_now;
    }
    if (PR_GetSockName(listenSock1, &addr) == PR_FAILURE) {
	fprintf(stderr, "PR_GetSockName failed\n");
	if (!debug_mode)  failed_already=1;
	goto exit_now;
    }
    listenPort1 = PR_ntohs(addr.inet.port);
    if (PR_Listen(listenSock1, 5) == PR_FAILURE) {
	fprintf(stderr, "Can't listen on a socket\n");
	if (!debug_mode)  failed_already=1;
	goto exit_now;
    }

    if ((listenSock2  = PR_NewTCPSocket()) == NULL) {
	fprintf(stderr, "Can't create a new TCP socket\n");
	if (!debug_mode)  failed_already=1;	
	goto exit_now;
    }
    addr.inet.family = PR_AF_INET;
    addr.inet.ip = PR_htonl(PR_INADDR_ANY);
    addr.inet.port = PR_htons(0);
    if (PR_Bind(listenSock2, &addr) == PR_FAILURE) {
	fprintf(stderr, "Can't bind socket\n");
	if (!debug_mode)  failed_already=1;	
	goto exit_now;
    }
    if (PR_GetSockName(listenSock2, &addr) == PR_FAILURE) {
	fprintf(stderr, "PR_GetSockName failed\n");
	if (!debug_mode)  failed_already=1;	
	goto exit_now;
    }
    listenPort2 = PR_ntohs(addr.inet.port);
    if (PR_Listen(listenSock2, 5) == PR_FAILURE) {
	fprintf(stderr, "Can't listen on a socket\n");
	if (!debug_mode)  failed_already=1;	
	goto exit_now;
    }
    PR_snprintf(buf, sizeof(buf),
	    "The server thread is listening on ports %hu and %hu\n\n",
	    listenPort1, listenPort2);
    if (debug_mode) printf("%s", buf);

    /* Set up the poll descriptor array */
    pds = pds0;
    other_pds = pds1;
    memset(pds, 0, sizeof(pds));
    pds[0].fd = listenSock1;
    pds[0].in_flags = PR_POLL_READ;
    pds[1].fd = listenSock2;
    pds[1].in_flags = PR_POLL_READ;
    npds = 2;

    /* Testing timeout */
    if (debug_mode) printf("PR_Poll should time out in 5 seconds\n");
    retVal = PR_Poll(pds, npds, PR_SecondsToInterval(5));
    if (retVal != 0) {
	PR_snprintf(buf, sizeof(buf),
		"PR_Poll should time out and return 0, but it returns %ld\n",
		retVal);
	fprintf(stderr, "%s", buf);
	if (!debug_mode)  failed_already=1;	
	goto exit_now;
    }
    if (debug_mode) printf("PR_Poll timed out.  Test passed.\n\n");

exit_now:

    if (listenSock1) {
        PR_Close(listenSock1);
    }
    if (listenSock2) {
        PR_Close(listenSock2);
    }

    PR_Cleanup();

	if(failed_already)	
		return 1;
	else
		return 0;

}
