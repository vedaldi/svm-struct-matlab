/***********************************************************************/
/*                                                                     */
/*   svm_struct_api.c                                                  */
/*                                                                     */
/*   Definition of API for attaching implementing SVM learning of      */
/*   structures (e.g. parsing, multi-label classification, HMM)        */
/*                                                                     */
/*   Author: Thorsten Joachims                                         */
/*   Date: 03.07.04                                                    */
/*                                                                     */
/*   Copyright (c) 2004  Thorsten Joachims - All rights reserved       */
/*                                                                     */
/*   This software is available for non-commercial use only. It must   */
/*   not be modified and distributed without prior permission of the   */
/*   author. The author is not responsible for implications from the   */
/*   use of this software.                                             */
/*                                                                     */
/***********************************************************************/

#include <stdio.h>
#include <string.h>
#include "svm_struct/svm_struct_common.h"
#include "svm_struct_api.h"

/** ------------------------------------------------------------------
 ** @brief
 **
 ** Called in learning part before anything else is done to allow any
 ** initializations that might be necessary.
 **/

void
svm_struct_learn_api_init(int argc, char* argv[])
{ }

/** ------------------------------------------------------------------
 ** @brief
 **
 ** Called in learning part at the very end to allow any clean-up
 ** that might be necessary.
 **/

void
svm_struct_learn_api_exit()
{ }

/** ------------------------------------------------------------------
 ** @brief
 **
 ** Called in prediction part before anything else is done to allow
 ** any initializations that might be necessary.
 **/

void
svm_struct_classify_api_init (int argc, char* argv[])
{ }

/** ------------------------------------------------------------------
 ** @brief
 **
 ** Called in prediction part at the very end to allow any clean-up
 ** that might be necessary.
 **/

void
svm_struct_classify_api_exit()
{ }

/** ------------------------------------------------------------------
 ** @brief Initialize structured model
 **
 ** Initialize structmodel sm. The weight vector w does not need to be
 ** initialized, but you need to provide the maximum size of the
 ** feature space in sizePsi. This is the maximum number of different
 ** weights that can be learned.  Later, the weight vector w will
 ** contain the learned weights for the model.
 **/

void
init_struct_model (SAMPLE sample, STRUCTMODEL *sm,
                   STRUCT_LEARN_PARM *sparm, LEARN_PARM *lparm,
                   KERNEL_PARM *kparm)
{
  if (kparm->kernel_type == LINEAR) {
    mxArray const * sizePsi_array = mxGetField(sparm->mex, 0, "dimension") ;
    if (! sizePsi_array) {
        mexErrMsgTxt("Field PARM.DIMENSION not found") ;
    }
    if (! uIsRealScalar(sizePsi_array)) {
      mexErrMsgTxt("PARM.DIMENSION must be a scalar") ;
    }

    sm->sizePsi = *mxGetPr(sizePsi_array) ;
    if (sm->sizePsi < 1) {
      mexErrMsgTxt("PARM.DIMENSION must be not smaller than 1") ;
    }
  } else {
    sm -> sizePsi = 0 ;
  }
}

/** ------------------------------------------------------------------
 ** @brief Initialize structred model constraints
 **
 ** Initializes the optimization problem. Typically, you do not need
 ** to change this function, since you want to start with an empty set
 ** of constraints. However, if for example you have constraints that
 ** certain weights need to be positive, you might put that in
 ** here. The constraints are represented as lhs[i]*w >= rhs[i]. lhs
 ** is an array of feature vectors, rhs is an array of doubles. m is
 ** the number of constraints. The function returns the initial set of
 ** constraints.
 **/

CONSTSET
init_struct_constraints (SAMPLE sample, STRUCTMODEL *sm,
                         STRUCT_LEARN_PARM *sparm)
{

  CONSTSET c;
  long     sizePsi=sm->sizePsi;
  long     i;
  WORD     words[2];

  if(1) { /* normal case: start with empty set of constraints */
    c.lhs=NULL;
    c.rhs=NULL;
    c.m=0;
  }
  else { /* add constraints so that all learned weights are
            positive. WARNING: Currently, they are positive only up to
            precision epsilon set by -e. */
    c.lhs=my_malloc(sizeof(DOC *)*sizePsi);
    c.rhs=my_malloc(sizeof(double)*sizePsi);
    for(i=0; i<sizePsi; i++) {
      words[0].wnum=i+1;
      words[0].weight=1.0;
      words[1].wnum=0;
      /* the following slackid is a hack. we will run into problems,
         if we have move than 1000000 slack sets (ie examples) */
      c.lhs[i]=create_example(i,0,1000000+i,1,create_svector(words,NULL,1.0));
      c.rhs[i]=0.0;
    }
  }
  return(c);
}

/** ------------------------------------------------------------------
 ** @brief Predict a structured label given a pattern
 **
 ** Finds the label yhat for pattern x that scores the highest
 ** according to the linear evaluation function in sm, especially the
 ** weights sm.w. The returned label is taken as the prediction of sm
 ** for the pattern x. The weights correspond to the features defined
 ** by psi() and range from index 1 to index sm->sizePsi. If the
 ** function cannot find a label, it shall return an empty label as
 ** recognized by the function empty_label(y).
 **/

LABEL
classify_struct_example (PATTERN x,
                         STRUCTMODEL *sm,
                         STRUCT_LEARN_PARM *sparm)
{
  LABEL y ;

  mxArray* fn_array ;
  mxArray* w_array ;
  mxArray* args [4] ;
  int status ;

  fn_array= mxGetField(sparm->mex, 0, "classifyFn") ;
  if (! fn_array) {
      mexErrMsgTxt("Field PARM.CLASSIFYFN not found") ;
  }
  if (! mxGetClassID(fn_array) == mxFUNCTION_CLASS) {
    mexErrMsgTxt("PARM.CLASSIFYFN must be a valid function handle") ;
  }

  /* encapsulate sm->w into a Matlab array */
  w_array = mxCreateDoubleMatrix(sm->sizePsi, 1, mxREAL) ;
  memcpy(mxGetPr(w_array),
         sm->w + 1,
         sm->sizePsi * sizeof(double)) ;

  /* evaluate Matlab callback */
  args[0] = fn_array ;
  args[1] = (mxArray*) sparm->mex ; /* model (discard conts) */
  args[2] = w_array ;
  args[3] = x.mex ;

  status = mexCallMATLAB(1, &y.mex, 4, args, "feval") ;

  mxDestroyArray(w_array) ;

  if (status) {
    mexErrMsgTxt("Error while executing PARM.CLASSIFYFN") ;
  }
  if (mxGetClassID(y.mex) == mxUNKNOWN_CLASS) {
    mexErrMsgTxt("PARM.CLASSIFYFN did not reutrn a result") ;
  }

  /* flag this label has one tha should be freed when discared */
  y.isOwner = 1 ;

  return (y) ;
}

/** ------------------------------------------------------------------
 ** @brief Find the most violated constraint with slack rescaling
 **
 ** Finds the label ybar for pattern x that that is responsible for
 ** the most violated constraint for the slack rescaling
 ** formulation. For linear slack variables, this is that label ybar
 ** that maximizes
 **
 **   argmax_{ybar} loss(y,ybar)*(1-psi(x,y)+psi(x,ybar))
 **
 ** Note that ybar may be equal to y (i.e. the max is 0), which is
 ** different from the algorithms described in
 ** [Tschantaridis/05]. Note that this argmax has to take into account
 ** the scoring function in sm, especially the weights sm.w, as well
 ** as the loss function, and whether linear or quadratic slacks are
 ** used. The weights in sm.w correspond to the features defined by
 ** psi() and range from index 1 to index sm->sizePsi. Most simple is
 ** the case of the zero/one loss function. For the zero/one loss,
 ** this function should return the highest scoring label ybar (which
 ** may be equal to the correct label y), or the second highest
 ** scoring label ybar, if Psi(x,ybar)>Psi(x,y)-1. If the function
 ** cannot find a label, it shall return an empty label as recognized
 ** by the function empty_label(y).
 **/

LABEL
find_most_violated_constraint_slackrescaling (PATTERN x, LABEL y,
                                              STRUCTMODEL *sm,
                                              STRUCT_LEARN_PARM *sparm)
{
  LABEL ybar ;

  mxArray* fn_array ;
  mxArray* model_array ;
  mxArray* args [5] ;
  int status ;

  fn_array = mxGetField(sparm->mex, 0, "constraintFn") ;
  if (! fn_array) {
    mexErrMsgTxt("Field PARM.CONSTRAINTFN not found") ;
  }
  if (! mxGetClassID(fn_array) == mxFUNCTION_CLASS) {
    mexErrMsgTxt("PARM.CONSTRAINTFN must be a valid function handle") ;
  }

  /* encapsulate sm->w into a Matlab array */
  model_array = newMxArrayEncapsulatingSmodel (sm) ;

  args[0] = fn_array ;
  args[1] = (mxArray*) sparm->mex ; /* model (discard conts) */
  args[2] = model_array ;
  args[3] = x.mex ;
  args[4] = y.mex ;

  /* Apparently we must make sure that model_array is not destoryed by
     the automatic cleaning up function. It seems that, because it is constructed by
     encapsulating input argument to the MEX file, it must be considered an hybrid
     array, i.e. an array which cannot be destroyed by MATLAB automatic
     cleanup system, invoked when an error is raised */

  mexSetTrapFlag (1) ;
  status = mexCallMATLAB(1, &ybar.mex, 5, args, "feval") ;
  mexSetTrapFlag (0) ;

  destroyMxArrayEncapsulatingSmodel (model_array) ;

  if (status) {
    mxArray * error_array ;
    mexCallMATLAB(1, &error_array, 0, NULL, "lasterror") ;
    mexCallMATLAB(0, NULL, 1, &error_array, "disp") ;
    mexCallMATLAB(0, NULL, 1, &error_array, "rethrow") ;
  }
  if (mxGetClassID(ybar.mex) == mxUNKNOWN_CLASS) {
    mexErrMsgTxt("PARM.CONSTRAINTFN did not reutrn a result") ;
  }
  ybar.isOwner = 1 ;

  return(ybar);
}

/** ------------------------------------------------------------------
 ** @brief Find the most violated constraint with margin rescaling
 ** Finds the label ybar for pattern x that that is responsible for
 ** the most violated constraint for the margin rescaling
 ** formulation. For linear slack variables, this is that label ybar
 ** that maximizes
 **
 **           argmax_{ybar} loss(y,ybar)+psi(x,ybar)
 **
 ** Note that ybar may be equal to y (i.e. the max is 0), which is
 ** different from the algorithms described in
 ** [Tschantaridis/05]. Note that this argmax has to take into account
 ** the scoring function in sm, especially the weights sm.w, as well
 ** as the loss function, and whether linear or quadratic slacks are
 ** used. The weights in sm.w correspond to the features defined by
 ** psi() and range from index 1 to index sm->sizePsi. Most simple is
 ** the case of the zero/one loss function. For the zero/one loss,
 ** this function should return the highest scoring label ybar (which
 ** may be equal to the correct label y), or the second highest
 ** scoring label ybar, if Psi(x,ybar)>Psi(x,y)-1. If the function
 ** cannot find a label, it shall return an empty label as recognized
 ** by the function empty_label(y).
 **/

LABEL
find_most_violated_constraint_marginrescaling (PATTERN x, LABEL y,
                                               STRUCTMODEL *sm,
                                               STRUCT_LEARN_PARM *sparm)
{
  LABEL ybar ;
  mxArray* fn_array ;
  mxArray* model_array ;
  mxArray* args [5] ;
  int status ;

  fn_array = mxGetField(sparm->mex, 0, "constraintFn") ;
  if (! fn_array) {
    mexErrMsgTxt("Field PARM.CONSTRAINTFN not found") ;
  }
  if (! mxGetClassID(fn_array) == mxFUNCTION_CLASS) {
    mexErrMsgTxt("PARM.CONSTRAINTFN is not a valid function handle") ;
  }

  /* encapsulate sm->w into a Matlab array */
  model_array = newMxArrayEncapsulatingSmodel (sm) ;

  args[0] = fn_array ;
  args[1] = (mxArray*) sparm->mex ; /* model (discard conts) */
  args[2] = model_array ;
  args[3] = x.mex ;
  args[4] = y.mex ;

  mexSetTrapFlag (1) ;
  status = mexCallMATLAB(1, &ybar.mex, 5, args, "feval") ;
  mexSetTrapFlag (0) ;

  destroyMxArrayEncapsulatingSmodel (model_array) ;

  if (status) {
    mxArray * error_array ;
    mexCallMATLAB(1, &error_array, 0, NULL, "lasterror") ;
    mexCallMATLAB(0, NULL, 1, &error_array, "error") ;
  }
  if (mxGetClassID(ybar.mex) == mxUNKNOWN_CLASS) {
    mexErrMsgTxt("PARM.CONSTRAINTFN did not reutrn a result") ;
  }
  ybar.isOwner = 1 ;

  return (ybar) ;
}

/** ------------------------------------------------------------------
 ** @brief Is the label empty?
 **
 ** Returns true, if y is an empty label. An empty label might be
 ** returned by find_most_violated_constraint_???(x, y, sm) if there
 ** is no incorrect label that can be found for x, or if it is unable
 ** to label x at all.
 **/

int
empty_label (LABEL y)
{
  return (y.mex == NULL) ;
}

/** ------------------------------------------------------------------
 ** @brief Evaluate Psi(x, y)
 **
 ** Returns a feature vector describing the match between pattern x
 ** and label y. The feature vector is returned as a list of
 ** SVECTOR's. Each SVECTOR is in a sparse representation of pairs
 ** <featurenumber:featurevalue>, where the last pair has
 ** featurenumber 0 as a terminator. Featurenumbers start with 1 and
 ** end with sizePsi. Featuresnumbers that are not specified default
 ** to value 0. As mentioned before, psi() actually returns a list of
 ** SVECTOR's. Each SVECTOR has a field 'factor' and 'next'. 'next'
 ** specifies the next element in the list, terminated by a NULL
 ** pointer. The list can be though of as a linear combination of
 ** vectors, where each vector is weighted by its 'factor'. This
 ** linear combination of feature vectors is multiplied with the
 ** learned (kernelized) weight vector to score label y for pattern
 ** x. Without kernels, there will be one weight in sm.w for each
 ** feature. Note that psi has to match
 ** find_most_violated_constraint_???(x, y, sm) and vice versa. In
 ** particular, find_most_violated_constraint_???(x, y, sm) finds that
 ** ybar!=y that maximizes psi(x,ybar,sm)*sm.w (where * is the inner
 ** vector product) and the appropriate function of the loss +
 ** margin/slack rescaling method. See that paper for details.
 **/

SVECTOR *
psi (PATTERN x, LABEL y, STRUCTMODEL *sm,
      STRUCT_LEARN_PARM *sparm)
{
  SVECTOR *sv = NULL;

  /* The algorith can use either a linear kernel (explicit feature map)
   * or a custom kernel (implicit feature map). For the explicit feature
   * map, this function returns a  sizePhi-dimensional vector. For
   * the implicit feature map this function returns a placeholder
   */

  if (sm -> svm_model -> kernel_parm .kernel_type == LINEAR) {
    /* For the linear kernel computes the vector Phi(x,y) */
    mxArray* out ;
    mxArray* fn_array ;
    mxArray* args [4] ;
    WORD* words = NULL ;
    double twonorm_sq = 0 ;
    int status ;

    fn_array = mxGetField(sparm->mex, 0, "featureFn") ;
    if (! fn_array) {
      mexErrMsgTxt("Field PARM.FEATUREFN not found") ;
    }
    if (! mxGetClassID(fn_array) == mxFUNCTION_CLASS) {
      mexErrMsgTxt("PARM.FEATUREFN must be a valid function handle") ;
    }

    args[0] = fn_array ;
    args[1] = (mxArray*) sparm->mex ; /* model (discard conts) */
    args[2] = x.mex ;                 /* pattern */
    args[3] = y.mex ;                 /* label */
    status = mexCallMATLAB(1, &out, 4, args, "feval") ;

    if (status) {
      mexErrMsgTxt("Error while executing PARM.FEATUREFN") ;
    }
    if (mxGetClassID(out) == mxUNKNOWN_CLASS) {
      mexErrMsgTxt("PARM.FEATUREFN must reutrn a result") ;
    }

    if (! mxIsSparse(out) ||
        ! mxGetClassID(out) == mxDOUBLE_CLASS ||
        ! mxGetN(out) == 1 ||
        ! mxGetM(out) == sm->sizePsi) {
      mexErrMsgTxt("PARM.FEATUREFN must return a sparse column vector "
                   "of the prescribed size") ;
    }

    {
      double * data = mxGetPr(out) ;
      int i ;
      mwIndex * colOffsets = mxGetJc(out) ;
      mwIndex * rowIndexes = mxGetIr(out) ;
      int numNZ = colOffsets[1] - colOffsets[0] ;

      words = (WORD*) my_malloc (sizeof(WORD) * (numNZ + 1)) ;

      for (i = 0 ; i < numNZ ; ++ i) {
        words[i].wnum = rowIndexes[i] + 1 ;
        words[i].weight = data[i] ;
        twonorm_sq += data[i] * data[i] ;
      }
      words[numNZ].wnum = 0 ;
      words[numNZ].weight = 0 ;
    }

    sv = create_svector_shallow (words, NULL, 1.0) ;
    sv->twonorm_sq = twonorm_sq ;

    mxDestroyArray (out) ;
  }
  else {
    /* For the ustom kernel returns a placeholder for (x,y). */
    MexPhiCustom phi = newMexPhiCustomFromPatternLabel(x.mex, y.mex) ;
    WORD * words = mxMalloc(sizeof(WORD)) ;
    words[0].wnum = 0 ;
    words[0].weight = 0 ;
    sv = create_svector_shallow(words, phi, 1.0) ;
  }

  return (sv) ;
}

/** ------------------------------------------------------------------
 ** @brief Evaluate loss function Delta(y, ybar)
 **/

double
loss (LABEL y, LABEL ybar, STRUCT_LEARN_PARM *sparm)
{

  double loss_value ;
  mxArray* fn_array ;
  mxArray* out ;
  mxArray* args [4] ;
  int status ;

  fn_array = mxGetField(sparm->mex, 0, "lossFn") ;
  if (! fn_array) {
    mexErrMsgTxt("Field PARM.LOSSFN not found") ;
  }
  if (! mxGetClassID(fn_array) == mxFUNCTION_CLASS) {
    mexErrMsgTxt("PARM.LOSSFN must be a valid function handle") ;
  }

  args[0] = fn_array ;
  args[1] = (mxArray*) sparm->mex ; /* model (discard conts) */
  args[2] = y.mex ;
  args[3] = ybar.mex ;

  status = mexCallMATLAB (1, &out, 4, args, "feval") ;

  if (status) {
    mexErrMsgTxt("Error while executing PARM.LOSSFN") ;
  }
  if (! uIsRealScalar(out)) {
    mexErrMsgTxt("PARM.LOSSFN must reutrn a scalar") ;
  }

  loss_value = *mxGetPr(out) ;
  mxDestroyArray(out) ;

  return (loss_value) ;
}

/** ------------------------------------------------------------------
 ** This function is called just before the end of each cutting plane
 ** iteration. ceps is the amount by which the most violated
 ** constraint found in the current iteration was
 ** violated. cached_constraint is true if the added constraint was
 ** constructed from the cache. If the return value is FALSE, then the
 ** algorithm is allowed to terminate. If it is TRUE, the algorithm
 ** will keep iterating even if the desired precision sparm->epsilon
 ** is already reached.
 **/

int
finalize_iteration (double ceps, int cached_constraint,
                    SAMPLE sample, STRUCTMODEL *sm,
                    CONSTSET cset, double *alpha,
                    STRUCT_LEARN_PARM *sparm)
{
  mxArray* fn_array ;
  mxArray* model_array ;
  mxArray* out ;
  mxArray* args [3] ;
  int status ;
  int result = 0 ;

  fn_array = mxGetField(sparm->mex, 0, "endIterationFn") ;

  if (! fn_array) return 0 ;
  if (! mxGetClassID(fn_array) == mxFUNCTION_CLASS) {
    mexErrMsgTxt("PARM.ENDITERATIONFN must be a valid function handle") ;
  }

  /* encapsulate sm->w into a Matlab array */
  model_array = newMxArrayEncapsulatingSmodel (sm) ;

  args[0] = fn_array ;
  args[1] = (mxArray*) sparm->mex ; /* model (discard conts) */
  args[2] = model_array ;

  status = mexCallMATLAB (1, &out, 3, args, "feval") ;

  destroyMxArrayEncapsulatingSmodel (model_array) ;

  if (status) {
    mexErrMsgTxt("Error while executing PARM.LOSSFN") ;
  }

  if (! uIsLogicalScalar(out)) {
    mexErrMsgTxt("PARM.ENDITERATIONFN must reutrn nothing or a scalar") ;
  }
  result = (int) (*mxGetLogicals(out)) ;
  mxDestroyArray(out) ;
  return result ;
}

/** ------------------------------------------------------------------
 ** This function is called after training and allows final touches to
 ** the model sm. But primarly it allows computing and printing any
 ** kind of statistic (e.g. training error) you might want.
 **/

void
print_struct_learning_stats (SAMPLE sample, STRUCTMODEL *sm,
                             CONSTSET cset, double *alpha,
                             STRUCT_LEARN_PARM *sparm)
{

}

/** ------------------------------------------------------------------
 ** This function is called after making all test predictions in
 ** svm_struct_classify and allows computing and printing any kind of
 ** evaluation (e.g. precision/recall) you might want. You can use the
 ** function eval_prediction to accumulate the necessary statistics for
 ** each prediction.
 **/

void
print_struct_testing_stats (SAMPLE sample, STRUCTMODEL *sm,
                            STRUCT_LEARN_PARM *sparm,
                            STRUCT_TEST_STATS *teststats)
{

}

/** ------------------------------------------------------------------
 ** This function allows you to accumlate statistic for how well the
 ** predicition matches the labeled example. It is called from
 ** svm_struct_classify. See also the function
 ** print_struct_testing_stats.
 **/

void
eval_prediction (long exnum, EXAMPLE ex, LABEL ypred,
                 STRUCTMODEL *sm, STRUCT_LEARN_PARM *sparm,
                 STRUCT_TEST_STATS *teststats)
{
  if(exnum == 0) { /* this is the first time the function is
		      called. So initialize the teststats */
  }
}

/** ------------------------------------------------------------------
 ** Frees the memory of x.
 **/

void
free_pattern (PATTERN x)
{ }

/** ------------------------------------------------------------------
 ** Frees the memory of y.
 **/

void
free_label (LABEL y)
{
  if (y.isOwner && y.mex) {
    mxDestroyArray(y.mex) ;
  }
}

/** ------------------------------------------------------------------
 ** Frees the memory of model.
 **/

void
free_struct_model (STRUCTMODEL sm)
{
  if(sm.svm_model) free_model(sm.svm_model, 1 );
  /* add free calls for user defined data here */
}

/** ------------------------------------------------------------------
 ** Frees the memory of a sample.
 **/

void
free_struct_sample (SAMPLE s)
{
  int i;
  for(i=0;i<s.n;i++) {
    free_pattern(s.examples[i].x);
    free_label(s.examples[i].y);
  }
  free(s.examples);
}

/** ------------------------------------------------------------------
 ** Prints a help text that is appended to the common help text of
 ** svm_struct_learn().
 **/

void
print_struct_help ()
{
  printf("         --* string  -> custom parameters that can be adapted for struct\n");
  printf("                        learning. The * can be replaced by any character\n");
  printf("                        and there can be multiple options starting with --.\n");
}

/** ------------------------------------------------------------------
 ** Parses the command line parameters that start with --
 **/

void
parse_struct_parameters(STRUCT_LEARN_PARM *sparm)
{

  int i;

  for(i=0;(i<sparm->custom_argc) && ((sparm->custom_argv[i])[0] == '-');i++) {
    switch ((sparm->custom_argv[i])[2]) {
    case 'a': i++; /* strcpy(learn_parm->alphafile,argv[i]); */ break;
    case 'e': i++; /* sparm->epsilon=atof(sparm->custom_argv[i]); */ break;
    case 'k': i++; /* sparm->newconstretrain=atol(sparm->custom_argv[i]); */ break;
    default: printf("\nUnrecognized option %s!\n\n",sparm->custom_argv[i]);
      exit(0);
    }
  }
}

/** ------------------------------------------------------------------
 ** Prints a help text that is appended to the common help text of
 ** svm_struct_classify.
 **/

void
print_struct_help_classify()
{
  printf("         --* string -> custom parameters that can be adapted for struct\n");
  printf("                       learning. The * can be replaced by any character\n");
  printf("                       and there can be multiple options starting with --.\n");
}

/** ------------------------------------------------------------------
 ** Parses the command line parameters that start with -- for the
 ** classification module.
 **/

void
parse_struct_parameters_classify (STRUCT_LEARN_PARM *sparm)
{
  int i;

  for(i=0;(i<sparm->custom_argc) && ((sparm->custom_argv[i])[0] == '-');i++) {
    switch ((sparm->custom_argv[i])[2]) {
      /* case 'x': i++; strcpy(xvalue,sparm->custom_argv[i]); break; */
    default: printf("\nUnrecognized option %s!\n\n",sparm->custom_argv[i]);
      exit(0);
    }
  }
}

