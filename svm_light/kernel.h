/************************************************************************/
/*                                                                      */
/*   kernel.h                                                           */
/*                                                                      */
/*   User defined kernel function. Feel free to plug in your own.       */
/*                                                                      */
/*   Copyright: Thorsten Joachims                                       */
/*   Date: 16.12.97                                                     */
/*                                                                      */
/************************************************************************/

/* KERNEL_PARM is defined in svm_common.h The field 'custom' is reserved for */
/* parameters of the user defined kernel. You can also access and use */
/* the parameters of the other kernels. Just replace the line
             return((double)(1.0));
   with your own kernel. */

  /* Example: The following computes the polynomial kernel. sprod_ss
              computes the inner product between two sparse vectors.

      return((CFLOAT)pow(kernel_parm->coef_lin*sprod_ss(a,b)
             +kernel_parm->coef_const,(double)kernel_parm->poly_degree));
  */

/* If you are implementing a kernel that is not based on a
   feature/value representation, you might want to make use of the
   field "userdefined" in SVECTOR. By default, this field will contain
   whatever string you put behind a # sign in the example file. So, if
   a line in your training file looks like

   -1 1:3 5:6 #abcdefg

   then the SVECTOR field "words" will contain the vector 1:3 5:6, and
   "userdefined" will contain the string "abcdefg". */

#include "../svm_struct_api_types.h" 

double custom_kernel(KERNEL_PARM *kernel_parm, SVECTOR *a, SVECTOR *b)
     /* plug in you favorite kernel */
{
  mxArray* structParm_array ;
  mxArray* kernelFn_array ;
  mxArray* args [6] ;
  mxArray* k_array ;
  double k  ;
  int status ;

  {
    MexKernelInfo* info = (MexKernelInfo*) kernel_parm -> custom ;
    structParm_array = (mxArray*) info -> structParm ; 
    kernelFn_array = (mxArray*) info -> kernelFn ;
  }

  args[0] = kernelFn_array ;
  args[1] = structParm_array ;
  args[2] = MexPhiCustomGetPattern (a->userdefined) ;
  args[3] = MexPhiCustomGetLabel (a->userdefined) ;
  args[4] = MexPhiCustomGetPattern (b->userdefined) ;
  args[5] = MexPhiCustomGetLabel (b->userdefined) ;

  status = mexCallMATLAB(1, &k_array, 6, args, "feval") ;

  if (status) {
    mexErrMsgTxt("Error while executing SPARM.KERNELFN") ;
  }
  if (mxGetClassID(k_array) == mxUNKNOWN_CLASS) {
    mexErrMsgTxt("SPARM.KERNELFN did not reutrn a result") ;
  }

  k = mxGetScalar (k_array) ;
  mxDestroyArray (k_array) ;

  return (k) ;
}
