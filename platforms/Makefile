
#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of images to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
ifndef PDIR
UP_EXTRACT_DIR = ..
GEN_LIBS = libplatforms.a
COMPONENTS_libplatforms = joylink/libplatforms_joylink.a
endif


#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#
#DEFINES += 

# DEFINES += -D__LINUX_UB2__ -D__LINUX__ -D__AVR__=0 -D_GET_HOST_BY_NAME_
DEFINES += -D__AVR__=0 -D_GET_HOST_BY_NAME_
#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include
INCLUDES += -I ./
INCLUDES += -I ./adaptation/joylink_dev_sdk_2.0_2018_05_24/joylink
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile

