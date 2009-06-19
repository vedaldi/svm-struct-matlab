% SVM_STRUCT_LEARN
%   W = SVM_STRUCT_LEARN(ARGS, SPARM)
%
%   ARGS is a string specifying options in the usual struct
%   SVM. These are:
%
%           example_file-> file with training data
%           model_file  -> file to store learned decision rule in
%
%   General Options::
%           -v [0..3]   -> verbosity level (default 1)
%           -y [0..3]   -> verbosity level for svm_light (default 0)
%
%   Learning Options::
%           -c float    -> C: trade-off between training error
%                          and margin (default 0.01)
%           -p [1,2]    -> L-norm to use for slack variables. Use 1 for L1-norm,
%                          use 2 for squared slacks. (default 1)
%           -o [1,2]    -> Rescaling method to use for loss.
%                          1: slack rescaling
%                          2: margin rescaling
%                          (default %d)",DEFAULT_RESCALING);
%           -l [0..]    -> Loss function to use.
%                          0: zero/one loss
%                          ?: see below in application specific options
%                          (default %d)",DEFAULT_LOSS_FCT);
%
%   Optimization Options (see [2][5])::
%           -w [0,..,9] -> choice of structural learning algorithm (default %d):",(int)DEFAULT_ALG_TYPE);
%                          0: n-slack algorithm described in [2]
%                          1: n-slack algorithm with shrinking heuristic
%                          2: 1-slack algorithm (primal) described in [5]
%                          3: 1-slack algorithm (dual) described in [5]
%                          4: 1-slack algorithm (dual) with constraint cache [5]
%                          9: custom algorithm in svm_struct_learn_custom.c
%           -e float    -> epsilon: allow that tolerance for termination
%                          criterion (default %f)",DEFAULT_EPS);
%           -k [1..]    -> number of new constraints to accumulate before
%                          recomputing the QP solution (default 100) (-w 0 and 1 only)
%           -f [5..]    -> number of constraints to cache for each example
%                          (default 5) (used with -w 4)
%           -b [1..100] -> percentage of training set for which to refresh cache
%                          when no epsilon violated constraint can be constructed
%                          from current cache (default 100%%) (used with -w 4)
%
%  SVM-light Options for Solving QP Subproblems (see [3])::
%           -n [2..q]   -> number of new variables entering the working set
%                          in each svm-light iteration (default n = q).
%                          Set n < q to prevent zig-zagging.
%           -m [5..]    -> size of svm-light cache for kernel evaluations in MB
%                          (default 40) (used only for -w 1 with kernels)
%           -h [5..]    -> number of svm-light iterations a variable needs to be
%                          optimal before considered for shrinking (default 100)
%           -# int      -> terminate svm-light QP subproblem optimization, if no
%                          progress after this number of iterations.
%                          (default 100000)
%
%  Kernel Options::
%           -t int      -> type of kernel function:
%                          0: linear (default)
%                          1: polynomial (s a*b+c)^d
%                          2: radial basis function exp(-gamma ||a-b||^2)
%                          3: sigmoid tanh(s a*b + c)
%                          4: user defined kernel from kernel.h
%           -d int      -> parameter d in polynomial kernel
%           -g float    -> parameter gamma in rbf kernel
%           -s float    -> parameter s in sigmoid/poly kernel
%           -r float    -> parameter c in sigmoid/poly kernel
%           -u string   -> parameter of user defined kernel
%
%  Output Options::
%           -a string   -> write all alphas to this file after learning
%                          (in the same order as in the training set)
%  References::
%    [1] T. Joachims, Learning to Align Sequences: A Maximum Margin Aproach.
%        Technical Report, September, 2003.
%    [2] I. Tsochantaridis, T. Joachims, T. Hofmann, and Y. Altun, Large Margin
%        Methods for Structured and Interdependent Output Variables, Journal
%        of Machine Learning Research (JMLR), Vol. 6(Sep):1453-1484, 2005.
%    [3] T. Joachims, Making Large-Scale SVM Learning Practical. Advances in
%        Kernel Methods - Support Vector Learning, B. Schölkopf and C. Burges and
%        A. Smola (ed.), MIT Press, 1999.
%    [4] T. Joachims, Learning to Classify Text Using Support Vector
%        Machines: Methods, Theory, and Algorithms. Dissertation, Kluwer,
%        2002.
%    [5] T. Joachims, T. Finley, Chun-Nam Yu, Cutting-Plane Training of Structural
%        SVMs, Machine Learning Journal, to appear.
%
%  Authors:: Andrea Vedaldi (MEX version)
