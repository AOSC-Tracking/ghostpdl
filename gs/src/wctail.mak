#    Copyright (C) 1995, 1997, 1998 Aladdin Enterprises.  All rights reserved.
# 
# This file is part of Aladdin Ghostscript.
# 
# Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
# or distributor accepts any responsibility for the consequences of using it,
# or for whether it serves any particular purpose or works at all, unless he
# or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
# License (the "License") for full details.
# 
# Every copy of Aladdin Ghostscript must include a copy of the License,
# normally in a plain ASCII text file named PUBLIC.  The License grants you
# the right to copy, modify and redistribute Aladdin Ghostscript, but only
# under certain conditions described in the License.  Among other things, the
# License requires that the copyright notice and this notice be preserved on
# all copies.

# Id: wctail.mak 
# wctail.mak
# Last part of Watcom C/C++ makefile common to MS-DOS and MS Windows.

# Define the name of this makefile.
WCTAIL_MAK=$(GLSRCDIR)\wctail.mak

# Include the generic makefiles, except for devs.mak, contrib.mak, and int.mak.
#!include $(COMMONDIR)/watcdefs.mak
#!include $(COMMONDIR)/pcdefs.mak
#!include $(COMMONDIR)/generic.mak
!include $(GLSRCDIR)\version.mak
!include $(GLSRCDIR)\gs.mak
!include $(GLSRCDIR)\lib.mak
!include $(GLSRCDIR)\jpeg.mak
# zlib.mak must precede libpng.mak
!include $(GLSRCDIR)\zlib.mak
!include $(GLSRCDIR)\libpng.mak

# -------------------------- Auxiliary programs --------------------------- #

$(ECHOGS_XE): $(AUXGEN)echogs.$(OBJ)
	echo OPTION STUB=$(STUB) >_temp_.tr
	echo $(LIBPATHS) >>_temp_.tr
	$(LINK) @_temp_.tr FILE $(AUXGEN)echogs

$(AUXGEN)echogs.$(OBJ): $(GLSRC)echogs.c
	$(CCAUX) $(GLSRC)echogs.c $(O_)$(AUXGEN)echogs.$(OBJ)

$(GENARCH_XE): $(AUXGEN)genarch.$(OBJ)
	echo $(LIBPATHS) >_temp_.tr
	$(LINK) @_temp_.tr FILE $(AUXGEN)genarch

$(AUXGEN)genarch.$(OBJ): $(GLSRC)genarch.c $(stdpre_h)
	$(CCAUX) $(GLSRC)genarch.c $(O_)$(AUXGEN)genarch.$(OBJ)

$(GENCONF_XE): $(AUXGEN)genconf.$(OBJ)
	echo OPTION STUB=$(STUB) >_temp_.tr
	echo OPTION STACK=8k >>_temp_.tr
	echo $(LIBPATHS) >>_temp_.tr
	$(LINK) @_temp_.tr FILE $(AUXGEN)genconf

$(AUXGEN)genconf.$(OBJ): $(GLSRC)genconf.c $(stdpre_h)
	$(CCAUX) $(GLSRC)genconf.c $(O_)$(AUXGEN)genconf.$(OBJ)

$(GENDEV_XE): $(AUXGEN)gendev.$(OBJ)
	echo OPTION STUB=$(STUB) >_temp_.tr
	echo OPTION STACK=8k >>_temp_.tr
	echo $(LIBPATHS) >>_temp_.tr
	$(LINK) @_temp_.tr FILE $(AUXGEN)gendev

$(AUXGEN)gendev.$(OBJ): $(GLSRC)gendev.c $(stdpre_h)
	$(CCAUX) $(GLSRC)gendev.c $(O_)$(AUXGEN)gendev.$(OBJ)

$(GENINIT_XE): $(PSOBJ)geninit.$(OBJ)
	echo OPTION STUB=$(STUB) >_temp_.tr
	echo OPTION STACK=8k >>_temp_.tr
	echo $(LIBPATHS) >>_temp_.tr
	$(LINK) @_temp_.tr FILE $(PSOBJ)geninit

$(PSOBJ)geninit.$(OBJ): $(PSSRC)geninit.c $(stdio__h) $(string__h)
	$(CCAUX) $(PSSRC)geninit.c $(PSO_)geninit.$(OBJ)

# No special gconfig_.h is needed.
# Watcom `make' supports output redirection.
$(gconfig__h): $(WCTAIL_MAK)
	echo /* This file deliberately left blank. */ >$(gconfig__h)

$(gconfigv_h): $(WCTAIL_MAK) $(MAKEFILE) $(ECHOGS_XE)
	$(ECHOGS_XE) -w $(gconfigv_h) -x 23 define USE_ASM -x 2028 -q $(USE_ASM)-0 -x 29
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define USE_FPU -x 2028 -q $(FPU_TYPE)-0 -x 29
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define EXTEND_NAMES 0$(EXTEND_NAMES)
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define SYSTEM_CONSTANTS_ARE_WRITABLE 0$(SYSTEM_CONSTANTS_ARE_WRITABLE)
