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
 * Created: Terry Weissman <terry@netscape.com>, 28 Aug 1997.
 */

package grendel.storage;

import calypso.util.Assert;
import calypso.util.ByteBuf;
import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;

import javax.mail.Flags;
import javax.mail.Folder;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.mail.internet.InternetHeaders;

class BerkeleyMessage extends MessageBase {
  int fIndex;
  int fLength;

  /* Note: these flag values are compatible with the X-Mozilla-Status
     headers generated by Netscape Mail 2.0 through 4.0.  Don't change
     them.

     Note also: Mozilla 2.0 and 3.0 can't read X-Mozilla-Status headers
     that contain other than exactly 4 hex digits (16 bits.)  If we
     ever need to save more than 16 flags to the file, we will probably
     have to do so incompatibly...

     Think twice before adding a new flag here!  Do you really need to
     write it to X-Mozilla-Status?  Is parsing it from another header
     good enough?  These flags are scarce, and once added, they can't
     ever be removed without breaking old files.
   */
  protected static final int X_MOZILLA_FLAG_READ      = 0x0001;
  protected static final int X_MOZILLA_FLAG_REPLIED   = 0x0002;  // since 4.0
  protected static final int X_MOZILLA_FLAG_MARKED    = 0x0004;
  protected static final int X_MOZILLA_FLAG_DELETED   = 0x0008;
  protected static final int X_MOZILLA_FLAG_HAS_RE    = 0x0010;
  /* 0x0020, 0x0040, 0x0080, and 0x0100 are unused. */
  protected static final int X_MOZILLA_FLAG_SMTP_AUTH = 0x0200;  // since 4.0
  protected static final int X_MOZILLA_FLAG_PARTIAL   = 0x0400;
  protected static final int X_MOZILLA_FLAG_QUEUED    = 0x0800;  // since 3.0?
  protected static final int X_MOZILLA_FLAG_FORWARDED = 0x1000;  // since 4.0
  /* Note: this one is 3 bits. */
  protected static final int X_MOZILLA_FLAG_PRIORITY  = 0xE000;  // since 4.0


  BerkeleyMessage(BerkeleyFolder f, InternetHeaders h) {
    super(f, h);
    parseMozillaStatus(h);
  }

  BerkeleyMessage(BerkeleyFolder f,
                  long date,
                  long flags,
                  ByteBuf author,
                  ByteBuf recipient,
                  ByteBuf subj,
                  ByteBuf id,
                  ByteBuf refs[]) {
    super(f, date, flags, author, recipient, subj, id, refs);
  }

  BerkeleyMessage(BerkeleyFolder f,
                  long date,
                  long flags,
                  ByteBuf author,
                  ByteBuf recipient,
                  ByteBuf subj,
                  MessageID id,
                  MessageID refs[]) {
    super(f, date, flags, author, recipient, subj, id, refs);
  }


  protected void parseMozillaStatus(InternetHeaders h) {
    int xms = 0;
    String hh[] = h.getHeader("X-Mozilla-Status");
    if (hh != null && hh.length != 0) {
      byte value[] = hh[0].trim().getBytes();
      /* sucks that we can't use Short.parseShort(buf, 16). */
      for (int i = 0; i < value.length; i++) {
        byte b = value[i];
        if       (b >= '0' && b <= '9') xms = (xms << 4) | (b - '0');
        else if  (b >= 'A' && b <= 'F') xms = (xms << 4) | (b - 'A' + 10);
        else if  (b >= 'a' && b <= 'f') xms = (xms << 4) | (b - 'a' + 10);
        else { xms = 0; break; }
      }
      flags = mozillaFlagsToInternalFlags(xms);
    }
  }

  /** Given a value read from an X-Mozilla-Status header, returns a
      value suitable for storing in MessageBase.flags.
      (The two types of flags don't use the same space or range.)
   */
  static long mozillaFlagsToInternalFlags(long x_mozilla_status) {
    long xms = x_mozilla_status;
    long flags = 0;
    if ((xms & X_MOZILLA_FLAG_READ)      != 0) flags |= FLAG_READ;
    if ((xms & X_MOZILLA_FLAG_REPLIED)   != 0) flags |= FLAG_REPLIED;
    if ((xms & X_MOZILLA_FLAG_MARKED)    != 0) flags |= FLAG_MARKED;
    if ((xms & X_MOZILLA_FLAG_DELETED)   != 0) flags |= FLAG_DELETED;
    if ((xms & X_MOZILLA_FLAG_HAS_RE)    != 0) flags |= FLAG_HAS_RE;
    if ((xms & X_MOZILLA_FLAG_SMTP_AUTH) != 0) flags |= FLAG_SMTP_AUTH;
    if ((xms & X_MOZILLA_FLAG_PARTIAL)   != 0) flags |= FLAG_PARTIAL;
    if ((xms & X_MOZILLA_FLAG_QUEUED)    != 0) flags |= FLAG_QUEUED;
    if ((xms & X_MOZILLA_FLAG_FORWARDED) != 0) flags |= FLAG_FORWARDED;
 /* if ((xms & X_MOZILLA_FLAG_PRIORITY)  != 0) ... #### */
    return flags;
  }

  /** Given a value of the form found in MessageBase.flags, returns a
      value that may be written to an X-Mozilla-Flags header.
      (The two types of flags don't use the same space or range.)
   */
  static long internalFlagsToMozillaFlags(long internal_flags) {
    long f = internal_flags;
    long xms = 0;
    if ((f & FLAG_READ)      != 0) xms |= X_MOZILLA_FLAG_READ;
    if ((f & FLAG_REPLIED)   != 0) xms |= X_MOZILLA_FLAG_REPLIED;
    if ((f & FLAG_MARKED)    != 0) xms |= X_MOZILLA_FLAG_MARKED;
    if ((f & FLAG_DELETED)   != 0) xms |= X_MOZILLA_FLAG_DELETED;
    if ((f & FLAG_HAS_RE)    != 0) xms |= X_MOZILLA_FLAG_HAS_RE;
    if ((f & FLAG_SMTP_AUTH) != 0) xms |= X_MOZILLA_FLAG_SMTP_AUTH;
    if ((f & FLAG_PARTIAL)   != 0) xms |= X_MOZILLA_FLAG_PARTIAL;
    if ((f & FLAG_QUEUED)    != 0) xms |= X_MOZILLA_FLAG_QUEUED;
    if ((f & FLAG_FORWARDED) != 0) xms |= X_MOZILLA_FLAG_FORWARDED;
    /* #### X_MOZILLA_FLAG_PRIORITY */
    return xms;
  }

  /** Returns a reasonable value for the X-Mozilla-Status header of the
      given Message.  If the Message is a subclass of MessageBase, this
      value will be based on MessageBase.flags; otherwise, we will go through
      the painful string-based javamail API.  (The Message may also be null.)
   */

  static long makeMozillaFlags(Message m) {
    long xms = 0;
    if (m instanceof MessageBase) {
      xms = internalFlagsToMozillaFlags(((MessageBase)m).flags);
    } else if (m == null) {
      xms = X_MOZILLA_FLAG_READ;
    } else {
      for (int i=0 ; i<FLAGDEFS.length ; i++) {
        MessageBase mb = (MessageBase) m;
        if (FLAGDEFS[i].builtin != null
            ? mb.isSet(FLAGDEFS[i].builtin)
            : mb.isSet(FLAGDEFS[i].non_builtin)) {
          xms |= FLAGDEFS[i].flag;
        }
      }
      xms = internalFlagsToMozillaFlags(xms);
    }
    return xms;
  }


  void setSize(int l) {
    fLength = l;
  }

  public int getSize() {
    return fLength;
  }

  void setStorageFolderIndex(int p) {
    fIndex = p;
  }

  int getStorageFolderIndex() {
    return fIndex;
  }

  public InputStream getInputStreamWithHeaders()
    throws MessagingException
  {
    BerkeleyFolder f = (BerkeleyFolder) getFolder();
    try {
      return new PartialFileInputStream(f.getFile(), getStorageFolderIndex(),
                                        fLength);
    } catch (IOException e) {
      throw new MessagingException(e.toString());
    }
  }


  protected void setFlagBit(long flag, boolean value) {
    long oflags = flags;
    super.setFlagBit(flag, value);
    if (oflags != flags) {
      setFlagsDirty(true);
      ((BerkeleyFolder) folder).setFlagsDirty(true, this, oflags);
    }
  }

  public void writeTo(OutputStream aStream) {
    Assert.NotYetImplemented("BerkeleyFolder.writeTo(IOStream)");
  }

  public int getLineCount() {
    Assert.NotYetImplemented("BerkeleyFolder.getLineCount()");
    return 0;
  }
}
