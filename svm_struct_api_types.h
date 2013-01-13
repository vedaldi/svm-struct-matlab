/***********************************************************************/
/*                                                                     */
/*   svm_struct_api_types.h                                            */
/*                                                                     */
/*   Definition of API for attaching implementing SVM learning of      */
/*   structures (e.g. parsing, multi-label classification, HMM)        */
/*                                                                     */
/*   Author: Thorsten Joachims                                         */
/*   Date: 13.10.03                                                    */
/*                                                                     */
/*   Copyright (c) 2003  Thorsten Joachims - All rights reserved       */
/*                                                                     */
/*   This software is available for non-commercial use only. It must   */
/*   not be modified and distributed without prior permission of the   */
/*   author. The author is not responsible for implications from the   */
/*   use of this software.                                             */
/*                                                                     */
/***********************************************************************/

#ifndef svm_struct_api_types
#define svm_struct_api_types

# include "svm_light/svm_common.h"
# include "svm_light/svm_learn.h"

# ifndef WIN
# include "strings.h"
# else
# include <string.h>
# endif

#ifndef WIN
#define inline_comm __inline__
#else
#define inline_comm __inline
#endif


typedef struct MexKernelInfo_
{
  mxArray const * structParm ;
  mxArray const * kernelFn ;
} MexKernelInfo ;

inline_comm static int
uIsString(const mxArray* A, int L)
{
  int M = mxGetM(A) ;
  int N = mxGetN(A) ;
  return
    mxIsChar(A) &&
    mxGetNumberOfDimensions(A) == 2 &&
    (M == 1 || (M == 0 && N == 0)) &&
    (L < 0 || N == L) ;
}

inline_comm static int
uIsReal (const mxArray* A)
{
  return
    mxIsDouble(A) &&
    ! mxIsComplex(A) ;
}

inline_comm static int
uIsRealScalar(const mxArray* A)
{
  return
    uIsReal (A) && mxGetNumberOfElements(A) == 1 ;
}

inline_comm static int
uIsLogicalScalar(const mxArray* A)
{
  return
  mxIsLogical(A) && mxGetNumberOfElements(A) == 1 ;
}

inline_comm static mxArray *
newMxArrayFromDoubleVector (int n, double const* v)
{
  mxArray* array = mxCreateDoubleMatrix(n, 1, mxREAL) ;
  memcpy(mxGetPr(array), v, sizeof(double) * n) ;
  return (array) ;
}

inline_comm static mxArray *
newMxArrayFromSvector (int n, SVECTOR const* sv)
{
  WORD* wi ;
  double *pr ;
  mwSize nz = 0 ;
  mwIndex *ir, *jc ;
  mxArray* sv_array ;
  
  /* count words */
  for (wi = sv->words ; wi->wnum >= 1 ; ++ wi) nz ++ ;
  
  /* allocate sparse array */
  sv_array = mxCreateSparse(n, 1, nz, mxREAL) ;
  /*  mxSetPr(mxMalloc(sizeof(double) * nz)) ;
   mxSetIr(mxMalloc(sizeof(mwIndex) * nz)) ;
   mxSetJc(mxMalloc(sizeof(mwIndex) * 2)) ;*/
  ir = mxGetIr (sv_array) ;
  jc = mxGetJc (sv_array) ;
  pr = mxGetPr (sv_array) ;
  
  /* copy fields */
  for (wi = sv->words ; wi->wnum >= 1 ; ++ wi) {
    *pr ++  = wi -> weight ;
    *ir ++  = wi -> wnum ;
    if (wi -> wnum > n) {
      char str [512] ;
      #ifndef WIN
      snprintf(str, sizeof(str),
               "Component index %d larger than sparse vector dimension %d", 
               wi -> wnum, n) ;
      #else
      sprintf(str, sizeof(str),
               "Component index %d larger than sparse vector dimension %d",
               wi -> wnum, n) ;
      #endif
      mexErrMsgTxt(str) ;
    }
  }
  jc [0] = 0 ;
  jc [1] = nz ;
  
  return (sv_array) ;
}


# define INST_NAME          "MATLAB MEX interface"
# define INST_VERSION       "V0.1"
# define INST_VERSION_DATE  "??.??.??"

/* default precision for solving the optimization problem */
# define DEFAULT_EPS         0.1
/* default loss rescaling method: 1=slack_rescaling, 2=margin_rescaling */
# define DEFAULT_RESCALING   2
/* default loss function: */
# define DEFAULT_LOSS_FCT    0
/* default optimization algorithm to use: */
# define DEFAULT_ALG_TYPE    3
/* store Psi(x,y) (for ALG_TYPE 1) instead of recomputing it every time: */
# define USE_FYCACHE         1
/* decide whether to evaluate sum before storing vectors in constraint
   cache:
   0 = NO,
   1 = YES (best, if sparse vectors and long vector lists),
   2 = YES (best, if short vector lists),
   3 = YES (best, if dense vectors and long vector lists) */
# define COMPACT_CACHED_VECTORS 1
/* minimum absolute value below which values in sparse vectors are
   rounded to zero. Values are stored in the FVAL type defined in svm_common.h
   RECOMMENDATION: assuming you use FVAL=float, use
     10E-15 if COMPACT_CACHED_VECTORS is 1
     10E-10 if COMPACT_CACHED_VECTORS is 2 or 3
*/
# define COMPACT_ROUNDING_THRESH 10E-15

typedef struct pattern {
  /* this defines the x-part of a training example, e.g. the structure
     for storing a natural language sentence in NLP parsing */
  mxArray* mex ;
} PATTERN;

typedef struct label {
  /* this defines the y-part (the label) of a training example,
     e.g. the parse tree of the corresponding sentence. */
  mxArray* mex ;
  int isOwner ;
} LABEL;

typedef struct structmodel {
  double *w;          /* pointer to the learned weights */
  MODEL  *svm_model;  /* the learned SVM model */
  long   sizePsi;     /* maximum number of weights in w */
  double walpha;
  /* other information that is needed for the stuctural model can be
     added here, e.g. the grammar rules for NLP parsing */
} STRUCTMODEL;

typedef struct struct_learn_parm {
  double epsilon;              /* precision for which to solve
				  quadratic program */
  double newconstretrain;      /* number of new constraints to
				  accumulate before recomputing the QP
				  solution (used in w=1 algorithm) */
  int    ccache_size;          /* maximum number of constraints to
				  cache for each example (used in w=4
				  algorithm) */
  double batch_size;           /* size of the mini batches in percent
				  of training set size (used in w=4
				  algorithm) */
  double C;                    /* trade-off between margin and loss */
  char   custom_argv[50][300]; /* storage for the --* command line options */
  int    custom_argc;          /* number of --* command line options */
  int    slack_norm;           /* norm to use in objective function
                                  for slack variables; 1 -> L1-norm,
				  2 -> L2-norm */
  int    loss_type;            /* selected loss type from -r
				  command line option. Select between
				  slack rescaling (1) and margin
				  rescaling (2) */
  int    loss_function;        /* select between different loss
				  functions via -l command line
				  option */
  /* further parameters that are passed to init_struct_model() */
  mxArray const * mex ;
} STRUCT_LEARN_PARM ;

typedef struct struct_test_stats {
#ifdef WIN
 int dum;
#endif
  /* you can add variables for keeping statistics when evaluating the
     test predictions in svm_struct_classify. This can be used in the
     function eval_prediction and print_struct_testing_stats. */
} STRUCT_TEST_STATS;

inline_comm static mxArray *
newMxArrayEncapsulatingDoubleVector (int n, double * v)
{
#if 1
  mxArray * v_array = mxCreateDoubleMatrix (0, 0, mxREAL) ;
  mxSetPr (v_array, v) ;
  mxSetM (v_array, n) ;
  mxSetN (v_array, 1) ;
  return v_array ;
#else
  return newMxArrayFromDoubleVector (n, v) ;
#endif
}

inline_comm static mxArray *
newMxArrayEncapsulatingSmodel (STRUCTMODEL * smodel)
{
  mxArray * alpha_array;
  mxArray * svPatterns_array;
  mxArray * svLabels_array;

  mwSize dims [] = {1, 1} ;
  char const * fieldNames [] = {
    "w", "alpha", "svPatterns", "svLabels"
  } ;  
  mxArray * smodel_array = mxCreateStructArray (2, dims, 4, fieldNames) ;
  
  /* we cannot just encapsulate the arrays because we need to shift by
   * one */
  if (smodel -> svm_model -> kernel_parm .kernel_type == LINEAR) {
    mxSetField (smodel_array, 0, "w",
                newMxArrayFromDoubleVector 
                (smodel->sizePsi, smodel->w + 1) ) ;
  } else {
    int numFeatures = 0, fi, svi ;
    SVECTOR * sv ;
    double * alpha ;
        
    /* count how much space we need to store the expansion */
    for (svi = 1 ; svi < smodel->svm_model->sv_num ; ++svi) {
      for (sv = smodel->svm_model->supvec[svi]->fvec ;
           sv ;
           sv = sv -> next) {
      ++ numFeatures ; 
      }
    }

    alpha_array = mxCreateDoubleMatrix (numFeatures, 1, mxREAL) ;
    svPatterns_array = mxCreateCellMatrix (1, numFeatures) ;
    svLabels_array = mxCreateCellMatrix (1, numFeatures) ;
    
    /* fill in the values */
    alpha = mxGetPr (alpha_array) ;
    for (fi = 0, svi = 1 ; svi < smodel->svm_model->sv_num ; ++svi) {
      for (sv = smodel->svm_model->supvec[svi]->fvec ;
           sv ; 
           sv = sv -> next, ++ fi) {        
        alpha [fi] = smodel->svm_model->alpha[svi] * sv->factor ;
        mxSetCell(svPatterns_array, fi, MexPhiCustomGetPattern (sv->userdefined)) ;
        mxSetCell(svLabels_array,   fi, MexPhiCustomGetLabel   (sv->userdefined)) ;
      }    
    }
    
    mxSetField (smodel_array, 0, "alpha", alpha_array) ;
    mxSetField (smodel_array, 0, "svPatterns", svPatterns_array) ;
    mxSetField (smodel_array, 0, "svLabels", svLabels_array) ;
  }  
  return smodel_array ;
}

inline_comm static void
destroyMxArrayEncapsulatingDoubleVector (mxArray * array)
{
  if (array) {
    mxSetN (array, 0) ;
    mxSetM (array, 0) ;
    mxSetPr (array, NULL) ;
    mxDestroyArray (array) ;
  }
}

inline_comm static void
destroyMxArrayEncapsulatingSmodel (mxArray * array)
{
  if (array) {
    /* w and alpha are freed by mxDestroyArray, but we do not want this
     * to happen to the encapsulated patterns and labels yet (or are these shared?) */
    int i, n ;
    mxArray * svPatterns_array = mxGetField (array, 0, "svPatterns") ;
    mxArray * svLabels_array   = mxGetField (array, 0, "svLabels") ;
    if (svPatterns_array) {
      n = mxGetNumberOfElements (svPatterns_array) ;    
      for (i = 0 ; i < n ; ++ i) {
        mxSetCell (svPatterns_array, i, NULL) ;
        mxSetCell (svLabels_array, i, NULL) ;
      }
    }
    mxDestroyArray (array) ;
  }
}

              
#endif
