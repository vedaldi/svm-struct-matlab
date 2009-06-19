# file:   Makefile
# brief:  Compile MEXified SVM-struct
# author: Andrea Vedaldi

MEX ?= mex
MATLABROOT := $(shell $(MEX) -v 2>&1 | grep "MATLAB  * =" | sed 's/->.*= //g')
MEXEXT := $(shell $(MATLABROOT)/bin/mexext)
MEXFLAGS := -largeArrayDims CFLAGS='$$CFLAGS $(CFLAGS) -Wall'

.PHONY: all
all: svm_struct_learn.$(MEXEXT)

svm_light_objs := \
  svm_light/svm_hideo.o \
  svm_light/svm_learn.o \
  svm_light/svm_common.o

svm_struct_objs := \
  svm_struct/svm_struct_learn.o \
  svm_struct/svm_struct_common.o

svm_custom_objs := \
  svm_struct_api.o \
  svm_struct_learn_custom.o

%.o : %.c
	$(MEX) $(MEXFLAGS) -outdir "$(dir $@)" -c "$<"
	
svm_struct_learn.$(MEXEXT) : svm_struct_learn_mex.c \
  $(svm_custom_objs) \
  $(svm_light_objs) \
  $(svm_struct_objs)
	$(MEX) $(MEXFLAGS) $^ -output "$@"

.PHONY: clean
clean:
	rm -fv $(svm_custom_objs) $(svm_struct_objs) $(svm_light_objs)

.PHONY: distclean
distclean: clean
	rm -fv svm_struct_learn.$(MEXEXT)

# svm_struct dependencies
svm_struct_api.o: \
  svm_struct_api.c \
  svm_struct_api.h \
  svm_struct_api_types.h \
  svm_struct/svm_struct_common.h

svm_struct_learn_custom.o: \
  svm_struct_learn_custom.c \
  svm_struct_api.h \
  svm_struct_api_types.h \
  svm_light/svm_common.h \
  svm_struct/svm_struct_common.h

svm_struct/svm_struct_mex.o : \
  svm_struct/svm_struct_mex.c

svm_struct/svm_struct_common.o : \
  svm_struct/svm_struct_common.c \
  svm_struct/svm_struct_common.h \
  svm_light/svm_common.h \
  svm_struct_api_types.h

svm_struct/svm_struct_learn.o : \
  svm_struct/svm_struct_learn.c \
  svm_struct/svm_struct_common.h \
  svm_light/svm_common.h \
  svm_light/svm_learn.h \
  svm_struct_api_types.h \
  svm_struct_api.h

svm_struct/svm_struct_classify.o : \
  svm_struct/svm_struct_classify.c \
  svm_struct/svm_struct_common.h \
  svm_light/svm_common.h \
  svm_struct_api_types.h \
  svm_struct_api.h

# svm_light dependencies
svm_light/svm_learn.o : \
  svm_light/svm_learn.c \
  svm_light/svm_learn.h \
  svm_light/svm_common.h

svm_light/svm_common.o : \
  svm_light/svm_common.c \
  svm_light/svm_common.h \
  svm_light/kernel.h

svm_light/svm_hideo.o : \
  svm_light/svm_hideo.c

