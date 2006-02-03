/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
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
 * The Original Code is the Metrics extension.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
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

#include "nsMetricsModule.h"
#include "nsMetricsService.h"
#include "nsMetricsEvent.h"
#include "nsIGenericFactory.h"
#include "nsICategoryManager.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsXPCOMCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMetricsService, Init)
NS_DECL_CLASSINFO(nsMetricsService)

static NS_METHOD
nsMetricsServiceRegisterSelf(nsIComponentManager *compMgr,
                             nsIFile *path,
                             const char *loaderStr,
                             const char *type,
                             const nsModuleComponentInfo *info)
{
  nsCOMPtr<nsICategoryManager> cat =
      do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  NS_ENSURE_STATE(cat);

  cat->AddCategoryEntry("app-startup",
                        NS_METRICSSERVICE_CLASSNAME,
                        NS_METRICSSERVICE_CONTRACTID,
                        PR_TRUE, PR_TRUE, nsnull);
  return NS_OK;
}

static const nsModuleComponentInfo components[] = {
  {
    NS_METRICSSERVICE_CLASSNAME,
    NS_METRICSSERVICE_CID,
    NS_METRICSSERVICE_CONTRACTID,
    nsMetricsServiceConstructor,
    nsMetricsServiceRegisterSelf,
    NULL,
    NULL,
    NS_CI_INTERFACE_GETTER_NAME(nsMetricsService),
    NULL,
    &NS_CLASSINFO_NAME(nsMetricsService),
    nsIClassInfo::MAIN_THREAD_ONLY | nsIClassInfo::SINGLETON
  }
};

NS_IMPL_NSGETMODULE(metrics, components)
