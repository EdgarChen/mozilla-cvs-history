/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- 
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
 * jparser.cpp
 * John Sun
 * 4/28/98 10:48:13 AM
 */

#include "stdafx.h"
#include "jdefines.h"

#include <unistring.h>
#include "jparser.h"
#include "prprty.h"
#include "tmbevent.h"
#include "nscal.h"
#include "jlog.h"
#include "keyword.h"
#include "icalsrdr.h"

#include "nspr.h"

//---------------------------------------------------------------------

JulianParser::JulianParser()
{
    m_Reader = 0;
    m_OutCalendars = 0;
    m_Thread = 0;
    m_bParseStarted = FALSE;
    m_bParseFinished = FALSE;
}

//---------------------------------------------------------------------

JulianParser::JulianParser(ICalCAPIReader * reader, 
                           JulianPtrArray * outCalendars,
                           PRThread * thread)
{
    m_Reader = reader;
    m_OutCalendars = outCalendars;
    m_Thread = thread;
    m_bParseStarted = FALSE;
    m_bParseFinished = FALSE;
}

//---------------------------------------------------------------------

JulianUtility::MimeEncoding
JulianParser::stringToEncodingType(UnicodeString & propVal)
{
    if (propVal.compareIgnoreCase(JulianKeyword::Instance()->ms_s7bit) == 0)
    {
        return JulianUtility::MimeEncoding_7bit;
    }
    else if (propVal.compareIgnoreCase(JulianKeyword::Instance()->ms_sQUOTED_PRINTABLE) == 0)
    {
        return JulianUtility::MimeEncoding_QuotedPrintable;
    }
    else if (propVal.compareIgnoreCase(JulianKeyword::Instance()->ms_sBase64) == 0)
    {
        return JulianUtility::MimeEncoding_Base64;
    }
    else
        return JulianUtility::MimeEncoding_7bit;
}

//---------------------------------------------------------------------

void JulianParser::ParseCalendars()
{
    JulianParser::ParseCalendars((ICalReader *) m_Reader, m_OutCalendars);
}

//---------------------------------------------------------------------

void JulianParser::ParseCalendars(ICalReader * reader, 
                                  JulianPtrArray * outCalendars)
{
    if (outCalendars == 0)
        return;

    JulianPtrArray * parameters = new JulianPtrArray();
   
    // TODO: this needs to be changed to capireader later
    PR_ASSERT(parameters != 0);
    if (parameters != 0)
    {
        JLog * log = 0;
        ErrorCode status = ZERO_ERROR;
        UnicodeString strLine, propName, propVal;
        JulianUtility::MimeEncoding encoding = JulianUtility::MimeEncoding_7bit;

        ICalCAPIReader * cr = (ICalCAPIReader *) reader;

        while(TRUE)
        {
            //PR_EnterMonitor((PRMonitor *) cr->getMonitor());
            //reader->readFullLine(strLine, status);
            cr->readFullLine(strLine, status);
            ICalProperty::Trim(strLine);

#if TESTING_ITIPRIG
            if (FALSE) TRACE("\t--Parser: line (size = %d) = ---%s---\r\n", 
                strLine.size(), strLine.toCString(""));       
#endif
            if (FAILURE(status) && strLine.size() == 0)
            {
                //PR_Notify((PRMonitor *) cr->getMonitor());
                if (cr->isFinished())
                {
                    break;
                }
#if TESTING_ITIPRIG
                if (FALSE) TRACE("\t--jParser: yielding\r\n");
#endif
                PR_Sleep(PR_INTERVAL_NO_WAIT);
                //break;
            }

            ICalProperty::parsePropertyLine(strLine, propName,
                propVal, parameters);
                                
#if TESTING_ITIPRIG
            if (TRUE) TRACE("\t--Parser: propName = --%s--, propVal = --%s--,paramSize = %d\r\n",
                propName.toCString(""), propVal.toCString(""), parameters->GetSize());
#endif
            if ((propName.compareIgnoreCase(JulianKeyword::Instance()->ms_sBEGIN) == 0) &&
                (propVal.compareIgnoreCase(JulianKeyword::Instance()->ms_sVCALENDAR) == 0))
            {
                // parse an NSCalendar, add it to outCalendars                    
                NSCalendar * cal = new NSCalendar(log);
                UnicodeString fileName;
                cal->parse(reader, fileName, encoding);
                outCalendars->Add(cal);
            }
            else if (propName.compareIgnoreCase(
                JulianKeyword::Instance()->ms_sCONTENT_TRANSFER_ENCODING) == 0)
            {
                ICalProperty::Trim(propVal);
                encoding = stringToEncodingType(propVal);
                cr->setEncoding(encoding);
            }
            ICalProperty::deleteICalParameterVector(parameters);
            parameters->RemoveAll();
    
            //PR_ExitMonitor((PRMonitor *)cr->getMonitor());
        }

        ICalProperty::deleteICalParameterVector(parameters);
        parameters->RemoveAll();
        delete parameters; parameters = 0;
        setParseFinished();
    }    
}
//---------------------------------------------------------------------

void jparser_ParseCalendarsZero(void * jp, void * nPtr)
{
    //if (FALSE) ("--calling ParseCalendarsZero\r\n");
    JulianParser * julPar = (JulianParser *) jp;
    julPar->ParseCalendars();
#if TESTING_ITIPRIG
    TRACE("\t--killing parseThread\r\n");
#endif
    PR_ProcessExit(0);
}

//---------------------------------------------------------------------



