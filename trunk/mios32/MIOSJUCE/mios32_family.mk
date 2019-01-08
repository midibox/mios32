# $Id: mios32_family.mk 231 2009-01-02 00:28:11Z tk $
# defines additional rules for MIOS32 family

# driver library must already be built and linked in

# enhance include path
C_INCLUDE += 

# add modules to thumb sources
THUMB_SOURCE += 

THUMB_AS_SOURCE += 

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/mios32/$(FAMILY)
