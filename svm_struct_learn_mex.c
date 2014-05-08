/***********************************************************************/
/*                                                                     */
/*   svm_struct_main.c                                                 */
/*                                                                     */
/*   Command line interface to the alignment learning module of the    */
/*   Support Vector Machine.                                           */
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

#ifdef __cplusplus
extern "C" {
#endif

#include "svm_light/svm_common.h"
#include "svm_light/svm_learn.h"

#ifdef __cplusplus
}
#endif

# include "svm_struct/svm_struct_learn.h"
# include "svm_struct/svm_struct_common.h"
# include "svm_struct_api.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

void read_input_parameters (int, char **,
                            long *, long *,
                            STRUCT_LEARN_PARM *, LEARN_PARM *, KERNEL_PARM *,
                            int *);

void arg_split (char *string, int *argc, char ***argv) ;
void init_qp_solver() ;
void free_qp_solver() ;

/** ------------------------------------------------------------------
 ** @brief MEX entry point
 **/

void
mexFunction (int nout, mxArray ** out, int nin, mxArray const ** in)
{
  SAMPLE sample;  /* training sample */
  LEARN_PARM learn_parm;
  KERNEL_PARM kernel_parm;
  STRUCT_LEARN_PARM struct_parm;
  STRUCTMODEL structmodel;
  int alg_type;

  enum {IN_ARGS=0, IN_SPARM} ;
  enum {OUT_W=0} ;

  char arg [1024 + 1] ;
  int argc ;
  char ** argv ;

  mxArray const * sparm_array;
  mxArray const * patterns_array ;
  mxArray const * labels_array ;
  mxArray const * kernelFn_array ;
  int numExamples, ei ;
  mxArray * model_array;

  /* SVM-light is not fully reentrant, so we need to run this patch first */
  init_qp_solver() ;
  verbosity = 0 ;
  kernel_cache_statistic = 0 ;

  if (nin != 2) {
    mexErrMsgTxt("Two arguments required") ;
  }

  /* Parse ARGS  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

  if (! uIsString(in[IN_ARGS], -1)) {
    mexErrMsgTxt("ARGS must be a string") ;
  }

  mxGetString(in[IN_ARGS], arg, sizeof(arg) / sizeof(char)) ;
  arg_split (arg, &argc, &argv) ;

  svm_struct_learn_api_init(argc+1, argv-1) ;

  read_input_parameters (argc+1,argv-1,
                         &verbosity, &struct_verbosity,
                         &struct_parm, &learn_parm,
                         &kernel_parm, &alg_type ) ;
  
  if (kernel_parm.kernel_type != LINEAR &&
      kernel_parm.kernel_type != CUSTOM) {
    mexErrMsgTxt ("Only LINEAR or CUSTOM kerneles are supported") ;
  }

  /* Parse SPARM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  sparm_array = in [IN_SPARM] ;
  /* jk remove */

  if (! sparm_array) {
    mexErrMsgTxt("SPARM must be a structure") ;
  }
  struct_parm.mex = sparm_array ;

  patterns_array = mxGetField(sparm_array, 0, "patterns") ;
  if (! patterns_array ||
      ! mxIsCell(patterns_array)) {
    mexErrMsgTxt("SPARM.PATTERNS must be a cell array") ;
  }

  numExamples = mxGetNumberOfElements(patterns_array) ;

  labels_array = mxGetField(sparm_array, 0, "labels") ;
  if (! labels_array ||
      ! mxIsCell(labels_array) ||
      ! mxGetNumberOfElements(labels_array) == numExamples) {
    mexErrMsgTxt("SPARM.LABELS must be a cell array "
                 "with the same number of elements of "
                 "SPARM.PATTERNS") ;
  }

  sample.n = numExamples ;
  sample.examples = (EXAMPLE *) my_malloc (sizeof(EXAMPLE) * numExamples) ;
  for (ei = 0 ; ei < numExamples ; ++ ei) {
    sample.examples[ei].x.mex = mxGetCell(patterns_array, ei) ;
    sample.examples[ei].y.mex = mxGetCell(labels_array,   ei) ;
    sample.examples[ei].y.isOwner = 0 ;
  }

  if (struct_verbosity >= 1) {
    mexPrintf("There are %d training examples\n", numExamples) ;
  }

  kernelFn_array = mxGetField(sparm_array, 0, "kernelFn") ;
  if (! kernelFn_array && kernel_parm.kernel_type == CUSTOM) {
    mexErrMsgTxt("SPARM.KERNELFN must be defined for CUSTOM kernels") ;
  }
  if (kernelFn_array) {
    MexKernelInfo * info ;
    if (mxGetClassID(kernelFn_array) != mxFUNCTION_CLASS) {
      mexErrMsgTxt("SPARM.KERNELFN must be a valid function handle") ;
    }
    info = (MexKernelInfo*) kernel_parm.custom ;
    info -> structParm = sparm_array ;
    info -> kernelFn   = kernelFn_array ;
  }

  /* Learning  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  switch (alg_type) {
  case 0:
    svm_learn_struct(sample,&struct_parm,&learn_parm,&kernel_parm,&structmodel,NSLACK_ALG) ;
    break ;
  case 1:
    svm_learn_struct(sample,&struct_parm,&learn_parm,&kernel_parm,&structmodel,NSLACK_SHRINK_ALG);
    break ;
  case 2:
    svm_learn_struct_joint(sample,&struct_parm,&learn_parm,&kernel_parm,&structmodel,ONESLACK_PRIMAL_ALG);
    break ;
  case 3:
    svm_learn_struct_joint(sample,&struct_parm,&learn_parm,&kernel_parm,&structmodel,ONESLACK_DUAL_ALG);
    break ;
  case 4:
    svm_learn_struct_joint(sample,&struct_parm,&learn_parm,&kernel_parm,&structmodel,ONESLACK_DUAL_CACHE_ALG);
    break  ;
  case 9:
    svm_learn_struct_joint_custom(sample,&struct_parm,&learn_parm,&kernel_parm,&structmodel);
    break ;
  default:
    mexErrMsgTxt("Unknown algorithm type") ;
  }

  /* Write output  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

  /* Warning: The model contains references to the original data 'docs'.
     If you want to free the original data, and only keep the model, you
     have to make a deep copy of 'model'. */

  /* jk change */
  model_array = newMxArrayEncapsulatingSmodel (&structmodel) ;
  out[OUT_W] = mxDuplicateArray (model_array) ;
  destroyMxArrayEncapsulatingSmodel (model_array) ;
  
  free_struct_sample (sample) ;
  free_struct_model (structmodel) ;
  svm_struct_learn_api_exit () ;
  free_qp_solver () ;
}

/** ------------------------------------------------------------------
 ** @brief Parse argument string
 **/

void
read_input_parameters (int argc,char *argv[],
                       long *verbosity,long *struct_verbosity,
                       STRUCT_LEARN_PARM *struct_parm,
                       LEARN_PARM *learn_parm, KERNEL_PARM *kernel_parm,
                       int *alg_type)
{
  long i ;

  (*alg_type)=DEFAULT_ALG_TYPE;

  /* SVM struct options */
  (*struct_verbosity)=1;

  struct_parm->C=-0.01;
  struct_parm->slack_norm=1;
  struct_parm->epsilon=DEFAULT_EPS;
  struct_parm->custom_argc=0;
  struct_parm->loss_function=DEFAULT_LOSS_FCT;
  struct_parm->loss_type=DEFAULT_RESCALING;
  struct_parm->newconstretrain=100;
  struct_parm->ccache_size=5;
  struct_parm->batch_size=100;

  /* SVM light options */
  (*verbosity)=0;

  strcpy (learn_parm->predfile, "trans_predictions");
  strcpy (learn_parm->alphafile, "");
  learn_parm->biased_hyperplane=1;
  learn_parm->remove_inconsistent=0;
  learn_parm->skip_final_opt_check=0;
  learn_parm->svm_maxqpsize=10;
  learn_parm->svm_newvarsinqp=0;
  learn_parm->svm_iter_to_shrink=-9999;
  learn_parm->maxiter=100000;
  learn_parm->kernel_cache_size=40;
  learn_parm->svm_c=99999999;  /* overridden by struct_parm->C */
  learn_parm->eps=0.001;       /* overridden by struct_parm->epsilon */
  learn_parm->transduction_posratio=-1.0;
  learn_parm->svm_costratio=1.0;
  learn_parm->svm_costratio_unlab=1.0;
  learn_parm->svm_unlabbound=1E-5;
  learn_parm->epsilon_crit=0.001;
  learn_parm->epsilon_a=1E-10;  /* changed from 1e-15 */
  learn_parm->compute_loo=0;
  learn_parm->rho=1.0;
  learn_parm->xa_depth=0;

  kernel_parm->kernel_type=0;
  kernel_parm->poly_degree=3;
  kernel_parm->rbf_gamma=1.0;
  kernel_parm->coef_lin=1;
  kernel_parm->coef_const=1;
  strcpy (kernel_parm->custom,"empty");

  /* Parse -x options, delegat --x ones */
  for(i=1;(i<argc) && ((argv[i])[0] == '-');i++) {
    switch ((argv[i])[1])
      {
      case 'a': i++; strcpy(learn_parm->alphafile,argv[i]); break;
      case 'c': i++; struct_parm->C=atof(argv[i]); break;
      case 'p': i++; struct_parm->slack_norm=atol(argv[i]); break;
      case 'e': i++; struct_parm->epsilon=atof(argv[i]); break;
      case 'k': i++; struct_parm->newconstretrain=atol(argv[i]); break;
      case 'h': i++; learn_parm->svm_iter_to_shrink=atol(argv[i]); break;
      case '#': i++; learn_parm->maxiter=atol(argv[i]); break;
      case 'm': i++; learn_parm->kernel_cache_size=atol(argv[i]); break;
      case 'w': i++; (*alg_type)=atol(argv[i]); break;
      case 'o': i++; struct_parm->loss_type=atol(argv[i]); break;
      case 'n': i++; learn_parm->svm_newvarsinqp=atol(argv[i]); break;
      case 'q': i++; learn_parm->svm_maxqpsize=atol(argv[i]); break;
      case 'l': i++; struct_parm->loss_function=atol(argv[i]); break;
      case 'f': i++; struct_parm->ccache_size=atol(argv[i]); break;
      case 'b': i++; struct_parm->batch_size=atof(argv[i]); break;
      case 't': i++; kernel_parm->kernel_type=atol(argv[i]); break;
      case 'd': i++; kernel_parm->poly_degree=atol(argv[i]); break;
      case 'g': i++; kernel_parm->rbf_gamma=atof(argv[i]); break;
      case 's': i++; kernel_parm->coef_lin=atof(argv[i]); break;
      case 'r': i++; kernel_parm->coef_const=atof(argv[i]); break;
      case 'u': i++; strcpy(kernel_parm->custom,argv[i]); break;
      case 'v': i++; (*struct_verbosity)=atol(argv[i]); break;
      case 'y': i++; (*verbosity)=atol(argv[i]); break;
      case '-':
        strcpy(struct_parm->custom_argv[struct_parm->custom_argc++],argv[i]);
        i++;
        strcpy(struct_parm->custom_argv[struct_parm->custom_argc++],argv[i]);
        break;
      default:
        {
          char msg [1024+1] ;
          #ifndef WIN
            snprintf(msg, sizeof(msg)/sizeof(char),
                   "Unrecognized option '%s'",argv[i]) ;
          #else
           sprintf(msg, sizeof(msg)/sizeof(char),
                   "Unrecognized option '%s'",argv[i]) ;
          #endif
          mexErrMsgTxt(msg) ;
        }
      }
  }

  /* whatever is left is an error */
  if (i < argc) {
    char msg [1024+1] ;
    #ifndef WIN
        snprintf(msg, sizeof(msg)/sizeof(char),
             "Unrecognized argument '%s'", argv[i]) ;
    #else
        sprintf(msg, sizeof(msg)/sizeof(char),
             "Unrecognized argument '%s'", argv[i]) ;
    #endif
    mexErrMsgTxt(msg) ;
  }

  /* Check parameter validity */
  if(learn_parm->svm_iter_to_shrink == -9999) {
    learn_parm->svm_iter_to_shrink=100;
  }

  if((learn_parm->skip_final_opt_check)
     && (kernel_parm->kernel_type == LINEAR)) {
    mexWarnMsgTxt("It does not make sense to skip the final optimality check for linear kernels.");
    learn_parm->skip_final_opt_check=0;
  }
  if((learn_parm->skip_final_opt_check)
     && (learn_parm->remove_inconsistent)) {
    mexErrMsgTxt("It is necessary to do the final optimality check when removing inconsistent examples.");
  }
  if((learn_parm->svm_maxqpsize<2)) {
    char msg [1025] ;
    #ifndef WIN
    snprintf(msg, sizeof(msg)/sizeof(char),
             "Maximum size of QP-subproblems not in valid range: %ld [2..]",learn_parm->svm_maxqpsize) ;
    #else
    sprintf(msg, sizeof(msg)/sizeof(char),
            "Maximum size of QP-subproblems not in valid range: %ld [2..]",learn_parm->svm_maxqpsize) ;
    #endif
    mexErrMsgTxt(msg) ;
  }
  if((learn_parm->svm_maxqpsize<learn_parm->svm_newvarsinqp)) {
    char msg [1025] ;
    #ifndef WIN
    snprintf(msg, sizeof(msg)/sizeof(char),
             "Maximum size of QP-subproblems [%ld] must be larger than the number of"
             " new variables [%ld] entering the working set in each iteration.",
             learn_parm->svm_maxqpsize, learn_parm->svm_newvarsinqp) ;
    #else
    sprintf(msg, sizeof(msg)/sizeof(char),
             "Maximum size of QP-subproblems [%ld] must be larger than the number of"
             " new variables [%ld] entering the working set in each iteration.",
             learn_parm->svm_maxqpsize, learn_parm->svm_newvarsinqp) ;
    #endif
    mexErrMsgTxt(msg) ;
  }
  if(learn_parm->svm_iter_to_shrink<1) {
    char msg [1025] ;
    #ifndef WIN
    snprintf(msg, sizeof(msg)/sizeof(char),
             "Maximum number of iterations for shrinking not in valid range: %ld [1,..]",
             learn_parm->svm_iter_to_shrink);
    #else
    sprintf(msg, sizeof(msg)/sizeof(char),
             "Maximum number of iterations for shrinking not in valid range: %ld [1,..]",
             learn_parm->svm_iter_to_shrink);
    #endif
    mexErrMsgTxt(msg) ;
  }
  if(struct_parm->C<0) {
    mexErrMsgTxt("You have to specify a value for the parameter '-c' (C>0)!");
  }
  if(((*alg_type) < 0) || (((*alg_type) > 5) && ((*alg_type) != 9))) {
    mexErrMsgTxt("Algorithm type must be either '0', '1', '2', '3', '4', or '9'!");
  }
  if(learn_parm->transduction_posratio>1) {
    mexErrMsgTxt("The fraction of unlabeled examples to classify as positives must "
                 "be less than 1.0 !!!");
  }
  if(learn_parm->svm_costratio<=0) {
    mexErrMsgTxt("The COSTRATIO parameter must be greater than zero!");
  }
  if(struct_parm->epsilon<=0) {
    mexErrMsgTxt("The epsilon parameter must be greater than zero!");
  }
  if((struct_parm->ccache_size<=0) && ((*alg_type) == 4)) {
    mexErrMsgTxt("The cache size must be at least 1!");
  }
  if(((struct_parm->batch_size<=0) || (struct_parm->batch_size>100))
     && ((*alg_type) == 4)) {
    mexErrMsgTxt("The batch size must be in the interval ]0,100]!");
  }
  if((struct_parm->slack_norm<1) || (struct_parm->slack_norm>2)) {
    mexErrMsgTxt("The norm of the slacks must be either 1 (L1-norm) or 2 (L2-norm)!");
  }
  if((struct_parm->loss_type != SLACK_RESCALING)
     && (struct_parm->loss_type != MARGIN_RESCALING)) {
    mexErrMsgTxt("The loss type must be either 1 (slack rescaling) or 2 (margin rescaling)!");
  }
  if(learn_parm->rho<0) {
    mexErrMsgTxt("The parameter rho for xi/alpha-estimates and leave-one-out pruning must"
                 " be greater than zero (typically 1.0 or 2.0, see T. Joachims, Estimating the"
                 " Generalization Performance of an SVM Efficiently, ICML, 2000.)!");
  }
  if((learn_parm->xa_depth<0) || (learn_parm->xa_depth>100)) {
    mexErrMsgTxt("The parameter depth for ext. xi/alpha-estimates must be in [0..100] (zero"
                  "for switching to the conventional xa/estimates described in T. Joachims,"
                  "Estimating the Generalization Performance of an SVM Efficiently, ICML, 2000.)") ;
  }

  parse_struct_parameters (struct_parm) ;
}

void
arg_split (char *string, int *argc, char ***argv)
{
  size_t size;
  char *d, *p;

  for (size = 1, p = string; *p; p++) {
    if (isspace((int) *p)) {
      size++;
    }
  }
  size++;			/* leave space for final NULL pointer. */

  *argv = (char **) my_malloc(((size * sizeof(char *)) + (p - string) + 1));

  for (*argc = 0, p = string, d = ((char *) *argv) + size*sizeof(char *);
       *p != 0; ) {
    (*argv)[*argc] = NULL;
    while (*p && isspace((int) *p)) p++;
    if (*argc == 0 && *p == '#') {
      break;
    }
    if (*p) {
      char *s = p;
      (*argv)[(*argc)++] = d;
      while (*p && !isspace((int) *p)) p++;
      memcpy(d, s, p-s);
      d += p-s;
      *d++ = 0;
      while (*p && isspace((int) *p)) p++;
    }
  }
}
