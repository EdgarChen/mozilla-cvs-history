/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/*
 * functions to manage large numbers of USENET newsgroups
 * store them on disk, free them, search them, etc.
 *
 */

#ifndef MKNEWSGR_H
#define MKNEWSGR_H

/* save a newsgroup name
 *
 * will munge the newsgroup name passed in for efficiency
 *
 */
extern void NET_StoreNewsGroup(char *hostname, XP_Bool is_secure, char * newsgroup);

/* free up the list of newsgroups
 */
extern void NET_FreeNewsgroups(char *hostname, XP_Bool is_secure);

/* sort the newsgroups
 */
extern void NET_SortNewsgroups(char *hostname, XP_Bool is_secure);

/* Search and display newsgroups
 */
extern int NET_DisplayNewsgroups(MWContext *context,
								 char *hostname,
								 Bool is_secure, 
								 char *search_string);

/* Save them out to disk
 */
extern void NET_SaveNewsgroupsToDisk(char *hostname, XP_Bool is_secure);

/* read them from disk
 */
extern time_t NET_ReadNewsgroupsFromDisk(char *hostname, XP_Bool is_secure);

/* Get the last access date, load the newsgroups from
 * disk if they are not loaded
 */
extern time_t NET_NewsgroupsLastUpdatedTime(char *hostname, XP_Bool is_secure);

#endif

