/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * The Original Code is Mozilla Communicator client code,
 * released March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Sean Su <ssu@netscape.com>
 *     Curt Patrick <curt@netscape.com>
 */

#include "extern.h"
#include "extra.h"
#include "dialogs.h"
#include "shortcut.h"
#include "ifuncns.h"
#include "wizverreg.h"
#include "logging.h"
#include <logkeys.h>

HRESULT TimingCheck(DWORD dwTiming, LPSTR szSection, LPSTR szFile)
{
  char szBuf[MAX_BUF_TINY];

  GetPrivateProfileString(szSection, "Timing", "", szBuf, sizeof(szBuf), szFile);
  if(*szBuf != '\0')
  {
    switch(dwTiming)
    {
      case T_PRE_DOWNLOAD:
        if(strcmpi(szBuf, "pre download") == 0)
          return(TRUE);
        break;

      case T_POST_DOWNLOAD:
        if(strcmpi(szBuf, "post download") == 0)
          return(TRUE);
        break;

      case T_PRE_XPCOM:
        if(strcmpi(szBuf, "pre xpcom") == 0)
          return(TRUE);
        break;

      case T_POST_XPCOM:
        if(strcmpi(szBuf, "post xpcom") == 0)
          return(TRUE);
        break;

      case T_PRE_SMARTUPDATE:
        if(strcmpi(szBuf, "pre smartupdate") == 0)
          return(TRUE);
        break;

      case T_POST_SMARTUPDATE:
        if(strcmpi(szBuf, "post smartupdate") == 0)
          return(TRUE);
        break;

      case T_PRE_LAUNCHAPP:
        if(strcmpi(szBuf, "pre launchapp") == 0)
          return(TRUE);
        break;

      case T_POST_LAUNCHAPP:
        if(strcmpi(szBuf, "post launchapp") == 0)
          return(TRUE);
        break;

      case T_PRE_ARCHIVE:
        if(strcmpi(szBuf, "pre archive") == 0)
          return(TRUE);
        break;

      case T_POST_ARCHIVE:
        if(strcmpi(szBuf, "post archive") == 0)
          return(TRUE);
        break;

      case T_DEPEND_REBOOT:
        if(strcmpi(szBuf, "depend reboot") == 0)
          return(TRUE);
        break;
    }
  }
  return(FALSE);
}

char *BuildNumberedString(DWORD dwIndex, char *szInputStringPrefix, char *szInputString, char *szOutBuf, DWORD dwOutBufSize)
{
  if((szInputStringPrefix) && (*szInputStringPrefix != '\0'))
    sprintf(szOutBuf, "%s-%s%d", szInputStringPrefix, szInputString, dwIndex);
  else
    sprintf(szOutBuf, "%s%d", szInputString, dwIndex);

  return(szOutBuf);
}

void GetUserAgentShort(char *szUserAgent, char *szOutUAShort, DWORD dwOutUAShortSize)
{
  char *ptrFirstSpace = NULL;

  memset(szOutUAShort, 0, dwOutUAShortSize);
  if((szUserAgent == NULL) || (*szUserAgent == '\0'))
    return;

  ptrFirstSpace = strstr(szUserAgent, " ");
  if(ptrFirstSpace != NULL)
  {
    *ptrFirstSpace = '\0';
    strcpy(szOutUAShort, szUserAgent);
    *ptrFirstSpace = ' ';
  }
}

/* EXPLANATION
Enumerate through a given subkey and check to see if any keys have the exact same install directory
If they do, delete them. Very straightforward */
DWORD GetWinRegSubKeyProductPath(char *szInKey, char *szReturnSubKey, DWORD dwReturnSubKeySize, char *szInSubSubKey, char *szInName, char *szCompare, char *szInCurrentVersion)
{
  char      *szRv = NULL;
  char      szKey[MAX_BUF];
  char      szBuf[MAX_BUF];
//  HKEY      hkHandle;
  DWORD     dwIndex;
  DWORD     dwBufSize;
  DWORD     dwTotalSubKeys;
  DWORD     dwTotalValues;
//  FILETIME  ftLastWriteFileTime;
  BOOL      bFoundSubKey;

  bFoundSubKey = FALSE;

//  if(RegOpenKeyEx(hkRootKey, szInKey, 0, KEY_READ, &hkHandle) != ERROR_SUCCESS)
  {
    *szReturnSubKey = '\0';
    return(0);
  }

  dwTotalSubKeys = 0;
  dwTotalValues  = 0;
//  RegQueryInfoKey(hkHandle, NULL, NULL, NULL, &dwTotalSubKeys, NULL, NULL, &dwTotalValues, NULL, NULL, NULL, NULL);
  for(dwIndex = 0; dwIndex < dwTotalSubKeys; dwIndex++)
  {
    dwBufSize = dwReturnSubKeySize;
//    if(RegEnumKeyEx(hkHandle, dwIndex, szReturnSubKey, &dwBufSize, NULL, NULL, NULL, &ftLastWriteFileTime) == ERROR_SUCCESS)
    {
      if(  (*szInCurrentVersion != '\0') && (strcmpi(szInCurrentVersion, szReturnSubKey) != 0)  )
      {
        /* The key found is not the CurrentVersion (current UserAgent), so we can return it to be deleted.
         * We don't want to return the SubKey that is the same as the CurrentVersion because it might
         * have just been created by the current installation process.  So deleting it would be a
         * "Bad Thing" (TM).
         *
         * If it was not created by the current installation process, then it'll be left
         * around which is better than deleting something we will need later. To make sure this case is
         * not encountered, CleanupPreviousVersionRegKeys() should be called at the *end* of the
         * installation process (at least after all the .xpi files have been processed). */
        if(szInSubSubKey && (*szInSubSubKey != '\0'))
          sprintf(szKey, "%s\\%s\\%s", szInKey, szReturnSubKey, szInSubSubKey);
        else
          sprintf(szKey, "%s\\%s", szInKey, szReturnSubKey);

  //      GetWinReg(hkRootKey, szKey, szInName, szBuf, sizeof(szBuf));
        AppendBackSlash(szBuf, sizeof(szBuf));
        if(strcmpi(szBuf, szCompare) == 0)
        {
          bFoundSubKey = TRUE;
          /* found one subkey. break out of the for() loop */
          break;
        }
      }
    }
  }

//  RegCloseKey(hkHandle);
  if(!bFoundSubKey)
    *szReturnSubKey = '\0';
  return(dwTotalSubKeys);
}

void CleanupPreviousVersionRegKeys(void)
{
/* OK, OS/2 things to do here */
/* Look at all keys in OS2.INI beginning with Mozilla (Product Reg Key)*/
/* If they contain an Install Directory that is the same as what we just installed, remove the app */



  DWORD dwIndex = 0;
  DWORD dwSubKeyCount;
  char  szBufTiny[MAX_BUF_TINY];
  char  szKeyRoot[MAX_BUF_TINY];
  char  szCurrentVersion[MAX_BUF_TINY];
  char  szUAShort[MAX_BUF_TINY];
  char  szRvSubKey[MAX_PATH + 1];
  char  szPath[MAX_BUF];
  char  szKey[MAX_BUF];
  char  szCleanupProduct[MAX_BUF];
//  HKEY  hkeyRoot;
  char  szSubSubKey[] = "Main";
  char  szName[] = "Install Directory";
  char  szWRMSUninstall[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
  char  szSection[] = "Cleanup Previous Product RegKeys";

  strcpy(szPath, sgProduct.szPath);
  if(*sgProduct.szSubPath != '\0')
  {
    AppendBackSlash(szPath, sizeof(szPath));
    strcat(szPath, sgProduct.szSubPath);
  }
  AppendBackSlash(szPath, sizeof(szPath));

  sprintf(szBufTiny, "Product Reg Key%d", dwIndex);        
  GetPrivateProfileString(szSection, szBufTiny, "", szKey, sizeof(szKey), szFileIniConfig);

  while(*szKey != '\0')
  {
    sprintf(szBufTiny, "Reg Key Root%d",dwIndex);
    GetPrivateProfileString(szSection, szBufTiny, "", szKeyRoot, sizeof(szKeyRoot), szFileIniConfig);
//    hkeyRoot = ParseRootKey(szKeyRoot);

    sprintf(szBufTiny, "Product Name%d", dwIndex);        
    GetPrivateProfileString(szSection, szBufTiny, "", szCleanupProduct, sizeof(szCleanupProduct), szFileIniConfig);
    // something is wrong, they didn't give a product name.
    if(*szCleanupProduct == '\0')
      return;

    sprintf(szBufTiny, "Current Version%d", dwIndex);        
    GetPrivateProfileString(szSection, szBufTiny, "", szCurrentVersion, sizeof(szCurrentVersion), szFileIniConfig);

    do
    {
      // if the current version is not found, we'll get null in szCurrentVersion and GetWinRegSubKeyProductPath() will do the right thing
//      dwSubKeyCount = GetWinRegSubKeyProductPath(hkeyRoot, szKey, szRvSubKey, sizeof(szRvSubKey), szSubSubKey, szName, szPath, szCurrentVersion);
  	  
      if(*szRvSubKey != '\0')
      {
        if(dwSubKeyCount > 1)
        {
          AppendBackSlash(szKey, sizeof(szKey));
          strcat(szKey, szRvSubKey);
        }
//        DeleteWinRegKey(hkeyRoot, szKey, TRUE);

        GetUserAgentShort(szRvSubKey, szUAShort, sizeof(szUAShort));
        if(*szUAShort != '\0')
        {
          /* delete uninstall key that contains product name and its user agent in parenthesis, for
           * example:
           *     Mozilla (0.8)
           */
          sprintf(szKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s (%s)", szCleanupProduct, szUAShort);
//          DeleteWinRegKey(hkeyRoot, szKey, TRUE);

          /* delete uninstall key that contains product name and its user agent not in parenthesis,
           * for example:
           *     Mozilla 0.8
           */
          sprintf(szKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s %s", szCleanupProduct, szUAShort);
//          DeleteWinRegKey(hkeyRoot, szKey, TRUE);

          /* We are not looking to delete just the product name key, for example:
           *     Mozilla
           *
           * because it might have just been created by the current installation process, so
           * deleting this would be a "Bad Thing" (TM).  Besides, we shouldn't be deleting the
           * CurrentVersion key that might have just gotten created because GetWinRegSubKeyProductPath()
           * will not return the CurrentVersion key.
           */
        }
        // the szKey was stepped on.  Reget it.
        sprintf(szBufTiny, "Product Reg Key%d", dwIndex);        
        GetPrivateProfileString(szSection, szBufTiny, "", szKey, sizeof(szKey), szFileIniConfig);
      }
    }  while (*szRvSubKey != '\0');
    sprintf(szBufTiny, "Product Reg Key%d", ++dwIndex);        
    GetPrivateProfileString(szSection, szBufTiny, "", szKey, sizeof(szKey), szFileIniConfig);
  } 

}

void ProcessFileOps(DWORD dwTiming, char *szSectionPrefix)
{
  ProcessUncompressFile(dwTiming, szSectionPrefix);
  ProcessCreateDirectory(dwTiming, szSectionPrefix);
  ProcessMoveFile(dwTiming, szSectionPrefix);
  ProcessCopyFile(dwTiming, szSectionPrefix);
  ProcessCopyFileSequential(dwTiming, szSectionPrefix);
  ProcessDeleteFile(dwTiming, szSectionPrefix);
  ProcessRemoveDirectory(dwTiming, szSectionPrefix);
  if(!gbIgnoreRunAppX)
    ProcessRunApp(dwTiming, szSectionPrefix);
  ProcessOS2INI(dwTiming, szSectionPrefix);
  ProcessProgramFolder(dwTiming, szSectionPrefix);
  ProcessSetVersionRegistry(dwTiming, szSectionPrefix);
}

void ProcessFileOpsForSelectedComponents(DWORD dwTiming)
{
  DWORD dwIndex0;
  siC   *siCObject = NULL;

  dwIndex0  = 0;
  siCObject = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
  while(siCObject)
  {
    if(siCObject->dwAttributes & SIC_SELECTED)
      /* Since the archive is selected, we need to process the file ops here */
      ProcessFileOps(dwTiming, siCObject->szReferenceName);

    ++dwIndex0;
    siCObject = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
  } /* while(siCObject) */
}

void ProcessFileOpsForAll(DWORD dwTiming)
{
  ProcessFileOps(dwTiming, NULL);
  ProcessFileOpsForSelectedComponents(dwTiming);
  ProcessCreateCustomFiles(dwTiming);
}

int VerifyArchive(LPSTR szArchive)
{
  void *vZip;
  int  iTestRv;

  /* Check for the existance of the from (source) file */
  if(!FileExists(szArchive))
    return(FO_ERROR_FILE_NOT_FOUND);

  if((iTestRv = ZIP_OpenArchive(szArchive, &vZip)) == ZIP_OK)
  {
    /* 1st parameter should be NULL or it will fail */
    /* It indicates extract the entire archive */
    iTestRv = ZIP_TestArchive(vZip);
    ZIP_CloseArchive(&vZip);
  }
  return(iTestRv);
}

HRESULT ProcessSetVersionRegistry(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD   dwIndex;
  BOOL    bIsDirectory;
  char    szBuf[MAX_BUF];
  char    szSection[MAX_BUF_TINY];
  char    szRegistryKey[MAX_BUF];
  char    szPath[MAX_BUF];
  char    szVersion[MAX_BUF_TINY];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Version Registry", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Registry Key", "", szRegistryKey, sizeof(szRegistryKey), szFileIniConfig);
  while(*szRegistryKey != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      GetPrivateProfileString(szSection, "Version", "", szVersion, sizeof(szVersion), szFileIniConfig);
      GetPrivateProfileString(szSection, "Path",    "", szBuf,     sizeof(szBuf),     szFileIniConfig);
      DecryptString(szPath, szBuf);
      if(FileExists(szPath) & FILE_DIRECTORY)
        bIsDirectory = TRUE;
      else
        bIsDirectory = FALSE;

      strcpy(szBuf, sgProduct.szPath);
      if(sgProduct.szSubPath != '\0')
      {
        AppendBackSlash(szBuf, sizeof(szBuf));
        strcat(szBuf, sgProduct.szSubPath);
      }

      VR_CreateRegistry(VR_DEFAULT_PRODUCT_NAME, szBuf, NULL);
      VR_Install(szRegistryKey, szPath, szVersion, bIsDirectory);
      VR_Close();
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Version Registry", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Registry Key", "", szRegistryKey, sizeof(szRegistryKey), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT FileUncompress(LPSTR szFrom, LPSTR szTo)
{
  char  szBuf[MAX_BUF];
  ULONG ulBuf, ulDiskNum, ulDriveMap;
  DWORD dwReturn;
  void  *vZip;
  APIRET rc;

  dwReturn = FO_SUCCESS;
  /* Check for the existance of the from (source) file */
  if(!FileExists(szFrom))
    return(FO_ERROR_FILE_NOT_FOUND);

  /* Check for the existance of the to (destination) path */
  dwReturn = FileExists(szTo);
  if(dwReturn && !(dwReturn & FILE_DIRECTORY))
  {
    /* found a file with the same name as the destination path */
    return(FO_ERROR_DESTINATION_CONFLICT);
  }
  else if(!dwReturn)
  {
    strcpy(szBuf, szTo);
    AppendBackSlash(szBuf, sizeof(szBuf));
    CreateDirectoriesAll(szBuf, FALSE);
  }

  ulBuf = MAX_BUF-3;
  rc = DosQueryCurrentDir(0, &szBuf[3], &ulBuf);
  // Directory does not start with 'x:\', so add it.
  rc = DosQueryCurrentDisk(&ulDiskNum, &ulDriveMap);

  // Follow the case of the first letter in the path.
  if (isupper(szBuf[3]))
     szBuf[0] = (char)('A' - 1 + ulDiskNum);
  else
     szBuf[0] = (char)('a' - 1 + ulDiskNum);
  szBuf[1] = ':';
  szBuf[2] = '\\';

  if (toupper(szBuf[0]) != toupper(szTo[0]))
     rc = DosSetDefaultDisk(toupper(szTo[0]) - 'A' + 1);
  if(DosSetCurrentDir(szTo) != NO_ERROR)
    return(FO_ERROR_CHANGE_DIR);

  if((dwReturn = ZIP_OpenArchive(szFrom, &vZip)) != ZIP_OK)
    return(dwReturn);

  /* 1st parameter should be NULL or it will fail */
  /* It indicates extract the entire archive */
  dwReturn = ExtractDirEntries(NULL, vZip);
  ZIP_CloseArchive(&vZip);

  if (toupper(szBuf[0]) != toupper(szTo[0]))
     DosSetDefaultDisk(toupper(szBuf[0]) -1 + 'A');
  if(DosSetCurrentDir(szBuf) != NO_ERROR)
    return(FO_ERROR_CHANGE_DIR);

  return(dwReturn);
}

HRESULT ProcessXpcomFile()
{
  char szSource[MAX_BUF];
  char szDestination[MAX_BUF];
  DWORD dwErr;

  if(*siCFXpcomFile.szMessage != '\0')
    ShowMessage(siCFXpcomFile.szMessage, TRUE);

  if((dwErr = FileUncompress(siCFXpcomFile.szSource, siCFXpcomFile.szDestination)) != FO_SUCCESS)
  {
    char szMsg[MAX_BUF];
    char szErrorString[MAX_BUF];

    if(*siCFXpcomFile.szMessage != '\0')
      ShowMessage(siCFXpcomFile.szMessage, FALSE);

    LogISProcessXpcomFile(LIS_FAILURE, dwErr);
    GetPrivateProfileString("Strings", "Error File Uncompress", "", szErrorString, sizeof(szErrorString), szFileIniConfig);
    sprintf(szMsg, szErrorString, siCFXpcomFile.szSource, dwErr);
    PrintError(szMsg, ERROR_CODE_HIDE);
    return(dwErr);
  }
  LogISProcessXpcomFile(LIS_SUCCESS, dwErr);

  /* copy msvcrt.dll and msvcirt.dll to the bin of the Xpcom temp dir:
   *   (c:\temp\Xpcom.ns\bin)
   * This is incase these files do not exist on the system */
  strcpy(szSource, siCFXpcomFile.szDestination);
  AppendBackSlash(szSource, sizeof(szSource));
  strcat(szSource, "ms*.dll");

  strcpy(szDestination, siCFXpcomFile.szDestination);
  AppendBackSlash(szDestination, sizeof(szDestination));
  strcat(szDestination, "bin");

  FileCopy(szSource, szDestination, TRUE, FALSE);

  if(*siCFXpcomFile.szMessage != '\0')
    ShowMessage(siCFXpcomFile.szMessage, FALSE);

  return(FO_SUCCESS);
}

HRESULT CleanupXpcomFile()
{
  if(siCFXpcomFile.bCleanup == TRUE)
    DirectoryRemove(siCFXpcomFile.szDestination, TRUE);

  return(FO_SUCCESS);
}

HRESULT CleanupArgsRegistry()
{
  char  szApp[MAX_BUF];

  sprintf(szApp, "%s %s", sgProduct.szProductNameInternal, sgProduct.szUserAgent);
  PrfWriteProfileString(HINI_USERPROFILE, szApp, "browserargs", NULL);
  return(FO_SUCCESS);
}

HRESULT ProcessUncompressFile(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD   dwIndex;
  BOOL    bOnlyIfExists;
  char    szBuf[MAX_BUF];
  char    szSection[MAX_BUF];
  char    szSource[MAX_BUF];
  char    szDestination[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Uncompress File", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szSource, szBuf);
      GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szDestination, szBuf);
      GetPrivateProfileString(szSection, "Only If Exists", "", szBuf, sizeof(szBuf), szFileIniConfig);
      if(strcmpi(szBuf, "TRUE") == 0)
        bOnlyIfExists = TRUE;
      else
        bOnlyIfExists = FALSE;

      if((!bOnlyIfExists) || (bOnlyIfExists && FileExists(szDestination)))
      {
        DWORD dwErr;

        GetPrivateProfileString(szSection, "Message",     "", szBuf, sizeof(szBuf), szFileIniConfig);
        ShowMessage(szBuf, TRUE);
        if((dwErr = FileUncompress(szSource, szDestination)) != FO_SUCCESS)
        {
          char szMsg[MAX_BUF];
          char szErrorString[MAX_BUF];

          ShowMessage(szBuf, FALSE);
          GetPrivateProfileString("Strings", "Error File Uncompress", "", szErrorString, sizeof(szErrorString), szFileIniConfig);
          sprintf(szMsg, szErrorString, szSource, dwErr);
          PrintError(szMsg, ERROR_CODE_HIDE);
          return(dwErr);
        }

        ShowMessage(szBuf, FALSE);
      }
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Uncompress File", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT FileMove(LPSTR szFrom, LPSTR szTo)
{
  HDIR            hFile;
  FILEFINDBUF3    fdFile;
  ULONG           ulFindCount;
  char            szFromDir[MAX_BUF];
  char            szFromTemp[MAX_BUF];
  char            szToTemp[MAX_BUF];
  char            szBuf[MAX_BUF];
  BOOL            bFound;

  /* From file path exists and To file path does not exist */
  if((FileExists(szFrom)) && (!FileExists(szTo)))
  {
    
    /* @MAK - need to handle OS/2 case where they are not the same drive*/
    DosMove(szFrom, szTo);

    /* log the file move command */
    sprintf(szBuf, "%s to %s", szFrom, szTo);
    UpdateInstallLog(KEY_MOVE_FILE, szBuf, FALSE);

    return(FO_SUCCESS);
  }
  /* From file path exists and To file path exists */
  if(FileExists(szFrom) && FileExists(szTo))
  {
    /* Since the To file path exists, assume it to be a directory and proceed.      */
    /* We don't care if it's a file.  If it is a file, then config.ini needs to be  */
    /* corrected to remove the file before attempting a MoveFile().                 */
    strcpy(szToTemp, szTo);
    AppendBackSlash(szToTemp, sizeof(szToTemp));
    ParsePath(szFrom, szBuf, sizeof(szBuf), FALSE, PP_FILENAME_ONLY);
    strcat(szToTemp, szBuf);
    DosMove(szFrom, szToTemp);

    /* log the file move command */
    sprintf(szBuf, "%s to %s", szFrom, szToTemp);
    UpdateInstallLog(KEY_MOVE_FILE, szBuf, FALSE);

    return(FO_SUCCESS);
  }

  ParsePath(szFrom, szFromDir, sizeof(szFromDir), FALSE, PP_PATH_ONLY);

  strcat(szFrom, "*.*");
  ulFindCount = 1;
  if((DosFindFirst(szFrom, &hFile, 0, &fdFile, sizeof(fdFile), &ulFindCount, FIL_STANDARD)) != NO_ERROR)
    bFound = FALSE;
  else
    bFound = TRUE;

  while(bFound)
  {
    if((strcmpi(fdFile.achName, ".") != 0) && (strcmpi(fdFile.achName, "..") != 0))
    {
      /* create full path string including filename for source */
      strcpy(szFromTemp, szFromDir);
      AppendBackSlash(szFromTemp, sizeof(szFromTemp));
      strcat(szFromTemp, fdFile.achName);

      /* create full path string including filename for destination */
      strcpy(szToTemp, szTo);
      AppendBackSlash(szToTemp, sizeof(szToTemp));
      strcat(szToTemp, fdFile.achName);

      DosMove(szFromTemp, szToTemp);

      /* log the file move command */
      sprintf(szBuf, "%s to %s", szFromTemp, szToTemp);
      UpdateInstallLog(KEY_MOVE_FILE, szBuf, FALSE);
    }

    ulFindCount = 1;
    if (DosFindNext(hFile, &fdFile, sizeof(fdFile), &ulFindCount) == NO_ERROR) {
      bFound = TRUE;
    } else {
      bFound = FALSE;
    }
  }

  DosFindClose(hFile);
  return(FO_SUCCESS);
}

HRESULT ProcessMoveFile(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szSource[MAX_BUF];
  char  szDestination[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Move File", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szSource, szBuf);
      GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szDestination, szBuf);
      FileMove(szSource, szDestination);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Move File", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT FileCopy(LPSTR szFrom, LPSTR szTo, BOOL bFailIfExists, BOOL bDnu)
{
  HDIR            hFile;
  FILEFINDBUF3    fdFile;
  ULONG           ulFindCount;
  char            szFromDir[MAX_BUF];
  char            szFromTemp[MAX_BUF];
  char            szToTemp[MAX_BUF];
  char            szBuf[MAX_BUF];
  BOOL            bFound;

  if(FileExists(szFrom))
  {
    /* The file in the From file path exists */
    ParsePath(szFrom, szBuf, sizeof(szBuf), FALSE, PP_FILENAME_ONLY);
    strcpy(szToTemp, szTo);
    AppendBackSlash(szToTemp, sizeof(szToTemp));
    strcat(szToTemp, szBuf);
    if (bFailIfExists) {
      DosCopy(szFrom, szToTemp, 0);
    } else {
      DosCopy(szFrom, szToTemp, DCPY_EXISTING);
    }
    sprintf(szBuf, "%s to %s", szFrom, szToTemp);
    UpdateInstallLog(KEY_COPY_FILE, szBuf, bDnu);

    return(FO_SUCCESS);
  }

  /* The file in the From file path does not exist.  Assume to contain wild args and */
  /* proceed acordingly.                                                             */
  ParsePath(szFrom, szFromDir, sizeof(szFromDir), FALSE, PP_PATH_ONLY);

  ulFindCount = 1;
  if((DosFindFirst(szFrom, &hFile, 0, &fdFile, sizeof(fdFile), &ulFindCount, FIL_STANDARD)) != NO_ERROR)
    bFound = FALSE;
  else
    bFound = TRUE;

  while(bFound)
  {
    if((strcmpi(fdFile.achName, ".") != 0) && (strcmpi(fdFile.achName, "..") != 0))
    {
      /* create full path string including filename for source */
      strcpy(szFromTemp, szFromDir);
      AppendBackSlash(szFromTemp, sizeof(szFromTemp));
      strcat(szFromTemp, fdFile.achName);

      /* create full path string including filename for destination */
      strcpy(szToTemp, szTo);
      AppendBackSlash(szToTemp, sizeof(szToTemp));
      strcat(szToTemp, fdFile.achName);

      if (bFailIfExists) {
        DosCopy(szFromTemp, szToTemp, 0);
      } else {
        DosCopy(szFromTemp, szToTemp, DCPY_EXISTING);
      }

      /* log the file copy command */
      sprintf(szBuf, "%s to %s", szFromTemp, szToTemp);
      UpdateInstallLog(KEY_COPY_FILE, szBuf, bDnu);
    }

    ulFindCount = 1;
    if (DosFindNext(hFile, &fdFile, sizeof(fdFile), &ulFindCount) == NO_ERROR) {
      bFound = TRUE;
    } else {
      bFound = FALSE;
    }
  }

  DosFindClose(hFile);
  return(FO_SUCCESS);
}

HRESULT FileCopySequential(LPSTR szSourcePath, LPSTR szDestPath, LPSTR szFilename)
{
  int             iFilenameOnlyLen;
  char            szDestFullFilename[MAX_BUF];
  char            szSourceFullFilename[MAX_BUF];
  char            szSearchFilename[MAX_BUF];
  char            szSearchDestFullFilename[MAX_BUF];
  char            szFilenameOnly[MAX_BUF];
  char            szFilenameExtensionOnly[MAX_BUF];
  char            szNumber[MAX_BUF];
  long            dwNumber;
  long            dwMaxNumber;
  LPSTR           szDotPtr;
  HDIR            hFile;
  FILEFINDBUF3    fdFile;
  ULONG           ulFindCount;
  BOOL            bFound;

  strcpy(szSourceFullFilename, szSourcePath);
  AppendBackSlash(szSourceFullFilename, sizeof(szSourceFullFilename));
  strcat(szSourceFullFilename, szFilename);

  if(FileExists(szSourceFullFilename))
  {
    /* zero out the memory */
    memset(szSearchFilename, 0,        sizeof(szSearchFilename));
    memset(szFilenameOnly, 0,          sizeof(szFilenameOnly));
    memset(szFilenameExtensionOnly, 0, sizeof(szFilenameExtensionOnly));

    /* parse for the filename w/o extention and also only the extension */
    if((szDotPtr = strstr(szFilename, ".")) != NULL)
    {
      *szDotPtr = '\0';
      strcpy(szSearchFilename, szFilename);
      strcpy(szFilenameOnly, szFilename);
      strcpy(szFilenameExtensionOnly, &szDotPtr[1]);
      *szDotPtr = '.';
    }
    else
    {
      strcpy(szFilenameOnly, szFilename);
      strcpy(szSearchFilename, szFilename);
    }

    /* create the wild arg filename to search for in the szDestPath */
    strcat(szSearchFilename, "*.*");
    strcpy(szSearchDestFullFilename, szDestPath);
    AppendBackSlash(szSearchDestFullFilename, sizeof(szSearchDestFullFilename));
    strcat(szSearchDestFullFilename, szSearchFilename);

    iFilenameOnlyLen = strlen(szFilenameOnly);
    dwNumber         = 0;
    dwMaxNumber      = 0;

    /* find the largest numbered filename in the szDestPath */
    ulFindCount = 1;
    if((DosFindFirst(szSearchDestFullFilename, &hFile, 0, &fdFile, sizeof(fdFile), &ulFindCount, FIL_STANDARD)) != NO_ERROR)
      bFound = FALSE;
    else
      bFound = TRUE;

    while(bFound)
    {
      memset(szNumber, 0, sizeof(szNumber));
      if((strcmpi(fdFile.achName, ".") != 0) && (strcmpi(fdFile.achName, "..") != 0))
      {
        strcpy(szNumber, &fdFile.achName[iFilenameOnlyLen]);
        dwNumber = atoi(szNumber);
        if(dwNumber > dwMaxNumber)
          dwMaxNumber = dwNumber;
      }

      ulFindCount = 1;
      if (DosFindNext(hFile, &fdFile, sizeof(fdFile), &ulFindCount) == NO_ERROR) {
        bFound = TRUE;
      } else {
        bFound = FALSE;
      }
    }

    DosFindClose(hFile);

    strcpy(szDestFullFilename, szDestPath);
    AppendBackSlash(szDestFullFilename, sizeof(szDestFullFilename));
    strcat(szDestFullFilename, szFilenameOnly);
    itoa(dwMaxNumber + 1, szNumber, 10);
    strcat(szDestFullFilename, szNumber);

    if(*szFilenameExtensionOnly != '\0')
    {
      strcat(szDestFullFilename, ".");
      strcat(szDestFullFilename, szFilenameExtensionOnly);
    }

    DosCopy(szSourceFullFilename, szDestFullFilename, 0);
  }

  return(FO_SUCCESS);
}

HRESULT ProcessCopyFile(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szSource[MAX_BUF];
  char  szDestination[MAX_BUF];
  BOOL  bFailIfExists;
  BOOL  bDnu;

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Copy File", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szSource, szBuf);
      GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szDestination, szBuf);

      GetPrivateProfileString(szSection, "Do Not Uninstall", "", szBuf, sizeof(szBuf), szFileIniConfig);
      if(strcmpi(szBuf, "TRUE") == 0)
        bDnu = TRUE;
      else
        bDnu = FALSE;

      GetPrivateProfileString(szSection, "Fail If Exists", "", szBuf, sizeof(szBuf), szFileIniConfig);
      if(strcmpi(szBuf, "TRUE") == 0)
        bFailIfExists = TRUE;
      else
        bFailIfExists = FALSE;

      FileCopy(szSource, szDestination, bFailIfExists, bDnu);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Copy File", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessCopyFileSequential(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szSource[MAX_BUF];
  char  szDestination[MAX_BUF];
  char  szFilename[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Copy File Sequential", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Filename", "", szFilename, sizeof(szFilename), szFileIniConfig);
  while(*szFilename != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szSource, szBuf);

      GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szDestination, szBuf);

      FileCopySequential(szSource, szDestination, szFilename);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Copy File Sequential", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Filename", "", szFilename, sizeof(szFilename), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

void UpdateInstallLog(PSZ szKey, PSZ szString, BOOL bDnu)
{
  FILE *fInstallLog;
  char szBuf[MAX_BUF];
  char szFileInstallLog[MAX_BUF];

  if(gbILUseTemp)
  {
    strcpy(szFileInstallLog, szTempDir);
    AppendBackSlash(szFileInstallLog, sizeof(szFileInstallLog));
  }
  else
  {
    strcpy(szFileInstallLog, sgProduct.szPath);
    AppendBackSlash(szFileInstallLog, sizeof(szFileInstallLog));
    strcat(szFileInstallLog, sgProduct.szSubPath);
    AppendBackSlash(szFileInstallLog, sizeof(szFileInstallLog));
  }

  CreateDirectoriesAll(szFileInstallLog, FALSE);
  strcat(szFileInstallLog, FILE_INSTALL_LOG);

  if((fInstallLog = fopen(szFileInstallLog, "a+")) != NULL)
  {
    if(bDnu)
      sprintf(szBuf, "     ** (*dnu*) %s%s\n", szKey, szString);
    else
      sprintf(szBuf, "     ** %s%s\n", szKey, szString);

    fwrite(szBuf, sizeof(char), strlen(szBuf), fInstallLog);
    fclose(fInstallLog);
  }
}

void UpdateInstallStatusLog(PSZ szString)
{
  FILE *fInstallLog;
  char szFileInstallStatusLog[MAX_BUF];

  if(gbILUseTemp)
  {
    strcpy(szFileInstallStatusLog, szTempDir);
    AppendBackSlash(szFileInstallStatusLog, sizeof(szFileInstallStatusLog));
  }
  else
  {
    strcpy(szFileInstallStatusLog, sgProduct.szPath);
    AppendBackSlash(szFileInstallStatusLog, sizeof(szFileInstallStatusLog));
    strcat(szFileInstallStatusLog, sgProduct.szSubPath);
    AppendBackSlash(szFileInstallStatusLog, sizeof(szFileInstallStatusLog));
  }

  CreateDirectoriesAll(szFileInstallStatusLog, FALSE);
  strcat(szFileInstallStatusLog, FILE_INSTALL_STATUS_LOG);

  if((fInstallLog = fopen(szFileInstallStatusLog, "a+")) != NULL)
  {
    fwrite(szString, sizeof(char), strlen(szString), fInstallLog);
    fclose(fInstallLog);
  }
}

void UpdateJSProxyInfo()
{
  FILE *fJSFile;
  char szBuf[MAX_BUF];
  char szJSFile[MAX_BUF];

  if((*diAdvancedSettings.szProxyServer != '\0') || (*diAdvancedSettings.szProxyPort != '\0'))
  {
    strcpy(szJSFile, sgProduct.szPath);
    if(*sgProduct.szSubPath != '\0')
    {
      AppendBackSlash(szJSFile, sizeof(szJSFile));
      strcat(szJSFile, sgProduct.szSubPath);
    }
    AppendBackSlash(szJSFile, sizeof(szJSFile));
    strcat(szJSFile, "defaults\\pref\\");
    CreateDirectoriesAll(szJSFile, TRUE);
    strcat(szJSFile, FILE_ALL_JS);

    if((fJSFile = fopen(szJSFile, "a+")) != NULL)
    {
      memset(szBuf, 0, sizeof(szBuf));
      if(*diAdvancedSettings.szProxyServer != '\0')
      {
        if(diAdditionalOptions.dwUseProtocol == UP_FTP)
          sprintf(szBuf,
                   "pref(\"network.proxy.ftp\", \"%s\");\n",
                   diAdvancedSettings.szProxyServer);
        else
          sprintf(szBuf,
                   "pref(\"network.proxy.http\", \"%s\");\n",
                   diAdvancedSettings.szProxyServer);
      }

      if(*diAdvancedSettings.szProxyPort != '\0')
      {
        if(diAdditionalOptions.dwUseProtocol == UP_FTP)
          sprintf(szBuf,
                   "pref(\"network.proxy.ftp_port\", %s);\n",
                   diAdvancedSettings.szProxyPort);
        else
          sprintf(szBuf,
                   "pref(\"network.proxy.http_port\", %s);\n",
                   diAdvancedSettings.szProxyPort);
      }

      strcat(szBuf, "pref(\"network.proxy.type\", 1);\n");

      fwrite(szBuf, sizeof(char), strlen(szBuf), fJSFile);
      fclose(fJSFile);
    }
  }
}

HRESULT CreateDirectoriesAll(char* szPath, BOOL bLogForUninstall)
{
  int     i;
  int     iLen = strlen(szPath);
  char    szCreatePath[MAX_BUF];
  HRESULT hrResult = 0;

  memset(szCreatePath, 0, MAX_BUF);
  memcpy(szCreatePath, szPath, iLen);
  for(i = 0; i < iLen; i++)
  {
    if((iLen > 1) &&
      ((i != 0) && ((szPath[i] == '\\') || (szPath[i] == '/'))) &&
      (!((szPath[0] == '\\') && (i == 1)) && !((szPath[1] == ':') && (i == 2))))
    {
      szCreatePath[i] = '\0';
      if(FileExists(szCreatePath) == FALSE)
      {
        APIRET rc = DosCreateDir(szCreatePath, NULL);  
        if (rc == NO_ERROR) {
          hrResult = 1;
        }

        if(bLogForUninstall)
          UpdateInstallLog(KEY_CREATE_FOLDER, szCreatePath, FALSE);
      }
      szCreatePath[i] = szPath[i];
    }
  }
  return(hrResult);
}

HRESULT ProcessCreateDirectory(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szDestination[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Create Directory", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szDestination, szBuf);
      AppendBackSlash(szDestination, sizeof(szDestination));
      CreateDirectoriesAll(szDestination, TRUE);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Create Directory", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT FileDelete(LPSTR szDestination)
{
  HDIR            hFile;
  FILEFINDBUF3    fdFile;
  ULONG           ulFindCount;
  char            szBuf[MAX_BUF];
  char            szPathOnly[MAX_BUF];
  BOOL            bFound;

  if(FileExists(szDestination))
  {
    /* The file in the From file path exists */
    DosDelete(szDestination);
    return(FO_SUCCESS);
  }

  /* The file in the From file path does not exist.  Assume to contain wild args and */
  /* proceed acordingly.                                                             */
  ParsePath(szDestination, szPathOnly, sizeof(szPathOnly), FALSE, PP_PATH_ONLY);

  ulFindCount = 1;
  if((DosFindFirst(szDestination, &hFile, 0, &fdFile, sizeof(fdFile), &ulFindCount, FIL_STANDARD)) != NO_ERROR)
    bFound = FALSE;
  else
    bFound = TRUE;

  while(bFound)
  {
    if(!(fdFile.attrFile & FILE_DIRECTORY))
    {
      strcpy(szBuf, szPathOnly);
      AppendBackSlash(szBuf, sizeof(szBuf));
      strcat(szBuf, fdFile.achName);

      DosDelete(szBuf);
    }

    ulFindCount = 1;
    if (DosFindNext(hFile, &fdFile, sizeof(fdFile), &ulFindCount) == NO_ERROR) {
      bFound = TRUE;
    } else {
      bFound = FALSE;
    }
  }

  DosFindClose(hFile);
  return(FO_SUCCESS);
}

HRESULT ProcessDeleteFile(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szDestination[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Delete File", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szDestination, szBuf);
      FileDelete(szDestination);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Delete File", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT DirectoryRemove(LPSTR szDestination, BOOL bRemoveSubdirs)
{
  HDIR            hFile;
  FILEFINDBUF3    fdFile;
  ULONG           ulFindCount;
  char            szDestTemp[MAX_BUF];
  BOOL            bFound;

  if(!FileExists(szDestination))
    return(FO_SUCCESS);

  if(bRemoveSubdirs == TRUE)
  {
    strcpy(szDestTemp, szDestination);
    AppendBackSlash(szDestTemp, sizeof(szDestTemp));
    strcat(szDestTemp, "*");

    ulFindCount = 1;
    if((DosFindFirst(szDestTemp, &hFile, 0, &fdFile, sizeof(fdFile), &ulFindCount, FIL_STANDARD)) != NO_ERROR)
      bFound = FALSE;
    else
      bFound = TRUE;
    while(bFound == TRUE)
    {
      if((strcmpi(fdFile.achName, ".") != 0) && (strcmpi(fdFile.achName, "..") != 0))
      {
        /* create full path */
        strcpy(szDestTemp, szDestination);
        AppendBackSlash(szDestTemp, sizeof(szDestTemp));
        strcat(szDestTemp, fdFile.achName);

        if(fdFile.attrFile & FILE_DIRECTORY)
        {
          DirectoryRemove(szDestTemp, bRemoveSubdirs);
        }
        else
        {
          DosDelete(szDestTemp);
        }
      }

      ulFindCount = 1;
      if (DosFindNext(hFile, &fdFile, sizeof(fdFile), &ulFindCount) == NO_ERROR) {
        bFound = TRUE;
      } else {
        bFound = FALSE;
      }
    }

    DosFindClose(hFile);
  }
  
  DosDeleteDir(szDestination);
  return(FO_SUCCESS);
}

HRESULT ProcessRemoveDirectory(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szDestination[MAX_BUF];
  BOOL  bRemoveSubdirs;

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Remove Directory", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szDestination, szBuf);
      GetPrivateProfileString(szSection, "Remove subdirs", "", szBuf, sizeof(szBuf), szFileIniConfig);
      bRemoveSubdirs = FALSE;
      if(strcmpi(szBuf, "TRUE") == 0)
        bRemoveSubdirs = TRUE;

      DirectoryRemove(szDestination, bRemoveSubdirs);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Remove Directory", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessRunApp(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szTarget[MAX_BUF];
  char  szParameters[MAX_BUF];
  char  szWorkingDir[MAX_BUF];
  BOOL  bRunApp;
  BOOL  bWait;

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "RunApp", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Target", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szTarget, szBuf);
      GetPrivateProfileString(szSection, "Parameters", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szParameters, szBuf);

      // If we are given a criterion to test against, we expect also to be told whether we should run
      //    the app when that criterion is true or when it is false.  If we are not told, we assume that
      //    we are to run the app when the criterion is true.
      bRunApp = TRUE;
      GetPrivateProfileString(szSection, "Criterion ID", "", szBuf, sizeof(szBuf), szFileIniConfig);
      if(strcmpi(szBuf, "RecaptureHP") == 0)
      {
        GetPrivateProfileString(szSection, "Run App If Criterion", "", szBuf, sizeof(szBuf), szFileIniConfig);
        if(strcmpi(szBuf, "FALSE") == 0)
        {
          if(diAdditionalOptions.bRecaptureHomepage == TRUE)
             bRunApp = FALSE;
        }
        else
        {
          if(diAdditionalOptions.bRecaptureHomepage == FALSE)
             bRunApp = FALSE;
        }
      }

      GetPrivateProfileString(szSection, "WorkingDir", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szWorkingDir, szBuf);

      GetPrivateProfileString(szSection, "Wait", "", szBuf, sizeof(szBuf), szFileIniConfig);
      if(strcmpi(szBuf, "FALSE") == 0)
        bWait = FALSE;
      else
        bWait = TRUE;

      if ((bRunApp == TRUE) && FileExists(szTarget))
      {
        if((dwTiming == T_DEPEND_REBOOT) && (NeedReboot() == TRUE))
        {
          strcat(szTarget, " ");
          strcat(szTarget, szParameters);
#ifdef OLDCODE /* @MAK */
          /* Add an object to the startup folder that points to szTarget and uses szParameters */
          /* and perhaps an xtra string that only we know (like deleteinstall)
          /* Then add code to nsNativeAppSupportOS2.cpp that when it sees this string, it */
          /* deletes the object from the startup folder */
          /* This is used to allow the app to run automatically on reboot, but clean up */
          /* after itself */
#endif
        }
        else
        {
          GetPrivateProfileString(szSection, "Message", "", szBuf, sizeof(szBuf), szFileIniConfig);
          if ( szBuf[0] != '\0' )
            ShowMessage(szBuf, TRUE);  
#ifdef OLDCODE /* @MAK */
          WinSpawn(szTarget, szParameters, szWorkingDir, SW_SHOWNORMAL, bWait);
#endif
          if ( szBuf[0] != '\0' )
            ShowMessage(szBuf, FALSE);  
        }
      }
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "RunApp", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Target", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessOS2INI(ULONG ulTiming, char *szSectionPrefix)
{
  char    szBuf[MAX_BUF];
  char    szApp[MAX_BUF];
  char    szKey[MAX_BUF];
  char    szValue[MAX_BUF];
  char    szDecrypt[MAX_BUF];
  char    szSection[MAX_BUF];
  BOOL    bDnu;
  ULONG   ulIndex;
  ULONG   ulSize;

  ulIndex = 0;
  BuildNumberedString(ulIndex, szSectionPrefix, "OS2 INI", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "App", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(ulTiming, szSection, szFileIniConfig))
    {
      GetPrivateProfileString(szSection, "App",                 "", szBuf,           sizeof(szBuf),          szFileIniConfig);
      GetPrivateProfileString(szSection, "Decrypt App",         "", szDecrypt,       sizeof(szDecrypt),      szFileIniConfig);
      memset(szApp, 0, sizeof(szApp));
      if(strcmpi(szDecrypt, "TRUE") == 0)
        DecryptString(szApp, szBuf);
      else
        strcpy(szApp, szBuf);

      GetPrivateProfileString(szSection, "Key",                "", szBuf,           sizeof(szBuf),           szFileIniConfig);
      GetPrivateProfileString(szSection, "Decrypt Key",        "", szDecrypt,       sizeof(szDecrypt),       szFileIniConfig);
      memset(szKey, 0, sizeof(szKey));
      if(strcmpi(szDecrypt, "TRUE") == 0)
        DecryptString(szKey, szBuf);
      else
        strcpy(szKey, szBuf);

      GetPrivateProfileString(szSection, "Key Value",          "", szBuf,           sizeof(szBuf), szFileIniConfig);
      GetPrivateProfileString(szSection, "Decrypt Key Value",  "", szDecrypt,       sizeof(szDecrypt), szFileIniConfig);
      memset(szValue, 0, sizeof(szValue));
      if(strcmpi(szDecrypt, "TRUE") == 0)
        DecryptString(szValue, szBuf);
      else
        strcpy(szValue, szBuf);

      GetPrivateProfileString(szSection, "Size",                "", szBuf,           sizeof(szBuf), szFileIniConfig);
      if(*szBuf != '\0')
        ulSize = atoi(szBuf);
      else
        ulSize = 0;

      GetPrivateProfileString(szSection,
                              "Do Not Uninstall",
                              "",
                              szBuf,
                              sizeof(szBuf),
                              szFileIniConfig);
      if(strcmpi(szBuf, "TRUE") == 0)
        bDnu = TRUE;
      else
        bDnu = FALSE;

      PrfWriteProfileString(HINI_USERPROFILE, szApp, szKey, szValue);
    }

    ++ulIndex;
    BuildNumberedString(ulIndex, szSectionPrefix, "OS2 INI", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "App", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessProgramFolder(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex0;
  DWORD dwIndex1;
  char  szIndex1[MAX_BUF];
  char  szBuf[MAX_BUF];
  char  szBuf2[MAX_BUF];
  char  szSection0[MAX_BUF];
  char  szSection1[MAX_BUF];
  char  szProgramFolder[MAX_BUF];

  char  szClassName[MAX_BUF];
  char  szTitle[MAX_BUF];
  char  szSetupString[MAX_BUF];
  char  szLocation[MAX_BUF];
  char  szAssocFilters[MAX_BUF];
  char  szAssocTypes[MAX_BUF];


  dwIndex0 = 0;
  BuildNumberedString(dwIndex0, szSectionPrefix, "Program Folder", szSection0, sizeof(szSection0));
  GetPrivateProfileString(szSection0, "Program Folder", "", szBuf, sizeof(szBuf), szFileIniConfig);
  if(TimingCheck(dwTiming, szSection0, szFileIniConfig))
  {
    /* Create program folder */
    WinCreateObject("WPFolder", sgProduct.szProgramFolderName, "",
                    sgProduct.szProgramFolderPath, CO_REPLACEIFEXISTS);
  }
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection0, szFileIniConfig))
    {
      DecryptString(szProgramFolder, szBuf);

      dwIndex1 = 0;
      itoa(dwIndex1, szIndex1, 10);
      strcpy(szSection1, szSection0);
      strcat(szSection1, "-Object");
      strcat(szSection1, szIndex1);
      GetPrivateProfileString(szSection1, "ClassName", "", szClassName, sizeof(szClassName), szFileIniConfig);
      while(*szBuf != '\0')
      {
        *szSetupString = '\0';
        GetPrivateProfileString(szSection1, "Title",    "", szBuf, sizeof(szBuf), szFileIniConfig);
        DecryptString(szTitle, szBuf);
        GetPrivateProfileString(szSection1, "Location",  "", szBuf, sizeof(szBuf), szFileIniConfig);
        DecryptString(szLocation, szBuf);
        GetPrivateProfileString(szSection1, "File",  "", szBuf, sizeof(szBuf), szFileIniConfig);
        DecryptString(szBuf2, szBuf);
        if (szBuf2[0]) {
          if (szSetupString[0]) {
            strcat(szSetupString, ";");
          }
          strcat(szSetupString, "EXENAME=");
          strcat(szSetupString, szBuf2);
        }
        GetPrivateProfileString(szSection1, "Parameters",  "", szBuf, sizeof(szBuf), szFileIniConfig);
        DecryptString(szBuf2, szBuf);
        if (szBuf2[0]) {
          if (szSetupString[0]) {
            strcat(szSetupString, ";");
          }
          strcat(szSetupString, "PARAMETERS=");
          strcat(szSetupString, szBuf2);
        }
        GetPrivateProfileString(szSection1, "Working Dir",  "", szBuf, sizeof(szBuf), szFileIniConfig);
        DecryptString(szBuf2, szBuf);
        if (szBuf2[0]) {
          if (szSetupString[0]) {
            strcat(szSetupString, ";");
          }
          strcat(szSetupString, "STARTUPDIR=");
          strcat(szSetupString, szBuf2);
        }
        GetPrivateProfileString(szSection1, "Object ID",  "", szBuf, sizeof(szBuf), szFileIniConfig);
        if (szBuf[0]) {
          if (szSetupString[0]) {
            strcat(szSetupString, ";");
          }
          strcat(szSetupString, "OBJECTID=");
          strcat(szSetupString, szBuf);
        }
        GetPrivateProfileString(szSection1, "Association Filters",  "", szBuf, sizeof(szBuf), szFileIniConfig);
        if (szBuf[0]) {
          if (diOS2Integration.oiCBAssociateHTML.bCheckBoxState == TRUE) {
            if (szSetupString[0]) {
              strcat(szSetupString, ";");
            }
            strcat(szSetupString, "ASSOCFILTER=");
            strcat(szSetupString, szBuf);
          }
        }
        GetPrivateProfileString(szSection1, "Association Types",  "", szBuf, sizeof(szBuf), szFileIniConfig);
        if (szBuf[0]) {
          if (diOS2Integration.oiCBAssociateHTML.bCheckBoxState == TRUE) {
            if (szSetupString[0]) {
              strcat(szSetupString, ";");
            }
            strcat(szSetupString, "ASSOCTYPE=");
            strcat(szSetupString, szBuf);
          }
        }
        GetPrivateProfileString(szSection1, "Setup String",  "", szBuf, sizeof(szBuf), szFileIniConfig);
        DecryptString(szBuf2, szBuf);
        if (szBuf2[0]) {
          if (szSetupString[0]) {
            strcat(szSetupString, ";");
          }
          strcat(szSetupString, szBuf2);
        }

        WinCreateObject(szClassName, szTitle, szSetupString, szLocation, CO_REPLACEIFEXISTS);

        if (diOS2Integration.oiCBMakeDefaultBrowser.bCheckBoxState == TRUE) {

        }
        
#ifdef OLDCODE
        strcpy(szBuf, szProgramFolder);
        AppendBackSlash(szBuf, sizeof(szBuf));
        strcat(szBuf, szDescription);
        UpdateInstallLog(KEY_WINDOWS_SHORTCUT, szBuf, FALSE);
#endif

        ++dwIndex1;
        itoa(dwIndex1, szIndex1, 10);
        strcpy(szSection1, szSection0);
        strcat(szSection1, "-Object");
        strcat(szSection1, szIndex1);
        GetPrivateProfileString(szSection1, "ClassName", "", szBuf, sizeof(szBuf), szFileIniConfig);
      }
    }

    ++dwIndex0;
    BuildNumberedString(dwIndex0, szSectionPrefix, "Program Folder", szSection0, sizeof(szSection0));
    GetPrivateProfileString(szSection0, "Program Folder", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessProgramFolderShowCmd()
{
  DWORD dwIndex0;
  int   iShowFolder;
  char  szBuf[MAX_BUF];
  char  szSection0[MAX_BUF];
  char  szProgramFolder[MAX_BUF];

  dwIndex0 = 0;
  BuildNumberedString(dwIndex0, NULL, "Program Folder", szSection0, sizeof(szSection0));
  GetPrivateProfileString(szSection0, "Program Folder", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    DecryptString(szProgramFolder, szBuf);
    GetPrivateProfileString(szSection0, "Show Folder", "", szBuf, sizeof(szBuf), szFileIniConfig);

#ifdef OLDCODE
    if(strcmpi(szBuf, "HIDE") == 0)
      iShowFolder = SW_HIDE;
    else if(strcmpi(szBuf, "MAXIMIZE") == 0)
      iShowFolder = SW_MAXIMIZE;
    else if(strcmpi(szBuf, "MINIMIZE") == 0)
      iShowFolder = SW_MINIMIZE;
    else if(strcmpi(szBuf, "RESTORE") == 0)
      iShowFolder = SW_RESTORE;
    else if(strcmpi(szBuf, "SHOW") == 0)
      iShowFolder = SW_SHOW;
    else if(strcmpi(szBuf, "SHOWMAXIMIZED") == 0)
      iShowFolder = SW_SHOWMAXIMIZED;
    else if(strcmpi(szBuf, "SHOWMINIMIZED") == 0)
      iShowFolder = SW_SHOWMINIMIZED;
    else if(strcmpi(szBuf, "SHOWMINNOACTIVE") == 0)
      iShowFolder = SW_SHOWMINNOACTIVE;
    else if(strcmpi(szBuf, "SHOWNA") == 0)
      iShowFolder = SW_SHOWNA;
    else if(strcmpi(szBuf, "SHOWNOACTIVATE") == 0)
      iShowFolder = SW_SHOWNOACTIVATE;
    else if(strcmpi(szBuf, "SHOWNORMAL") == 0)
      iShowFolder = SW_SHOWNORMAL;

    if(iShowFolder != SW_HIDE)
      if(sgProduct.dwMode != SILENT)
#ifdef OLDCODE
        WinSpawn(szProgramFolder, NULL, NULL, iShowFolder, TRUE);
#endif

#endif
    ++dwIndex0;
    BuildNumberedString(dwIndex0, NULL, "Program Folder", szSection0, sizeof(szSection0));
    GetPrivateProfileString(szSection0, "Program Folder", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessCreateCustomFiles(DWORD dwTiming)
{
  DWORD dwCompIndex;
  DWORD dwFileIndex;
  DWORD dwSectIndex;
  DWORD dwKVIndex;
  siC   *siCObject = NULL;
  char  szBufTiny[MAX_BUF_TINY];
  char  szSection[MAX_BUF_TINY];
  char  szBuf[MAX_BUF];
  char  szFileName[MAX_BUF];
  char  szDefinedSection[MAX_BUF]; 
  char  szDefinedKey[MAX_BUF]; 
  char  szDefinedValue[MAX_BUF];

  dwCompIndex   = 0;
  siCObject = SiCNodeGetObject(dwCompIndex, TRUE, AC_ALL);

  while(siCObject)
  {
    dwFileIndex   = 0;
    sprintf(szSection,"%s-Configuration File%d",siCObject->szReferenceName,dwFileIndex);
    siCObject = SiCNodeGetObject(++dwCompIndex, TRUE, AC_ALL);
    if(TimingCheck(dwTiming, szSection, szFileIniConfig) == FALSE)
    {
      continue;
    }

    GetPrivateProfileString(szSection, "FileName", "", szBuf, sizeof(szBuf), szFileIniConfig);
    while (*szBuf != '\0')
    {
      DecryptString(szFileName, szBuf);
      if(FileExists(szFileName))
      {
        DosDelete(szFileName);
      }

      /* TO DO - Support a File Type for something other than .ini */
      dwSectIndex = 0;
      sprintf(szBufTiny, "Section%d",dwSectIndex);
      GetPrivateProfileString(szSection, szBufTiny, "", szDefinedSection, sizeof(szDefinedSection), szFileIniConfig);
      while(*szDefinedSection != '\0')
      {  
        dwKVIndex =0;
        sprintf(szBufTiny,"Section%d-Key%d",dwSectIndex,dwKVIndex);
        GetPrivateProfileString(szSection, szBufTiny, "", szDefinedKey, sizeof(szDefinedKey), szFileIniConfig);
        while(*szDefinedKey != '\0')
        {
          sprintf(szBufTiny,"Section%d-Value%d",dwSectIndex,dwKVIndex);
          GetPrivateProfileString(szSection, szBufTiny, "", szBuf, sizeof(szBuf), szFileIniConfig);
          DecryptString(szDefinedValue, szBuf);
          if(WritePrivateProfileString(szDefinedSection, szDefinedKey, szDefinedValue, szFileName) == 0)
          {
            char szEWPPS[MAX_BUF];
            char szBuf[MAX_BUF];
            char szBuf2[MAX_BUF];
            if(GetPrivateProfileString("Messages", "ERROR_WRITEPRIVATEPROFILESTRING", "", szEWPPS, sizeof(szEWPPS), szFileIniInstall))
            {
              sprintf(szBuf, "%s\n    [%s]\n    %s=%s", szFileName, szDefinedSection, szDefinedKey, szDefinedValue);
              sprintf(szBuf2, szEWPPS, szBuf);
              PrintError(szBuf2, ERROR_CODE_SHOW);
            }
            return(FO_ERROR_WRITE);
          }
          sprintf(szBufTiny,"Section%d-Key%d",dwSectIndex,++dwKVIndex);
          GetPrivateProfileString(szSection, szBufTiny, "", szDefinedKey, sizeof(szDefinedKey), szFileIniConfig);
        } /* while(*szDefinedKey != '\0')  */

        sprintf(szBufTiny, "Section%d",++dwSectIndex);
        GetPrivateProfileString(szSection, szBufTiny, "", szDefinedSection, sizeof(szDefinedSection), szFileIniConfig);
      } /*       while(*szDefinedSection != '\0') */

      sprintf(szSection,"%s-Configuration File%d",siCObject->szReferenceName,++dwFileIndex);
      GetPrivateProfileString(szSection, "FileName", "", szBuf, sizeof(szBuf), szFileIniConfig);
    } /* while(*szBuf != '\0') */
  } /* while(siCObject) */
  return (FO_SUCCESS);
}

