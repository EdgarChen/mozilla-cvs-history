/* -*- Mode: java; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Grendel mail/news client.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1997
 * Netscape Communications Corporation.  All Rights Reserved.
 *
 * Modified: Jeff Galyan <jeffrey.galyan@sun.com>, 30 Dec 1998
 */

package grendel.composition;

import calypso.util.Assert;

import java.awt.BorderLayout;
import java.util.Properties;
import java.util.Vector;

import javax.swing.*;
import javax.swing.event.*;

//import netscape.orion.toolbars.*;
//import netscape.orion.menus.NsMenuManager;

import grendel.storage.MessageExtra;
import grendel.storage.MessageExtraFactory;
import grendel.widgets.Animation;
import grendel.ui.GeneralFrame;
import grendel.ui.StoreFactory;

import javax.mail.Address;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.mail.Session;

/**
 *
 * @author  Lester Schueler
 */
public class Composition extends GeneralFrame {
    CompositionPanel mCompositionPanel;
    AddressBar mAddressBar;

    public static void main(String[] args) {
        //check arguments
        if (2 != args.length) {
            System.out.println ("Usage: composition mail_server_name user_name");
            System.exit(0);
        }

        Composition compFrame = new Composition ();
        compFrame.show();
    }

    /**
     *
     */
    public Composition() {
        super("Composition", "mail.composition");
        fResourceBase = "grendel.composition";

        Session session = StoreFactory.Instance().getSession();

        //session.setDebug(true);
        mCompositionPanel = new CompositionPanel(session);

        mCompositionPanel.addCompositionPanelListener(new PanelListener());

        //create menubar (top)
        //  fMenu = buildMenu("mainMenubar",
        //                  mCompositionPanel.getActions());
        fMenu = buildMenu();

        getRootPane().setMenuBar(fMenu);

        fToolBar = mCompositionPanel.getToolBar();
        //    fToolBar.addItem(ToolbarFactory.MakeINSToolbarItem(ToolBarLayout.CreateSpring(),
        //                                                  null));
//    fToolBar.addItem(ToolbarFactory.MakeINSToolbarItem(fAnimation, null));

        mAddressBar = mCompositionPanel.getAddressBar();

        //top collapsible item
        fToolBarPanel.add(fToolBar);
        //bottom collapsible item
        fToolBarPanel.add(mAddressBar);

        fStatusBar = buildStatusBar();
        fPanel.add(BorderLayout.SOUTH, fStatusBar);

        fPanel.add(mCompositionPanel);

        restoreBounds();
    }

    public void dispose() {
        saveBounds();
        super.dispose(); // call last
    }

    protected void startAnimation() {
        super.startAnimation();
    }

    protected void stopAnimation() {
        super.stopAnimation();
    }

    /** Initialize the headers of this composition as being a reply to
      the given message. */
    public void initializeAsReply(Message msg, boolean replyall) {
        mCompositionPanel.setReferredMessage(msg);
        MessageExtra mextra = MessageExtraFactory.Get(msg);
        int i;
        Vector dests = new Vector();
        Address from[] = null;
        try {
            from = msg.getReplyTo();
        } catch (MessagingException e) {
        }
        if (from == null || from.length == 0) {
            try {
                from = msg.getFrom();
            } catch (MessagingException e) {
            }
        }

        if (from != null) {
            for (i=0 ; i<from.length ; i++) {
                dests.addElement(new Addressee(from[i], Addressee.TO));
            }
        }
        if (replyall) {
            for (int w=0 ; w<2 ; w++) {
                Address list[] = null;
                try {
                    list = msg.getRecipients(w == 0 ? Message.RecipientType.TO : Message.RecipientType.CC);
                } catch (MessagingException e) {
                }
                if (list != null) {
                    for (i=0 ; i<list.length ; i++) {
                        dests.addElement(new Addressee(list[i], Addressee.CC));
                    }
                }
            }
        }
        Addressee destarray[] = new Addressee[dests.size()];
        dests.copyInto(destarray);
        mAddressBar.getAddressList().setAddresses(destarray);

        try {
            mCompositionPanel.setSubject("Re: " + mextra.simplifiedSubject());
        } catch (MessagingException e) {
        }
    }

    class PanelListener implements CompositionPanelListener {
        public void sendingMail(ChangeEvent aEvent) {
            startAnimation();
        }
        public void doneSendingMail(ChangeEvent aEvent) {
            stopAnimation();
            dispose();
        }
        public void sendFailed(ChangeEvent aEvent) {
            stopAnimation();
        }
    }
}

