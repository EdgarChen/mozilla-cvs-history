#ifndef __NS_INSTALLTRIGGER_H__
#define __NS_INSTALLTRIGGER_H__

#include "nscore.h"
#include "nsString.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIScriptObjectOwner.h"

#include "prtypes.h"
#include "nsHashtable.h"

#include "nsIDOMInstallTriggerGlobal.h"
#include "nsSoftwareUpdate.h"
#include "nsXPITriggerInfo.h"

#include "nsIContentHandler.h"

#define CHROME_SKIN         1
#define CHROME_LOCALE       2
#define CHROME_SAFEMAX      CHROME_LOCALE
#define CHROME_CONTENT      4
#define CHROME_ALL          7
#define CHROME_PROFILE      8
#define CHROME_DELAYED      0x10
#define CHROME_SELECT       0x20

class nsInstallTrigger: public nsIScriptObjectOwner, 
                        public nsIDOMInstallTriggerGlobal,
                        public nsIContentHandler
{
    public:
        static const nsIID& IID() { static nsIID iid = NS_SoftwareUpdateInstallTrigger_CID; return iid; }

        nsInstallTrigger();
        virtual ~nsInstallTrigger();
        
        NS_DECL_ISUPPORTS
        NS_DECL_NSICONTENTHANDLER

        NS_IMETHOD    GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
        NS_IMETHOD    SetScriptObject(void* aScriptObject);

        NS_IMETHOD    UpdateEnabled(PRBool* aReturn);
        NS_IMETHOD    Install(nsIScriptGlobalObject* aGlobalObject, nsXPITriggerInfo *aInfo, PRBool* aReturn);
        NS_IMETHOD    InstallChrome(nsIScriptGlobalObject* aGlobalObject, PRUint32 aType, nsXPITriggerItem* aItem, PRBool* aReturn);
        NS_IMETHOD    StartSoftwareUpdate(nsIScriptGlobalObject* aGlobalObject, const nsString& aURL, PRInt32 aFlags, PRInt32* aReturn);
        NS_IMETHOD    CompareVersion(const nsString& aRegName, PRInt32 aMajor, PRInt32 aMinor, PRInt32 aRelease, PRInt32 aBuild, PRInt32* aReturn);
        NS_IMETHOD    CompareVersion(const nsString& aRegName, const nsString& aVersion, PRInt32* aReturn);
        NS_IMETHOD    CompareVersion(const nsString& aRegName, nsIDOMInstallVersion* aVersion, PRInt32* aReturn);
        NS_IMETHOD    GetVersion(const nsString& component, nsString& version);

        
    private:
        void *mScriptObject;
};

#define NS_INSTALLTRIGGERCOMPONENT_CONTRACTID NS_IAPPSHELLCOMPONENT_CONTRACTID "/xpinstall/installtrigger;1"
#endif
