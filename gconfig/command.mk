# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 

#######################################################################
# Master "Core Components" default command macros;                    #
# can be overridden in <arch>.mk                                      #
#######################################################################

AS            = $(CC)
ASFLAGS      += $(CFLAGS)
CCF           = $(CC) $(CFLAGS)
PURIFY        = purify $(PURIFYOPTIONS)
LINK_DLL      = $(LINK) $(OS_DLLFLAGS) $(DLLFLAGS)
LINK_EXE      = $(LINK) $(OS_LFLAGS) $(LFLAGS)
NFSPWD        = $(NSINSTALL_DIR)/nfspwd
CFLAGS       += -I$(SOURCE_XP_DIR)/include $(OPTIMIZER) $(OS_CFLAGS) $(XP_DEFINE) $(DEFINES) $(INCLUDES) \
		$(XCFLAGS)
RANLIB        = echo
JAR           = /bin/jar
TAR           = /bin/tar
CFLAGS       +=-DDEBUG_$(USERNAME)
ifdef NSPR20
DEFINES     += -DNSPR20
endif
ifdef MOZ_TREX
DEFINES     += -DMOZ_TREX
endif

MOZ_TOOLS_FLIPPED=$(MOZ_TOOLS:\=/)
DOCXX         = $(MOZ_TOOLS_FLIPPED)/bin/docxx
DOCXX_FLAGS   = -H -A -p -f -j -a -u -G -b

#
# For purify
#
NOMD_CFLAGS  += $(OPTIMIZER) $(NOMD_OS_CFLAGS) $(XP_DEFINE) $(DEFINES) $(INCLUDES) \
		$(XCFLAGS)

