                        SVM^struct for MATLAB
                                 V1.0
                            Andrea Vedaldi

This is a patch to Thorsten Joachims (http://www.joachims.org/)
SVM-struct implementation that provides a simple MATLAB interface.

COMPILING:

To compile this software use the included Makefile. It should work out
of the box on Linux and Mac OS X provied that MATLAB MEX program is in
the command line path. So just issue

> make

The only files that are needed to run the package with MATLAB are
svm_struct_learn.mex* (MEX program) and svm_struct_learn.m
(documentation).

If MEX is not on the command line, the path can be specified as

> make MEX=<MATLABROOT>/bin/mex

where <MATLABROOT> is MATLAB root directory. Finally, it is also
possible to specify manually the architecture

> make ARCH=maci     # Mac Intel 32 bit
> make ARCH=maci64   # Mac Intel 64 bit
> make ARCH=glnx86   # Linux 32 bit
> make ARCH=glnxa64  # Linux 64 bit

To clean the build products use

> make clean     # clean all build but the MEX file
> make distclean # clean all build products

USAGE:

There is only one MATLAB command, i.e. SVM_STRUCT_LEARN. This commands
take as input handles to the function implementing the maximal
constriant violation search et simila. See TEST_SVM_STRUCT_LEARN and
TEST_SVM_STRUCT_LEARN_KER for example usage and SVM_STRUCT_LEARN
builtin help for further informatinon.

