                        SVM^struct for MATLAB
                                 V1.0
                            Andrea Vedaldi

This is a patch to Thorsten Joachims (http://www.joachims.org/)
SVM-struct implementation that provides a simple-to-use MATLAB
interface.

COMPILING:

To compile this software use the included Makefile. It should work out
of the box on Linux and Mac OS X provided that MATLAB MEX program is
in the command line path. So just issue

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
constraint violation search, the loss, and the feature map. See
TEST_SVM_STRUCT_LEARN and TEST_SVM_STRUCT_LEARN_KER for example usage
and SVM_STRUCT_LEARN built-in help for further information.

LICENSE:

The MATLAB interface to T. Joachim's SVM^struct is distributed under
the following license.  This license covers only the additions to
T. Joachims code that constitute the MATLAB interface (the "Software")
and does not cover SVM^struct itself.

Copyright (C) 2011 by Andrea Vedaldi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

