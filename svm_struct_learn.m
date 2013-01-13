% SVM_STRUCT_LEARN  Calls the SVM-struct solver
%   MODEL = SVM_STRUCT_LEARN(ARGS, PARM) runs SVM-struct solver with
%   parameters ARGS on the problem PARM. See [1-6] for the
%   theory. SPARM is a structure of with the fields
%
%     PATTERNS:: patterns (X)
%       A cell array of patterns. The entries can have any nature
%       (they can just be indexes of the actual data for example).
%
%     LABELS:: labels (Y)
%       A cell array of labels. The entries can have any nature.
%
%     LOSSFN:: loss function callback
%       A handle to the loss function. This function has the form
%       L = LOSS(PARAM, Y, YBAR) where PARAM is the SPARM structure,
%       Y a ground truth label, YBAR another label, and L a
%       non-negative scalar.
%
%     CONSTRAINTFN:: constraint callback
%       A handle to the constraint generation function. This function
%       has the form YBAR = FUNC(PARAM, MODEL, X, Y) where PARAM is
%       the input PARM structure, MODEL is the a structure
%       representing the current model, X is an input pattern, and Y
%       is its ground truth label. YBAR is the most violated labels.
%
%     FEATUREN:: feature map callback
%       A handle to the feature map. This function has the form PSI =
%       FEATURE(PARAM, X, Y) where PARAM is the input PARM structure,
%       X is a pattern, Y is a label, and PSI a sparse vector of
%       dimension PARM.DIMENSION. This handle does not need to be
%       specified if kernels are used.
%
%     ENDITERATIONFN:: end iteration callback
%       The optional callback CONTINUE = ENDITERATIONFN(PARAM, MODEL)
%       is called at the end of each cutting plane iteration. This can
%       be used to display diagnostic information. The callback should
%       return a logcial value, usually equal to FALSE. If the value
%       is TRUE, then the algorithm keeps iterating even if the
%       convergence criterion has been satisfied.
%
%     DIMENSION:: dimension of the feature map
%       The dimension of the feature map. This value does not need to
%       be specified i kernels are used.
%
%     KERNELFN:: kernel function callback
%       A handle to the kernel function. This function has the form K
%       = KERN(PARAM, X, Y, XP, YP) where PARAM is the input PARM
%       structure, and X, Y and XP, YP are two pattern-label pairs,
%       input of the joint kernel. This handle does not need to be
%       specified if feature maps are used.
%
%   MODEL is a structure with fields:
%
%     W:: weight vector
%       This is a spare vector of size PARAM.DIMENSION. It is used
%       with feature maps.
%
%     ALPHA:: dual variables
%     SVPATTERNS:: patterns which are support vectors
%     SVLABELS:: labels which are support vectors
%       Used with kernels.
%
%   ARGS is a string specifying options in the usual struct
%   SVM. These are:
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
%           -l [0..]    -> Loss function to use.
%                          0: zero/one loss
%                          ?: see below in application specific options
%
%   Optimization Options (see [2][5])::
%           -w [0,..,9] -> choice of structural learning algorithm
%                          0: n-slack algorithm described in [2]
%                          1: n-slack algorithm with shrinking heuristic
%                          2: 1-slack algorithm (primal) described in [5]
%                          3: 1-slack algorithm (dual) described in [5]
%                          4: 1-slack algorithm (dual) with constraint cache [5]
%                          9: custom algorithm in svm_struct_learn_custom.c
%           -e float    -> epsilon: allow that tolerance for termination
%                          criterion
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
%    [1] T. Joachims, Learning to Align Sequences: A Maximum Margin Approach.
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
%    [6] http://svmlight.joachims.org/

%  Authors:: Andrea Vedaldi (MATLAB MEX version)
