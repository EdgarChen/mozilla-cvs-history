/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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

#include "nsAbCardProperty.h"	 
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsXPIDLString.h"
#include "nsAbBaseCID.h"
#include "prmem.h"	 
#include "prlog.h"	 
#include "prprf.h"	 
#include "rdf.h"
#include "nsCOMPtr.h"

#include "nsICollation.h"
#include "nsILocale.h"
#include "nsLocaleCID.h"
#include "nsILocaleService.h"
#include "nsCollationCID.h"

#include "nsAddrDatabase.h"
#include "nsIAddrBookSession.h"

static NS_DEFINE_CID(kAddressBookDBCID, NS_ADDRESSBOOKDB_CID);
static NS_DEFINE_CID(kAddrBookSessionCID, NS_ADDRBOOKSESSION_CID);
static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID); 
static NS_DEFINE_CID(kCollationFactoryCID, NS_COLLATIONFACTORY_CID);
static NS_DEFINE_IID(kICollationFactoryIID, NS_ICOLLATIONFACTORY_IID);


/* The definition is nsAddressBook.cpp */
extern const char *kDirectoryDataSourceRoot;
extern const char *kCardDataSourceRoot;

/* The definition is nsAddrDatabase.cpp */
extern const char *kMainPersonalAddressBook;
extern const char *kFirstNameColumn;
extern const char *kLastNameColumn;
extern const char *kDisplayNameColumn;
extern const char *kNicknameColumn;
extern const char *kPriEmailColumn;
extern const char *k2ndEmailColumn;
extern const char *kPlainTextColumn;
extern const char *kWorkPhoneColumn;
extern const char *kHomePhoneColumn;
extern const char *kFaxColumn;
extern const char *kPagerColumn;
extern const char *kCellularColumn;
extern const char *kHomeAddressColumn;
extern const char *kHomeAddress2Column;
extern const char *kHomeCityColumn;
extern const char *kHomeStateColumn;
extern const char *kHomeZipCodeColumn;
extern const char *kHomeCountryColumn;
extern const char *kWorkAddressColumn;
extern const char *kWorkAddress2Column;
extern const char *kWorkCityColumn;
extern const char *kWorkStateColumn;
extern const char *kWorkZipCodeColumn;
extern const char *kWorkCountryColumn;
extern const char *kJobTitleColumn;
extern const char *kDepartmentColumn;
extern const char *kCompanyColumn;
extern const char *kWebPage1Column;
extern const char *kWebPage2Column;
extern const char *kBirthYearColumn;
extern const char *kBirthMonthColumn;
extern const char *kBirthDayColumn;
extern const char *kCustom1Column;
extern const char *kCustom2Column;
extern const char *kCustom3Column;
extern const char *kCustom4Column;
extern const char *kNotesColumn;
extern const char *kLastModifiedDateColumn;
/* end */

nsAbCardProperty::nsAbCardProperty(void)
{
	NS_INIT_REFCNT();

	m_FirstName = "";
	m_LastName = "";
	m_DisplayName = "";
	m_NickName = "";
	m_PrimaryEmail = "";
	m_SecondEmail = "";
	m_WorkPhone = "";
	m_HomePhone = "";
	m_FaxNumber = "";
	m_PagerNumber = "";
	m_CellularNumber = "";

	m_HomeAddress = "";
	m_HomeAddress2 = "";
	m_HomeCity = "";
	m_HomeState = "";
	m_HomeZipCode = "";
	m_HomeCountry = "";
	m_WorkAddress = "";
	m_WorkAddress2 = "";
	m_WorkCity = "";
	m_WorkState = "";
	m_WorkZipCode = "";
	m_WorkCountry = "";
	m_JobTitle = "";
	m_Department = "";
	m_Company = "";
	m_WebPage1 = "";
	m_WebPage2 = "";
	m_BirthYear = "";
	m_BirthMonth = "";
	m_BirthDay = "";
	m_Custom1 = "";
	m_Custom2 = "";
	m_Custom3 = "";
	m_Custom4 = "";
	m_Note = "";
	m_LastModDate = 0;

	m_bSendPlainText = PR_FALSE;

	m_dbTableID = 0;
	m_dbRowID = 0;

	m_pAnonymousStrAttributes = nsnull;
	m_pAnonymousStrValues = nsnull;
	m_pAnonymousIntAttributes = nsnull;
	m_pAnonymousIntValues = nsnull;
	m_pAnonymousBoolAttributes = nsnull;
	m_pAnonymousBoolValues = nsnull;
}

nsAbCardProperty::~nsAbCardProperty(void)
{
	if (m_pAnonymousStrAttributes)
		RemoveAnonymousList(m_pAnonymousStrAttributes);
	if (m_pAnonymousIntAttributes)
		RemoveAnonymousList(m_pAnonymousIntAttributes);
	if (m_pAnonymousBoolAttributes)
		RemoveAnonymousList(m_pAnonymousBoolAttributes);

	if (m_pAnonymousStrValues)
		RemoveAnonymousList(m_pAnonymousStrValues);
	if (m_pAnonymousIntValues)
		RemoveAnonymousList(m_pAnonymousIntValues);
	if (m_pAnonymousBoolValues)
		RemoveAnonymousList(m_pAnonymousBoolValues);

	if (mCardDatabase)
	{
		mCardDatabase->Close(PR_TRUE);
		mCardDatabase = null_nsCOMPtr();
	}
}

nsresult nsAbCardProperty::RemoveAnonymousList(nsVoidArray* pArray)
{
	if (pArray)
	{
		PRUint32 count = pArray->Count();
		for (int i = count - 1; i >= 0; i--)
		{
			void* pPtr = pArray->ElementAt(i);
			PR_FREEIF(pPtr);
			pArray->RemoveElementAt(i);
		}
		delete pArray;
	}
	return NS_OK;
}

NS_IMPL_ADDREF(nsAbCardProperty)
NS_IMPL_RELEASE(nsAbCardProperty)

NS_IMETHODIMP nsAbCardProperty::QueryInterface(REFNSIID aIID, void** aResult)
{   
    if (aResult == NULL)  
        return NS_ERROR_NULL_POINTER;  

    if (aIID.Equals(nsCOMTypeInfo<nsIAbCard>::GetIID()) ||
        aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIAbCard*, this);   
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}   

////////////////////////////////////////////////////////////////////////////////

nsresult nsAbCardProperty::GetAttributeName(PRUnichar **aName, nsString& value)
{
	if (aName)
	{
		*aName = value.ToNewUnicode();
		if (!(*aName)) 
			return NS_ERROR_OUT_OF_MEMORY;
		else
			return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;

}

nsresult nsAbCardProperty::SetAttributeName(PRUnichar *aName, nsString& arrtibute)
{
	if (aName)
		arrtibute = aName;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::GetSendPlainText(PRBool *aSendPlainText)
{
	*aSendPlainText = m_bSendPlainText;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::SetSendPlainText(PRBool aSendPlainText)
{
	m_bSendPlainText = aSendPlainText;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::GetDbTableID(PRUint32 *aDbTableID)
{
	*aDbTableID = m_dbTableID;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::SetDbTableID(PRUint32 aDbTableID)
{
	m_dbTableID = aDbTableID;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::GetDbRowID(PRUint32 *aDbRowID)
{
	*aDbRowID = m_dbRowID;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::SetDbRowID(PRUint32 aDbRowID)
{
	m_dbRowID = aDbRowID;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::GetCardValue(const char *attrname, PRUnichar **value)
{
    if (!PL_strcmp(attrname, kFirstNameColumn))
		GetFirstName(value);
    else if (!PL_strcmp(attrname, kLastNameColumn))
		GetLastName(value);
    else if (!PL_strcmp(attrname, kDisplayNameColumn))
		GetDisplayName(value);
    else if (!PL_strcmp(attrname, kNicknameColumn))
		GetNickName(value);
    else if (!PL_strcmp(attrname, kPriEmailColumn))
		GetPrimaryEmail(value);
    else if (!PL_strcmp(attrname, k2ndEmailColumn))
		GetSecondEmail(value);
    else if (!PL_strcmp(attrname, kWorkPhoneColumn))
		GetWorkPhone(value);
    else if (!PL_strcmp(attrname, kHomePhoneColumn))
		GetHomePhone(value);
    else if (!PL_strcmp(attrname, kFaxColumn))
		GetFaxNumber(value);
    else if (!PL_strcmp(attrname, kPagerColumn))
		GetPagerNumber(value);
    else if (!PL_strcmp(attrname, kCellularColumn))
		GetCellularNumber(value);
    else if (!PL_strcmp(attrname, kHomeAddressColumn))
		GetHomeAddress(value);
    else if (!PL_strcmp(attrname, kHomeAddress2Column))
		GetHomeAddress2(value);
    else if (!PL_strcmp(attrname, kHomeCityColumn))
		GetHomeCity(value);
    else if (!PL_strcmp(attrname, kHomeStateColumn))
		GetHomeState(value);
    else if (!PL_strcmp(attrname, kHomeZipCodeColumn))
		GetHomeZipCode(value);
    else if (!PL_strcmp(attrname, kHomeCountryColumn))
		GetHomeCountry(value);
    else if (!PL_strcmp(attrname, kWorkAddressColumn))
		GetWorkAddress(value);
    else if (!PL_strcmp(attrname, kWorkAddress2Column))
		GetWorkAddress2(value);
    else if (!PL_strcmp(attrname, kWorkCityColumn))
		GetWorkCity(value);
    else if (!PL_strcmp(attrname, kWorkStateColumn))
		GetWorkState(value);
    else if (!PL_strcmp(attrname, kWorkZipCodeColumn))
		GetWorkZipCode(value);
    else if (!PL_strcmp(attrname, kWorkCountryColumn))
		GetWorkCountry(value);
    else if (!PL_strcmp(attrname, kWebPage1Column))
		GetWebPage1(value);
    else if (!PL_strcmp(attrname, kWebPage2Column))
		GetWebPage2(value);
    else if (!PL_strcmp(attrname, kBirthYearColumn))
		GetBirthYear(value);
    else if (!PL_strcmp(attrname, kBirthMonthColumn))
		GetBirthMonth(value);
    else if (!PL_strcmp(attrname, kBirthDayColumn))
		GetBirthDay(value);
    else if (!PL_strcmp(attrname, kCustom1Column))
		GetCustom1(value);
    else if (!PL_strcmp(attrname, kCustom2Column))
		GetCustom2(value);
    else if (!PL_strcmp(attrname, kCustom3Column))
		GetCustom3(value);
    else if (!PL_strcmp(attrname, kCustom4Column))
		GetCustom4(value);
    else if (!PL_strcmp(attrname, kNotesColumn))
		GetNotes(value);
	/* else handle pass down attribute */

	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::SetCardValue(const char *attrname, const PRUnichar *value)
{
	if (!attrname && !value)
		return NS_ERROR_NULL_POINTER;

	nsresult rv = NS_OK;

    if (!PL_strcmp(attrname, kFirstNameColumn))
		rv = SetFirstName((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kLastNameColumn))
		rv = SetLastName((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kDisplayNameColumn))
		rv = SetDisplayName((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kNicknameColumn))
		rv = SetNickName((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kPriEmailColumn))
		rv = SetPrimaryEmail((PRUnichar *)value);
    else if (!PL_strcmp(attrname, k2ndEmailColumn))
		rv = SetSecondEmail((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kWorkPhoneColumn))
		rv = SetWorkPhone((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kHomePhoneColumn))
		rv = SetHomePhone((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kFaxColumn))
		rv = SetFaxNumber((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kPagerColumn))
		rv = SetPagerNumber((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kCellularColumn))
		rv = SetCellularNumber((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kHomeAddressColumn))
		rv = SetHomeAddress((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kHomeAddress2Column))
		rv = SetHomeAddress2((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kHomeCityColumn))
		rv = SetHomeCity((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kHomeStateColumn))
		rv = SetHomeState((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kHomeZipCodeColumn))
		rv = SetHomeZipCode((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kHomeCountryColumn))
		rv = SetHomeCountry((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kWorkAddressColumn))
		rv = SetWorkAddress((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kWorkAddress2Column))
		rv = SetWorkAddress2((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kWorkCityColumn))
		rv = SetWorkCity((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kWorkStateColumn))
		rv = SetWorkState((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kWorkZipCodeColumn))
		rv = SetWorkZipCode((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kWorkCountryColumn))
		rv = SetWorkCountry((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kWebPage1Column))
		rv = SetWebPage1((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kWebPage2Column))
		rv = SetWebPage2((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kBirthYearColumn))
		rv = SetBirthYear((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kBirthMonthColumn))
		rv = SetBirthMonth((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kBirthDayColumn))
		rv = SetBirthDay((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kCustom1Column))
		rv = SetCustom1((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kCustom2Column))
		rv = SetCustom2((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kCustom3Column))
		rv = SetCustom3((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kCustom4Column))
		rv = SetCustom4((PRUnichar *)value);
    else if (!PL_strcmp(attrname, kNotesColumn))
		rv = SetNotes((PRUnichar *)value);
	else
	{
		nsAutoString cardValue(value);
		char* valueStr = cardValue.ToNewCString();
		rv = SetAnonymousStringAttribute(attrname, valueStr);
		delete[] valueStr;
	}
	return rv;
}

NS_IMETHODIMP nsAbCardProperty::SetAbDatabase(nsIAddrDatabase* database)
{
	mCardDatabase = database;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::GetAnonymousStrAttrubutesList(nsVoidArray **attrlist)
{
	if (attrlist && m_pAnonymousStrAttributes)
	{
		*attrlist = m_pAnonymousStrAttributes;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbCardProperty::GetAnonymousStrValuesList(nsVoidArray **valuelist)
{
	if (valuelist && m_pAnonymousStrValues)
	{
		*valuelist = m_pAnonymousStrValues;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbCardProperty::GetAnonymousIntAttrubutesList(nsVoidArray **attrlist)
{
	if (attrlist && m_pAnonymousIntAttributes)
	{
		*attrlist = m_pAnonymousIntAttributes;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbCardProperty::GetAnonymousIntValuesList(nsVoidArray **valuelist)
{
	if (valuelist && m_pAnonymousIntValues)
	{
		*valuelist = m_pAnonymousIntValues;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbCardProperty::GetAnonymousBoolAttrubutesList(nsVoidArray **attrlist)
{
	if (attrlist && m_pAnonymousBoolAttributes)
	{
		*attrlist = m_pAnonymousBoolAttributes;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbCardProperty::GetAnonymousBoolValuesList(nsVoidArray **valuelist)
{
	if (valuelist && m_pAnonymousBoolValues)
	{
		*valuelist = m_pAnonymousBoolValues;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

nsresult nsAbCardProperty::SetAnonymousAttribute
(nsVoidArray** pAttrAray, nsVoidArray** pValueArray, void *attrname, void *value)
{
	nsresult rv = NS_OK;
	nsVoidArray* pAttributes = *pAttrAray;
	nsVoidArray* pValues = *pValueArray; 

	if (!pAttributes && !pValues)
	{
		pAttributes = new nsVoidArray();
		pValues = new nsVoidArray();
		*pAttrAray = pAttributes;
		*pValueArray = pValues;
	}
	if (pAttributes && pValues)
	{
		if (attrname && value)
		{
			pAttributes->AppendElement(attrname);
			pValues->AppendElement(value);
		}
	}
	else
	{ 
		rv = NS_ERROR_FAILURE;
	}

	return rv;
}	


NS_IMETHODIMP nsAbCardProperty::SetAnonymousStringAttribute
(const char *attrname, const char *value)
{
	nsresult rv = NS_OK;

	char* pAttribute = PL_strdup(attrname);
	char* pValue = PL_strdup(value);
	if (pAttribute && pValue)
	{
		rv = SetAnonymousAttribute(&m_pAnonymousStrAttributes, 
			&m_pAnonymousStrValues, pAttribute, pValue);
	}
	else
	{
		PR_FREEIF(pAttribute);
		PR_FREEIF(pValue);
		rv = NS_ERROR_NULL_POINTER;
	}
	return rv;
}	

NS_IMETHODIMP nsAbCardProperty::SetAnonymousIntAttribute
(const char *attrname, PRUint32 value)
{
	nsresult rv = NS_OK;

	char* pAttribute = PL_strdup(attrname);
	PRUint32* pValue = (PRUint32 *)PR_Calloc(1, sizeof(PRUint32));
	*pValue = value;
	if (pAttribute && pValue)
	{
		rv = SetAnonymousAttribute(&m_pAnonymousIntAttributes, 
			&m_pAnonymousIntValues, pAttribute, pValue);
	}
	else
	{
		PR_FREEIF(pAttribute);
		PR_FREEIF(pValue);
		rv = NS_ERROR_NULL_POINTER;
	}
	return rv;
}	

NS_IMETHODIMP nsAbCardProperty::SetAnonymousBoolAttribute
(const char *attrname, PRBool value)
{
	nsresult rv = NS_OK;

	char* pAttribute = PL_strdup(attrname);
	PRBool* pValue = (PRBool *)PR_Calloc(1, sizeof(PRBool));
	*pValue = value;
	if (pAttribute && pValue)
	{
		rv = SetAnonymousAttribute(&m_pAnonymousBoolAttributes, 
			&m_pAnonymousBoolValues, pAttribute, pValue);
	}
	else
	{
		PR_FREEIF(pAttribute);
		PR_FREEIF(pValue);
		rv = NS_ERROR_NULL_POINTER;
	}
	return rv;
}

NS_IMETHODIMP nsAbCardProperty::AddAnonymousAttributesToDB()
{
	nsresult rv = NS_OK;
	if (mCardDatabase)
		mCardDatabase = null_nsCOMPtr();
	rv = GetCardDatabase("abdirectory://abook.mab");
	if (NS_SUCCEEDED(rv) && mCardDatabase)
		rv = mCardDatabase->AddAnonymousAttributesFromCard(this);
	return rv;
}

NS_IMETHODIMP nsAbCardProperty::EditAnonymousAttributesInDB()
{
	nsresult rv = NS_OK;
	if (mCardDatabase)
		mCardDatabase = null_nsCOMPtr();
	rv = GetCardDatabase("abdirectory://abook.mab");
	if (NS_SUCCEEDED(rv) && mCardDatabase)
		rv = mCardDatabase->EditAnonymousAttributesFromCard(this);
	return rv;
}

/* caller need to PR_smprintf_free *uri */
NS_IMETHODIMP nsAbCardProperty::GetCardURI(char **uri)
{
	char* cardURI = nsnull;
	nsFileSpec  *filePath = nsnull;
	if (mCardDatabase)
	{
		mCardDatabase->GetDbPath(&filePath);
		if (filePath)
		{
			char* file = nsnull;
			file = filePath->GetLeafName();
			if (file && m_dbRowID)
				cardURI = PR_smprintf("%s%s/Card%ld", kCardDataSourceRoot, file, m_dbRowID);
			if (file)
				nsCRT::free(file);
			delete filePath;
		}
	}
	if (cardURI)
	{
		*uri = cardURI;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

nsresult nsAbCardProperty::GetCardDatabase(const char *uri)
{
	nsresult rv = NS_OK;

	NS_WITH_SERVICE(nsIAddrBookSession, abSession, kAddrBookSessionCID, &rv); 
	if (NS_SUCCEEDED(rv))
	{
		nsFileSpec* dbPath;
		abSession->GetUserProfileDirectory(&dbPath);

		const char* file = nsnull;
		file = &(uri[PL_strlen(kDirectoryDataSourceRoot)]);
		(*dbPath) += file;

		NS_WITH_SERVICE(nsIAddrDatabase, addrDBFactory, kAddressBookDBCID, &rv);

		if (NS_SUCCEEDED(rv) && addrDBFactory)
			rv = addrDBFactory->Open(dbPath, PR_TRUE, getter_AddRefs(mCardDatabase), PR_TRUE);
	}
	return rv;
}

NS_IMETHODIMP nsAbCardProperty::AddCardToDatabase(const char *uri)
{
	if (!mCardDatabase && uri)
		GetCardDatabase(uri);

	if (mCardDatabase)
	{
		mCardDatabase->CreateNewCardAndAddToDB(this, PR_TRUE);
		mCardDatabase->Commit(kLargeCommit);
		return NS_OK;
	}
	else
		return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsAbCardProperty::EditCardToDatabase(const char *uri)
{
	if (!mCardDatabase && uri)
		GetCardDatabase(uri);

	if (mCardDatabase)
	{
		mCardDatabase->EditCard(this, PR_TRUE);
		mCardDatabase->Commit(kSmallCommit);
		return NS_OK;
	}
	else
		return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsAbCardProperty::CopyCard(nsIAbCard* srcCard)
{
	PRUnichar *str = nsnull;
	srcCard->GetFirstName(&str);
	SetFirstName(str);
	PR_FREEIF(str);

	srcCard->GetLastName(&str);
	SetLastName(str);
	PR_FREEIF(str);
	srcCard->GetDisplayName(&str);
	SetDisplayName(str);
	PR_FREEIF(str);
	srcCard->GetNickName(&str);
	SetNickName(str);
	PR_FREEIF(str);
	srcCard->GetPrimaryEmail(&str);
	SetPrimaryEmail(str);
	PR_FREEIF(str);
	srcCard->GetSecondEmail(&str);
	SetSecondEmail(str);
	PR_FREEIF(str);
	srcCard->GetWorkPhone(&str);
	SetWorkPhone(str);
	PR_FREEIF(str);
	srcCard->GetHomePhone(&str);
	SetHomePhone(str);
	PR_FREEIF(str);
	srcCard->GetFaxNumber(&str);
	SetFaxNumber(str);
	PR_FREEIF(str);
	srcCard->GetPagerNumber(&str);
	SetPagerNumber(str);
	PR_FREEIF(str);
	srcCard->GetCellularNumber(&str);
	SetCellularNumber(str);
	PR_FREEIF(str);
	srcCard->GetHomeAddress(&str);
	SetHomeAddress(str);
	PR_FREEIF(str);
	srcCard->GetHomeAddress2(&str);
	SetHomeAddress2(str);
	PR_FREEIF(str);
	srcCard->GetHomeCity(&str);
	SetHomeCity(str);
	PR_FREEIF(str);
	srcCard->GetHomeState(&str);
	SetHomeState(str);
	PR_FREEIF(str);
	srcCard->GetHomeZipCode(&str);
	SetHomeZipCode(str);
	PR_FREEIF(str);
	srcCard->GetHomeCountry(&str);
	SetHomeCountry(str);
	PR_FREEIF(str);
	srcCard->GetWorkAddress(&str);
	SetWorkAddress(str);
	PR_FREEIF(str);
	srcCard->GetWorkAddress2(&str);
	SetWorkAddress2(str);
	PR_FREEIF(str);
	srcCard->GetWorkCity(&str);
	SetWorkCity(str);
	PR_FREEIF(str);
	srcCard->GetWorkState(&str);
	SetWorkState(str);
	PR_FREEIF(str);
	srcCard->GetWorkZipCode(&str);
	SetWorkZipCode(str);
	PR_FREEIF(str);
	srcCard->GetWorkCountry(&str);
	SetWorkCountry(str);
	PR_FREEIF(str);
	srcCard->GetJobTitle(&str);
	SetJobTitle(str);
	PR_FREEIF(str);
	srcCard->GetDepartment(&str);
	SetDepartment(str);
	PR_FREEIF(str);
	srcCard->GetCompany(&str);
	SetCompany(str);
	PR_FREEIF(str);
	srcCard->GetWebPage1(&str);
	SetWebPage1(str);
	PR_FREEIF(str);
	srcCard->GetWebPage2(&str);
	SetWebPage2(str);
	PR_FREEIF(str);
	srcCard->GetBirthYear(&str);
	SetBirthYear(str);
	PR_FREEIF(str);
	srcCard->GetBirthMonth(&str);
	SetBirthMonth(str);
	PR_FREEIF(str);
	srcCard->GetBirthDay(&str);
	SetBirthDay(str);
	PR_FREEIF(str);
	srcCard->GetCustom1(&str);
	SetCustom1(str);
	PR_FREEIF(str);
	srcCard->GetCustom2(&str);
	SetCustom2(str);
	PR_FREEIF(str);
	srcCard->GetCustom3(&str);
	SetCustom3(str);
	PR_FREEIF(str);
	srcCard->GetCustom4(&str);
	SetCustom4(str);
	PR_FREEIF(str);
	srcCard->GetNotes(&str);
	SetNotes(str);
	PR_FREEIF(str);

	PRUint32 tableID, rowID;
	srcCard->GetDbTableID(&tableID);
	SetDbTableID(tableID);
	srcCard->GetDbRowID(&rowID);
	SetDbRowID(rowID);

	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::GetCollationKey(const PRUnichar *str, PRUnichar **key)
{
	nsresult rv;
	nsAutoString convertStr(str);
	nsAutoString resultStr;

	nsCOMPtr<nsILocale> locale; 
	nsCOMPtr<nsICollation> collationKeyGenerator;

    NS_WITH_SERVICE(nsILocaleService, localeSvc, kLocaleServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = localeSvc->GetApplicationLocale(getter_AddRefs(locale));
	if (NS_SUCCEEDED(rv) && locale)
	{
		nsCOMPtr <nsICollationFactory> factory;

		rv = nsComponentManager::CreateInstance(kCollationFactoryCID, NULL,
								  kICollationFactoryIID, getter_AddRefs(factory)); 
		if (NS_SUCCEEDED(rv) && factory)
		{
			rv = factory->CreateCollation(locale, getter_AddRefs(collationKeyGenerator));
			if (NS_SUCCEEDED(rv) && collationKeyGenerator)
			{
				rv = collationKeyGenerator->CreateSortKey(kCollationCaseInSensitive, convertStr, resultStr);
				*key = resultStr.ToNewUnicode();
			}
		}
	}

	return rv;
}
