/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "prmem.h"
#include "prmon.h"
#include "prlog.h"
#include "prthread.h"
#include "fe_proto.h"
#include "xp.h"
#include "xp_str.h"
#include "xpgetstr.h"
#include "prefapi.h"
#include "proto.h"
#include "prprf.h"
#include "prthread.h"
#include "nsFolderSpec.h"
#include "nsSUError.h"
#include "su_folderspec.h"

#ifndef MAX_PATH
#if defined(XP_WIN) || defined(XP_OS2)
#define MAX_PATH _MAX_PATH
#endif
#ifdef XP_UNIX
#ifdef NSPR20
#include "md/prosdep.h"
#else
#include "prosdep.h"
#endif
#if defined(HPUX) || defined(SCO)
/*
** HPUX: PATH_MAX is defined in <limits.h> to be 1023, but they
**       recommend that it not be used, and that pathconf() be
**       used to determine the maximum at runtime.
** SCO:  This is what MAXPATHLEN is set to in <arpa/ftp.h> and
**       NL_MAXPATHLEN in <nl_types.h>.  PATH_MAX is defined in
**       <limits.h> to be 256, but the comments in that file
**       claim the setting is wrong.
*/
#define MAX_PATH 1024
#else
#define MAX_PATH PATH_MAX
#endif
#endif
#endif

extern int SU_INSTALL_ASK_FOR_DIRECTORY;


PR_BEGIN_EXTERN_C

/* Public Methods */

/* Constructor
 */
nsFolderSpec::nsFolderSpec(char* inFolderID , char* inVRPath, char* inPackageName)
{
  urlPath = folderID = versionRegistryPath = userPackageName = NULL;

  /* May be we should return an error message */
  if ((inFolderID == NULL) || (inVRPath == NULL) || (inPackageName == NULL)) {
    return;
  }

  folderID = XP_STRDUP(inFolderID);
  versionRegistryPath = XP_STRDUP(inVRPath);
  userPackageName = XP_STRDUP(inPackageName);
}

nsFolderSpec::~nsFolderSpec(void)
{
  if (folderID) 
    XP_FREE(folderID);
  if (versionRegistryPath) 
    XP_FREE(versionRegistryPath);
  if (userPackageName) 
    XP_FREE(userPackageName);
  if (urlPath) 
    XP_FREE(urlPath);
}

/*
 * GetDirectoryPath
 * returns full path to the directory in the standard URL form
 *
 * Caller shouldn't free the returned value. It returns it copy.
 *
 */
char* nsFolderSpec::GetDirectoryPath(char* *errorMsg)
{
  if ((folderID == NULL) || (versionRegistryPath == NULL)) {
    *errorMsg = SU_GetErrorMsg3("Invalid arguments to the constructor", 
                               nsSoftUpdateError_INVALID_ARGUMENTS);
    return NULL;
  }

  if (urlPath == NULL) {
    if (XP_STRCMP(folderID, "User Pick") == 0)  {
      // Default package folder

      // Try the version registry default path first
      // urlPath = VersionRegistry.getDefaultDirectory( versionRegistryPath );
      // if (urlPath == NULL)
      urlPath = PickDefaultDirectory(errorMsg);

    } else if (XP_STRCMP(folderID, "Installed") == 0)  {
      // Then use the Version Registry path
      urlPath = XP_STRDUP(versionRegistryPath);
    } else {
      // Built-in folder
      /* NativeGetDirectoryPath updates urlPath */
      int err = NativeGetDirectoryPath();
      if (err != 0)
        *errorMsg = SU_GetErrorMsg3(folderID, err);
    }
  }
  return urlPath;
}

/**
 * Returns full path to a file. Makes sure that the full path is bellow
 * this directory (security measure)
 *
 * Caller should free the returned string
 *
 * @param relativePath      relative path
 * @return                  full path to a file
 */
char* nsFolderSpec::MakeFullPath(char* relativePath, char* *errorMsg)
{
  // Security check. Make sure that we do not have '.. in the name
  // if ( (GetSecurityTargetID() == SoftwareUpdate.LIMITED_INSTALL) &&
  //        ( relativePath.regionMatches(0, "..", 0, 2)))
  //    throw new SoftUpdateException(Strings.error_IllegalPath(), SoftwareUpdate.ILLEGAL_RELATIVE_PATH );
  char *fullPath=NULL;
  char *dir_path;
  *errorMsg = NULL;
  dir_path = GetDirectoryPath(errorMsg);
  if (errorMsg != NULL) {
    return NULL;
  }
  fullPath = XP_Cat(dir_path, GetNativePath(relativePath));
  return fullPath;
}

/* 
 * The caller is supposed to free the memory. 
 */
char* nsFolderSpec::toString()
{
  char *errorMsg = NULL;
  char* path = GetDirectoryPath(&errorMsg);
  if (errorMsg != NULL) {
    path = NULL;
  } else {
    PR_ASSERT(path != NULL);
    XP_STRDUP(path);
  }
  return path;
}


/* Private Methods */

/* PickDefaultDirectory
 * asks the user for the default directory for the package
 * stores the choice
 */
char* nsFolderSpec::PickDefaultDirectory(char* *errorMsg)
{
  urlPath = NativePickDefaultDirectory();

  if (urlPath == NULL)
    *errorMsg = SU_GetErrorMsg3(folderID, nsSoftUpdateError_INVALID_PATH_ERR);

  return urlPath;
}


/* Private Native Methods */

/* NativeGetDirectoryPath
 * gets a platform-specific directory path
 * stores it in urlPath
 */
int nsFolderSpec::NativeGetDirectoryPath()
{
  su_DirSpecID folderDirSpecID;
  char*	folderPath = NULL;

  /* Get the name of the package to prompt for */

  folderDirSpecID = MapNameToEnum(folderID);
  switch (folderDirSpecID) 
    {
    case eBadFolder:
      return nsSoftUpdateError_INVALID_PATH_ERR;

    case eCurrentUserFolder:
      {
        char dir[MAX_PATH];
        int len = MAX_PATH;
        if ( PREF_GetCharPref("profile.directory", dir, &len) == PREF_NOERROR)      
          {
            char * platformDir = WH_FileName(dir, xpURL);
            if (platformDir)
              folderPath = AppendSlashToDirPath(platformDir);
            XP_FREEIF(platformDir);
          }
      }
    break;

    default:
      /* Get the FE path */
      folderPath = FE_GetDirectoryPath(folderDirSpecID);
      break;
    }


  /* Store it in the object */
  if (folderPath != NULL) {
    urlPath = NULL;
    urlPath = XP_STRDUP(folderPath);
    XP_FREE(folderPath);
    return 0;
  }

  return nsSoftUpdateError_INVALID_PATH_ERR;
}

/* GetNativePath
 * returns a native equivalent of a XP directory path
 */
char* nsFolderSpec::GetNativePath(char* path)
{
  char *xpPath, *p;
  char pathSeparator;

#define XP_PATH_SEPARATOR  '/'

#ifdef XP_WIN
  pathSeparator = '\\';
#elif defined(XP_MAC)
  pathSeparator = ':';
#else /* XP_UNIX */
  pathSeparator = '/';
#endif

  p = xpPath = path;

  /*
   * Traverse XP path and replace XP_PATH_SEPARATOR with
   * the platform native equivalent
   */
  if ( p == NULL )
    {
      xpPath = "";
    }
  else
    {
      while ( *p )
        {
          if ( *p == XP_PATH_SEPARATOR )
            *p = pathSeparator;
          ++p;
        }
    }

  return xpPath;
}

/*
 * NativePickDefaultDirectory
 * Platform-specific implementation of GetDirectoryPath
 */
char* nsFolderSpec::NativePickDefaultDirectory(void)
{
  su_PickDirTimer callback;
  char * packageName;
  char prompt[200];

 
  callback.context = XP_FindSomeContext();
  callback.fileName = NULL;
  callback.done = FALSE;
  /* Get the name of the package to prompt for */
  packageName = userPackageName;

  if (packageName)
    {
      /* In Java thread now, and need to call FE_PromptForFileName
       * from the main thread
       * post an event on a timer, and busy-wait until it completes.
       */
      PR_snprintf(prompt, 200, XP_GetString(SU_INSTALL_ASK_FOR_DIRECTORY), packageName); 
      callback.prompt = prompt;
      FE_SetTimeout( pickDirectoryCallback, &callback, 1 );
      while (!callback.done)  /* Busy loop for now */
        PR_Sleep(PR_INTERVAL_NO_WAIT); /* java_lang_Thread_yield(WHAT?); */
    }

  return callback.fileName;
}

PRBool nsFolderSpec::NativeIsJavaDir()
{
  char* folderName;

  /* Get the name of the package to prompt for */
  folderName = folderID;

  PR_ASSERT( folderName );
  if ( folderName != NULL) {
    int i;
    for (i=0; DirectoryTable[i].directoryName[0] != 0; i++ ) {
      if ( XP_STRCMP(folderName, DirectoryTable[i].directoryName) == 0 )
        return DirectoryTable[i].bJavaDir;
    }
  }

  return PR_FALSE;
}

PR_END_EXTERN_C

