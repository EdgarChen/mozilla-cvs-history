ifndef OBJDIR
  ifdef OBJDIR_NAME
    OBJDIR = $(OBJDIR_NAME)
  endif
endif

NSPR_VERSION     = v3.1
NSPR_LOCAL       = $(MOZ_DEPTH)/dist/$(OBJDIR)/nspr
NSPR_DIST        = $(MOZ_DEPTH)/dist/$(OBJDIR)
NSPR_OBJDIR      = $(OBJDIR)
ifeq ($(OS_ARCH), SunOS)
  NSPR_OBJDIR   := $(subst _sparc,,$(NSPR_OBJDIR))
endif
ifeq ($(OS_ARCH), Linux)
  LINUX_REL     := $(shell uname -r)
  ifneq (,$(findstring 2.0,$(LINUX_REL)))
    NSPR_OBJDIR := $(subst _All,2.0_x86_glibc_PTH,$(NSPR_OBJDIR))
  else
    NSPR_OBJDIR := $(subst _All,2.2_x86_glibc_PTH,$(NSPR_OBJDIR))
  endif
endif
ifeq ($(OS_ARCH), AIX)
  NSPR_OBJDIR   := $(subst 4.1,4.2,$(NSPR_OBJDIR))
endif
ifeq ($(OS_CONFIG), IRIX6.2)
  NSPR_OBJDIR   := $(subst 6.2,6.2_n32_PTH,$(NSPR_OBJDIR))
endif
ifeq ($(OS_CONFIG), IRIX6.5)
  NSPR_OBJDIR   := $(subst 6.5,6.5_n32_PTH,$(NSPR_OBJDIR))
endif
ifeq ($(OS_ARCH), WINNT)
  ifeq ($(OBJDIR), WIN32_D.OBJ)
    NSPR_OBJDIR  = WINNT4.0_DBG.OBJ
  endif
  ifeq ($(OBJDIR), WIN32_O.OBJ)
    NSPR_OBJDIR  = WINNT4.0_OPT.OBJ
  endif
endif
NSPR_SHARED      = /share/builds/components/nspr20/$(NSPR_VERSION)/$(NSPR_OBJDIR)
ifeq ($(OS_ARCH), WINNT)
  NSPR_SHARED    = nspr20/$(NSPR_VERSION)/$(NSPR_OBJDIR)
endif
NSPR_VERSIONFILE = $(NSPR_LOCAL)/Version
NSPR_CURVERSION := $(shell cat $(NSPR_VERSIONFILE))

get_nspr:
	@echo "Grabbing NSPR component..."
ifeq ($(NSPR_VERSION), $(NSPR_CURVERSION))
	@echo "No need, NSPR is up to date in this tree (ver=$(NSPR_VERSION))."
else
	mkdir -p $(NSPR_LOCAL)
	mkdir -p $(NSPR_DIST)
  ifneq ($(OS_ARCH), WINNT)
	cp       $(NSPR_SHARED)/*.jar $(NSPR_LOCAL)
  else
	sh       $(MOZ_DEPTH)/../reltools/compftp.sh $(NSPR_SHARED) $(NSPR_LOCAL) *.jar
  endif
	unzip -o $(NSPR_LOCAL)/mdbinary.jar -d $(NSPR_DIST)
	mkdir -p $(NSPR_DIST)/include
	unzip -o $(NSPR_LOCAL)/mdheader.jar -d $(NSPR_DIST)/include
	rm -rf   $(NSPR_DIST)/META-INF
	rm -rf   $(NSPR_DIST)/include/META-INF
	echo $(NSPR_VERSION) > $(NSPR_VERSIONFILE)
endif

SHIP_DIST  = $(MOZ_DEPTH)/dist/$(OBJDIR)
SHIP_DIR   = $(SHIP_DIST)/SHIP

SHIP_LIBS      = libjs.$(SO_SUFFIX) libjs.a
ifdef JS_LIVECONNECT
  SHIP_LIBS   += libjsj.$(SO_SUFFIX) libjsj.a
endif
ifeq ($(OS_ARCH), WINNT)
  SHIP_LIBS    = js32.dll js32.lib
  ifdef JS_LIVECONNECT
    SHIP_LIBS += jsj.dll jsj.lib
  endif
endif
SHIP_LIBS     += $(LCJAR)
SHIP_LIBS     := $(addprefix $(SHIP_DIST)/lib/, $(SHIP_LIBS))

SHIP_INCS      = js*.h prmjtime.h resource.h *.msg *.tbl
ifdef JS_LIVECONNECT
  SHIP_INCS   += netscape*.h nsC*.h nsI*.h
endif
SHIP_INCS     := $(addprefix $(SHIP_DIST)/include/, $(SHIP_INCS))

SHIP_BINS      = js
ifdef JS_LIVECONNECT
  SHIP_BINS   += lcshell
endif
ifeq ($(OS_ARCH), WINNT)
  SHIP_BINS   := $(addsuffix .exe, $(SHIP_BINS))
endif
SHIP_BINS     := $(addprefix $(SHIP_DIST)/bin/, $(SHIP_BINS))

ifdef BUILD_OPT
  JSREFJAR = jsref_opt.jar
else
ifdef BUILD_IDG
  JSREFJAR = jsref_idg.jar
else
  JSREFJAR = jsref_dbg.jar
endif
endif

ship:
	mkdir -p $(SHIP_DIR)/lib
	mkdir -p $(SHIP_DIR)/include
	mkdir -p $(SHIP_DIR)/bin
	cp $(SHIP_LIBS) $(SHIP_DIR)/lib
	cp $(SHIP_INCS) $(SHIP_DIR)/include
	cp $(SHIP_BINS) $(SHIP_DIR)/bin
	cd $(SHIP_DIR); \
	  zip -r $(JSREFJAR) bin lib include
ifdef BUILD_SHIP
	cp $(SHIP_DIR)/$(JSREFJAR) $(BUILD_SHIP)
endif

CWD = $(shell pwd)
shipSource: $(SHIP_DIR)/jsref_src.lst .FORCE
	mkdir -p $(SHIP_DIR)
	cd $(MOZ_DEPTH)/.. ; \
	  zip $(CWD)/$(SHIP_DIR)/jsref_src.jar -@ < $(CWD)/$(SHIP_DIR)/jsref_src.lst
ifdef BUILD_SHIP
	cp $(SHIP_DIR)/jsref_src.jar $(BUILD_SHIP)
endif

JSREFSRCDIRS := $(shell cat $(DEPTH)/SpiderMonkey.rsp)
$(SHIP_DIR)/jsref_src.lst: .FORCE
	mkdir -p $(SHIP_DIR)
	rm -f $@
	touch $@
	for d in $(JSREFSRCDIRS); do                                \
	  cd $(MOZ_DEPTH)/..;                                       \
	  ls -1 -d $$d | grep -v CVS | grep -v \.OBJ >> $(CWD)/$@;  \
	  cd $(CWD);                                                \
	done

.FORCE:
