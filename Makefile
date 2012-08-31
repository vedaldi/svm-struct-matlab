# file:   Makefile
# brief:  Compile MEXified SVM-struct
# author: Andrea Vedaldi

MEX ?= mex
VER = 1.2
PACKAGE = svm-struct-matlab

# --------------------------------------------------------------------
#                                             Auto-detect architecture
# --------------------------------------------------------------------

Darwin_PPC_ARCH := mac
Darwin_Power_Macintosh_ARCH := mac
Darwin_i386_ARCH := maci64
Darwin_x86_64_ARCH := maci64
Linux_i386_ARCH := glnx86
Linux_i686_ARCH := glnx86
Linux_unknown_ARC := glnx86
Linux_x86_64_ARCH := glnxa64

UNAME := $(shell uname -sm)
ARCH ?= $($(shell echo "$(UNAME)" | tr \  _)_ARCH)

# Mac OS X Intel 32
ifeq ($(ARCH),maci)
SDKROOT ?= $(shell xcodebuild -version -sdk macosx | sed -n '/^Path\:/p' | sed 's/^Path: //')
MACOSX_DEPLOYMENT_TARGET ?= 10.4
CFLAGS += -m32 -isysroot $(SDKROOT) -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
LDFLAGS += -Wl,-syslibroot,$(SDKROOT) -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
MEXEXT = mexmaci
CC = gcc
MEXFLAGS += CC='$(CC)' LD='$(CC)'
endif

# Mac OS X Intel 64
ifeq ($(ARCH),maci64)
SDKROOT ?= $(shell xcodebuild -version -sdk macosx | sed -n '/^Path\:/p' | sed 's/^Path: //')
MACOSX_DEPLOYMENT_TARGET ?= 10.4
CFLAGS += -m64 -isysroot $(SDKROOT) -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
LDFLAGS += -Wl,-syslibroot,$(SDKROOT) -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
MEXEXT = mexmaci64
CC = gcc
MEXFLAGS += CC='$(CC)' LD='$(CC)'
endif

# Linux-32
ifeq ($(ARCH),glnx86)
CFLAGS  += -march=i686
LDFLAGS +=
MEXEXT = mexglx
endif

# Linux-64
ifeq ($(ARCH),glnxa64)
LDFLAGS +=
MEXEXT = mexa64
endif

MEXFLAGS += -largeArrayDims -$(ARCH) CFLAGS='$$CFLAGS $(CFLAGS) -Wall' LDFLAGS='$$LDFLAGS $(LDFLAGS)'
BUILD = build/$(ARCH)

# --------------------------------------------------------------------
#                                                                Build
# --------------------------------------------------------------------

svm_light_objs := \
$(BUILD)/svm_light/svm_hideo.o \
$(BUILD)/svm_light/svm_learn.o \
$(BUILD)/svm_light/svm_common.o

svm_struct_objs := \
$(BUILD)/svm_struct/svm_struct_learn.o \
$(BUILD)/svm_struct/svm_struct_common.o

svm_custom_objs := \
$(BUILD)/svm_struct_api.o \
$(BUILD)/svm_struct_learn_custom.o

$(BUILD)/%.o : %.c
	$(MEX) $(MEXFLAGS) -outdir "$(dir $@)" -c "$<"

svm_struct_learn.$(MEXEXT) : svm_struct_learn_mex.c \
  $(svm_custom_objs) \
  $(svm_light_objs) \
  $(svm_struct_objs)
	$(MEX) $(MEXFLAGS) $^ -output "$@"

.PHONY: clean
clean:
	rm -fv $(svm_custom_objs) $(svm_struct_objs) $(svm_light_objs)
	find . -name '*~' -delete

.PHONY: distclean
distclean: clean
	for ext in mexmaci mexmaci64 mexglx mexa64 ; \
	do \
	  rm -fv svm_struct_learn.$${ext} ; \
	done
	rm -rf build
	rm -rf $(PACKAGE)-*.tar.gz

.PHONY: dist
dist:
	git archive --format=tar --prefix=$(PACKAGE)-$(VER)/ v$(VER) | gzip >$(PACKAGE)-$(VER).tar.gz
	@if [ -n "$$(git diff v$(VER) HEAD)" ] ; \
	then \
	   echo "Warning: the repository HEAD is not the same as the tag v$(VER)" ; \
	fi


# svm_struct dependencies
svm_struct_api.o: \
  $(BUILD)/.dir \
  svm_struct_api.c \
  svm_struct_api.h \
  svm_struct_api_types.h \
  svm_struct/svm_struct_common.h

svm_struct_learn_custom.o: \
  $(BUILD)/.dir \
  svm_struct_learn_custom.c \
  svm_struct_api.h \
  svm_struct_api_types.h \
  svm_light/svm_common.h \
  svm_struct/svm_struct_common.h

svm_struct/svm_struct_mex.o : \
  $(BUILD)/.dir \
  svm_struct/svm_struct_mex.c

svm_struct/svm_struct_common.o : \
  $(BUILD)/.dir \
  svm_struct/svm_struct_common.c \
  svm_struct/svm_struct_common.h \
  svm_light/svm_common.h \
  svm_struct_api_types.h

svm_struct/svm_struct_learn.o : \
  $(BUILD)/.dir \
  svm_struct/svm_struct_learn.c \
  svm_struct/svm_struct_common.h \
  svm_light/svm_common.h \
  svm_light/svm_learn.h \
  svm_struct_api_types.h \
  svm_struct_api.h

svm_struct/svm_struct_classify.o : \
  $(BUILD)/.dir \
  svm_struct/svm_struct_classify.c \
  svm_struct/svm_struct_common.h \
  svm_light/svm_common.h \
  svm_struct_api_types.h \
  svm_struct_api.h

# svm_light dependencies
svm_light/svm_learn.o : \
  $(BUILD)/.dir \
  svm_light/svm_learn.c \
  svm_light/svm_learn.h \
  svm_light/svm_common.h

svm_light/svm_common.o : \
  $(BUILD)/.dir \
  svm_light/svm_common.c \
  svm_light/svm_common.h \
  svm_light/kernel.h

svm_light/svm_hideo.o : \
  $(BUILD)/.dir \
  svm_light/svm_hideo.c

