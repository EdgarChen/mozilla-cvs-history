/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsInstallDelete_h__
#define nsInstallDelete_h__

#include "prtypes.h"
#include "nsSoftwareUpdate.h"
#include "nsFolderSpec.h"

PR_BEGIN_EXTERN_C

struct nsInstallDelete  : public nsInstallObject {

public:

  /* Public Fields */

  PRInt32 FILE_DOES_NOT_EXIST;  // File cannot be deleted as it does not exist.
  PRInt32 FILE_READ_ONLY;	// File cannot be deleted as it is read only.
  PRInt32 FILE_IS_DIRECTORY;	// File cannot be deleted as it is a directory.
  PRInt32 deleteStatus;

  /* Public Methods */

  /*	Constructor
   *    inFolder	- a folder object representing the directory that contains the file
   *    inRelativeFileName  - a relative path and file name
   */
  nsInstallDelete(nsSoftwareUpdate* inSoftUpdate, nsFolderSpec* inFolder, char* inRelativeFileName, char* *errorMsg);

  /*	Constructor
   *    inRegistryName	- name of the component in the registry
   */
  nsInstallDelete(nsSoftwareUpdate* inSoftUpdate, char* inRegistryName, char* *errorMsg);

  virtual ~nsInstallDelete();

  char* Prepare();
  char* Complete();
  void Abort();
  char* toString();

  /* should these be protected? */
  PRBool CanUninstall();
  PRBool RegisterPackageNode();

private:

  /* Private Fields */
  char* finalFile;	// final file to be deleted
  char* registryName;		// final file to be deleted

  /* Private Methods */
  void processInstallDelete(char* *errorMsg);
  int NativeComplete();
  int NativeCheckFileStatus();

};

PR_END_EXTERN_C

#endif /* nsInstallDelete_h__ */
