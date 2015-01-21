/************************************************************************/
/*                                                                      */
/*   svm_common.c                                                       */
/*                                                                      */
/*   Definitions and functions used in both svm_learn and svm_classify. */
/*                                                                      */
/*   Author: Thorsten Joachims                                          */
/*   Date: 02.07.04                                                     */
/*                                                                      */
/*   Copyright (c) 2004  Thorsten Joachims - All rights reserved        */
/*                                                                      */
/*   This software is available for non-commercial use only. It must    */
/*   not be modified and distributed without prior permission of the    */
/*   author. The author is not responsible for implications from the    */
/*   use of this software.                                              */
/*                                                                      */
/************************************************************************/

# include "ctype.h"
# include "svm_common.h"
# include "kernel.h"           /* this contains a user supplied kernel */

#define MAX(x,y)      ((x) < (y) ? (y) : (x))
#define MIN(x,y)      ((x) > (y) ? (y) : (x))
#define SIGN(x)       ((x) > (0) ? (1) : (((x) < (0) ? (-1) : (0))))

long   verbosity;              /* verbosity level (0-4) */
long   kernel_cache_statistic;

double classify_example(MODEL *model, DOC *ex) 
     /* classifies one example */
{
  register long i;
  register double dist;

  if((model->kernel_parm.kernel_type == LINEAR) && (model->lin_weights))
    return(classify_example_linear(model,ex));
	   
  dist=0;
  for(i=1;i<model->sv_num;i++) {  
    dist+=kernel(&model->kernel_parm,model->supvec[i],ex)*model->alpha[i];
  }
  return(dist-model->b);
}

double classify_example_linear(MODEL *model, DOC *ex) 
     /* classifies example for linear kernel */
     
     /* important: the model must have the linear weight vector computed */
     /* use: add_weight_vector_to_linear_model(&model); */


     /* important: the feature numbers in the example to classify must */
     /*            not be larger than the weight vector!               */
{
  double sum=0;
  SVECTOR *f;

  for(f=ex->fvec;f;f=f->next)  
    sum+=f->factor*sprod_ns(model->lin_weights,f);
  return(sum-model->b);
}


double kernel(KERNEL_PARM *kernel_parm, DOC *a, DOC *b) 
     /* calculate the kernel function */
{
  double sum=0;
  SVECTOR *fa,*fb;

  if(kernel_parm->kernel_type == GRAM) {  /* use value from explicitly */
    if((a->kernelid>=0) && (b->kernelid>=0)) /* stored gram matrix */
      return(kernel_parm->gram_matrix->element[MAX(a->kernelid,b->kernelid)]
	                                      [MIN(a->kernelid,b->kernelid)]);
    else 
      return(0); /* in case it is called for unknown vector */
  }

  /* in case the constraints are sums of feature vector as represented
     as a list of SVECTOR's with their coefficient factor in the sum,
     take the kernel between all pairs */ 
  for(fa=a->fvec;fa;fa=fa->next) { 
    for(fb=b->fvec;fb;fb=fb->next) {
      if(fa->kernel_id == fb->kernel_id)
	sum+=fa->factor*fb->factor*single_kernel(kernel_parm,fa,fb);
    }
  }
  return(sum);
}

double single_kernel(KERNEL_PARM *kernel_parm, SVECTOR *a, SVECTOR *b) 
     /* calculate the kernel function between two vectors */
{
  kernel_cache_statistic++;
  switch(kernel_parm->kernel_type) {
    case LINEAR: /* linear */ 
            return(sprod_ss(a,b)); 
    case POLY:   /* polynomial */
            return(pow(kernel_parm->coef_lin*sprod_ss(a,b)+kernel_parm->coef_const,(double)kernel_parm->poly_degree)); 
    case RBF:    /* radial basis function */
            if(a->twonorm_sq<0) a->twonorm_sq=sprod_ss(a,a);
            if(b->twonorm_sq<0) b->twonorm_sq=sprod_ss(b,b);
            return(exp(-kernel_parm->rbf_gamma*(a->twonorm_sq-2*sprod_ss(a,b)+b->twonorm_sq)));
    case SIGMOID:/* sigmoid neural net */
            return(tanh(kernel_parm->coef_lin*sprod_ss(a,b)+kernel_parm->coef_const)); 
    case CUSTOM: /* custom-kernel supplied in file kernel.h*/
            return(custom_kernel(kernel_parm,a,b)); 
    default: printf("Error: Unknown kernel function\n"); exit(1);
  }
}

SVECTOR *create_svector(WORD *words, MexPhiCustom userdefined, double factor)
{
  SVECTOR *vec;
  long    fnum,i;

  fnum=0;
  while(words[fnum].wnum) {
    fnum++;
  }
  fnum++;
  vec = (SVECTOR *)my_malloc(sizeof(SVECTOR));
  vec->words = (WORD *)my_malloc(sizeof(WORD)*(fnum));
  for(i=0;i<fnum;i++) { 
      vec->words[i]=words[i];
  }
  vec->twonorm_sq=-1;

  retainMexPhiCustom(userdefined) ;
  vec->userdefined = userdefined ;

  vec->kernel_id=0;
  vec->next=NULL;
  vec->factor=factor;
  return(vec);
}

SVECTOR *create_svector_shallow(WORD *words,MexPhiCustom userdefined,double factor)
     /* unlike 'create_svector' this does not copy words and userdefined */
{
  SVECTOR *vec;
  
  retainMexPhiCustom(userdefined) ;
  
  vec = (SVECTOR *)my_malloc(sizeof(SVECTOR));
  vec->words = words;
  vec->twonorm_sq=-1;
  vec->userdefined=userdefined;
  vec->kernel_id=0;
  vec->next=NULL;
  vec->factor=factor;
  return(vec);
}

SVECTOR *create_svector_n(double *nonsparsevec, long maxfeatnum, MexPhiCustom userdefined, double factor)
{
  return(create_svector_n_r(nonsparsevec,maxfeatnum,userdefined,factor,0));
}

SVECTOR *create_svector_n_r(double *nonsparsevec, long maxfeatnum, MexPhiCustom userdefined, double factor, double min_non_zero)
{
  SVECTOR *vec;
  long    fnum,i;

  fnum=0;
  for(i=1;i<=maxfeatnum;i++)  
    if((nonsparsevec[i]<-min_non_zero) || (nonsparsevec[i]>min_non_zero))
      fnum++;
  vec = (SVECTOR *)my_malloc(sizeof(SVECTOR));
  vec->words = (WORD *)my_malloc(sizeof(WORD)*(fnum+1));
  fnum=0;
  for(i=1;i<=maxfeatnum;i++) { 
    if((nonsparsevec[i]<-min_non_zero) || (nonsparsevec[i]>min_non_zero)) {
      vec->words[fnum].wnum=i;
      vec->words[fnum].weight=nonsparsevec[i];
      fnum++;
    }
  }
  vec->words[fnum].wnum=0;
  vec->twonorm_sq=-1;

  retainMexPhiCustom(userdefined) ;
  vec->userdefined = userdefined ;

  vec->kernel_id=0;
  vec->next=NULL;
  vec->factor=factor;
  return(vec);
}

SVECTOR *copy_svector(SVECTOR *vec)
{
  SVECTOR *newvec=NULL;
  if(vec) {
    newvec=create_svector(vec->words,vec->userdefined,vec->factor);
    newvec->kernel_id=vec->kernel_id;
    newvec->next=copy_svector(vec->next);
  }
  return(newvec);
}
    
SVECTOR *copy_svector_shallow(SVECTOR *vec)
     /* unlike 'copy_svector' this does not copy words and userdefined */
{
  SVECTOR *newvec=NULL;
  if(vec) {
    newvec=create_svector_shallow(vec->words,vec->userdefined,vec->factor);
    newvec->kernel_id=vec->kernel_id;
    newvec->next=copy_svector_shallow(vec->next);
  }
  return(newvec);
}
    
void free_svector(SVECTOR *vec)
{
  SVECTOR *next;
  while(vec) {
    if(vec->words)
      free(vec->words);
    releaseMexPhiCustom (vec->userdefined) ;
    next=vec->next;
    free(vec);
    vec=next;
  }
}

void free_svector_shallow(SVECTOR *vec)
     /* unlike 'free_svector' this does not free words and userdefined */
{
  SVECTOR *next;
  while(vec) {
    next=vec->next;
    releaseMexPhiCustom(vec->userdefined) ;
    free(vec);
    vec=next;
  }
}

double sprod_ss(SVECTOR *a, SVECTOR *b) 
     /* compute the inner product of two sparse vectors */
{
    register double sum=0;
    register WORD *ai,*bj;
    ai=a->words;
    bj=b->words;
    while (ai->wnum && bj->wnum) {
      if(ai->wnum > bj->wnum) {
	bj++;
      }
      else if (ai->wnum < bj->wnum) {
	ai++;
      }
      else {
	sum+=(ai->weight) * (bj->weight);
	ai++;
	bj++;
      }
    }
    return((double)sum);
}

SVECTOR* multadd_ss(SVECTOR *a, SVECTOR *b, double fa, double fb)
{
  return(multadd_ss_r(a,b,fa,fb,0));
}

SVECTOR* multadd_ss_r(SVECTOR *a,SVECTOR *b,double fa, double fb,
		      double min_non_zero) 
     /* compute fa*a+fb*b of two sparse vectors */
     /* Note: SVECTOR lists are not followed, but only the first
	SVECTOR is used */
{
    SVECTOR *vec;
    register WORD *sum,*sumi;
    register WORD *ai,*bj;
    long veclength;
    double weight;
  
    ai=a->words;
    bj=b->words;
    veclength=0;
    while (ai->wnum && bj->wnum) {
      if(ai->wnum > bj->wnum) {
	veclength++;
	bj++;
      }
      else if (ai->wnum < bj->wnum) {
	veclength++;
	ai++;
      }
      else {
	veclength++;
	ai++;
	bj++;
      }
    }
    while (bj->wnum) {
      veclength++;
      bj++;
    }
    while (ai->wnum) {
      veclength++;
      ai++;
    }
    veclength++;

    sum=(WORD *)my_malloc(sizeof(WORD)*veclength);
    sumi=sum;
    ai=a->words;
    bj=b->words;
    while (ai->wnum && bj->wnum) {
      if(ai->wnum > bj->wnum) {
	(*sumi)=(*bj);
	sumi->weight*=fb;
	sumi++;
	bj++;
      }
      else if (ai->wnum < bj->wnum) {
	(*sumi)=(*ai);
	sumi->weight*=fa;
	sumi++;
	ai++;
      }
      else {
	weight=fa*(double)ai->weight+fb*(double)bj->weight;
	if((weight<-min_non_zero) || (weight>min_non_zero)) {
	  sumi->wnum=ai->wnum;
	  sumi->weight=weight;
	  sumi++;
	}
	ai++;
	bj++;
      }
    }
    while (bj->wnum) {
      (*sumi)=(*bj);
      sumi->weight*=fb;
      sumi++;
      bj++;
    }
    while (ai->wnum) {
      (*sumi)=(*ai);
      sumi->weight*=fa;
      sumi++;
      ai++;
    }
    sumi->wnum=0;

    if(1) { /* potentially this wastes some memory, but saves malloc'ing */
      vec=create_svector_shallow(sum,NULL,1.0);
    }
    else {  /* this is more memory efficient */
      vec=create_svector(sum,NULL,1.0);
      free(sum);
    }
    return(vec);
}

SVECTOR* sub_ss(SVECTOR *a, SVECTOR *b) 
     /* compute the difference a-b of two sparse vectors */
     /* Note: SVECTOR lists are not followed, but only the first
	SVECTOR is used */
{
  return(multadd_ss_r(a,b,1.0,-1.0,0));
}

SVECTOR* sub_ss_r(SVECTOR *a, SVECTOR *b, double min_non_zero) 
     /* compute the difference a-b of two sparse vectors and rounds to zero */
     /* Note: SVECTOR lists are not followed, but only the first
	SVECTOR is used */
{
  return(multadd_ss_r(a,b,1.0,-1.0,min_non_zero));
}

SVECTOR* add_ss(SVECTOR *a, SVECTOR *b) 
     /* compute the sum a+b of two sparse vectors */
     /* Note: SVECTOR lists are not followed, but only the first
	SVECTOR is used */
{
  return(multadd_ss_r(a,b,1.0,1.0,0));
}

SVECTOR* add_ss_r(SVECTOR *a, SVECTOR *b, double min_non_zero) 
     /* compute the sum a+b of two sparse vectors and rounds to zero */
     /* Note: SVECTOR lists are not followed, but only the first
	SVECTOR is used */
{
  return(multadd_ss_r(a,b,1.0,1.0,min_non_zero));
}

SVECTOR* add_list_ss(SVECTOR *a) 
{
  return(add_list_ss_r(a,0));
}

SVECTOR* add_dual_list_ss_r(SVECTOR *a, SVECTOR *b, double min_non_zero) 
     /* computes the linear combination of the two SVECTOR lists weighted
	by the factor of each SVECTOR */
{
  SVECTOR *f,*sum;

  for(f=a;f->next;f=f->next);  /* find end of first vector list */
  f->next=b;                   /* temporarily append the two vector lists */
  sum=add_list_ss_r(a,min_non_zero);
  f->next=NULL;                /* undo append */
  return(sum);
}

SVECTOR* add_list_ss_r(SVECTOR *a, double min_non_zero) 
     /* computes the linear combination of the SVECTOR list weighted
	by the factor of each SVECTOR */
{
  SVECTOR *oldsum,*sum,*f;
  WORD    empty[2];
    
  if(!a) {
    empty[0].wnum=0;
    sum=create_svector(empty,NULL,1.0);
  }
  else if(a && (!a->next)) {
    sum=smult_s(a,a->factor);
  }
  else {
    sum=multadd_ss_r(a,a->next,a->factor,a->next->factor,min_non_zero);
    for(f=a->next->next;f;f=f->next) {
      oldsum=sum;
      sum=multadd_ss_r(oldsum,f,1.0,f->factor,min_non_zero);
      free_svector(oldsum);
    }
  }
  return(sum);
}

int compareup_word(const void *a, const void *b) 
{
  double va,vb;
  va=((WORD *)a)->wnum;
  vb=((WORD *)b)->wnum;
  return((va > vb) - (va < vb));
}

SVECTOR* add_list_sort_ss(SVECTOR *a) 
     /* computes the linear combination of the SVECTOR list weighted
	by the factor of each SVECTOR. This should be a lot faster
	than add_list_ss for long lists. */
{
  return(add_list_sort_ss_r(a,0));
}

SVECTOR* add_dual_list_sort_ss_r(SVECTOR *a, SVECTOR *b, double min_non_zero) 
     /* computes the linear combination of the two SVECTOR lists weighted
	by the factor of each SVECTOR */
{
  SVECTOR *f,*sum;

  for(f=a;f->next;f=f->next);  /* find end of first vector list */
  f->next=b;                   /* temporarily append the two vector lists */
  sum=add_list_sort_ss_r(a,min_non_zero);
  f->next=NULL;                /* undo append */
  return(sum);
}

SVECTOR* add_list_sort_ss_r(SVECTOR *a, double min_non_zero) 
     /* Like add_list_sort_ss(SVECTOR *a), but rounds values smaller
	than min_non_zero to zero. */
{
  SVECTOR *sum,*f;
  WORD    empty[2],*ai,*concat,*concati,*concat_read,*concat_write;
  long    length,i;
  double  weight;
    
  if(a){
    /* count number or entries over all vectors */
    length=0;
    for(f=a;f;f=f->next) {

      ai=f->words;
      while (ai->wnum) {
	length++;
	ai++;
      }
    }

    /* write all entries into one long array and sort by feature number */
    concat=(WORD *)my_malloc(sizeof(WORD)*(length+1));
    concati=concat;
    for(f=a;f;f=f->next) {
      ai=f->words;
      while (ai->wnum) {
	(*concati)=(*ai);
	concati->weight*=f->factor;
	concati++;
	ai++;
      }
    }
    qsort(concat,length,sizeof(WORD),compareup_word);

    concat_read=concat+1;
    concat_write=concat;
    for(i=0;(i<length-1) && (concat_write->wnum != concat_read->wnum);i++) {
      concat_write++;
      concat_read++;
    }
    weight=concat_write->weight;
    for(i=i;(i<length-1);i++) {
      if(concat_write->wnum == concat_read->wnum) {
	weight+=(double)concat_read->weight;
	concat_read++;
      }
      else {
	if((weight > min_non_zero) || (weight < -min_non_zero)) {
	  concat_write->weight=weight;
	  concat_write++;
	}
	(*concat_write)=(*concat_read);
	weight=concat_write->weight;
	concat_read++;
      }
    }
    if((length>0) && ((weight > min_non_zero) || (weight < -min_non_zero))) {
      concat_write->weight=weight;
      concat_write++;
    }
    concat_write->wnum=0;

    if(1) { /* this wastes some memory, but saves malloc'ing */
      sum=create_svector_shallow(concat,NULL,1.0);
    }
    else {  /* this is more memory efficient */
      sum=create_svector(concat,NULL,1.0);
      free(concat);
    }
  }
  else {
    empty[0].wnum=0;
    sum=create_svector(empty,NULL,1.0);
  }
  return(sum);
}

SVECTOR* add_list_ns(SVECTOR *a)
{
  return(add_list_ns_r(a,0));
}
 
SVECTOR* add_dual_list_ns_r(SVECTOR *a, SVECTOR *b, double min_non_zero) 
     /* computes the linear combination of the two SVECTOR lists weighted
	by the factor of each SVECTOR */
{
  SVECTOR *f,*sum;

  for(f=a;f->next;f=f->next);  /* find end of first vector list */
  f->next=b;                   /* temporarily append the two vector lists */
  sum=add_list_ns_r(a,min_non_zero);
  f->next=NULL;                /* undo append */
  return(sum);
}

SVECTOR* add_list_ns_r(SVECTOR *a, double min_non_zero) 
     /* computes the linear combination of the SVECTOR list weighted
	by the factor of each SVECTOR. assumes that the number of
	features is small compared to the number of elements in the
	list */
{
    SVECTOR *vec,*f;
    register WORD *ai;
    long totwords;
    double *sum;

    /* find max feature number */
    totwords=0;
    for(f=a;f;f=f->next) {
      ai=f->words;
      while (ai->wnum) {
	if(totwords<ai->wnum) 
	  totwords=ai->wnum;
	ai++;
      }
    }
    sum=create_nvector(totwords);

    clear_nvector(sum,totwords);
    for(f=a;f;f=f->next)  
      add_vector_ns(sum,f,f->factor);

    vec=create_svector_n_r(sum,totwords,NULL,1.0,min_non_zero);
    free(sum);

    return(vec);
}

void add_list_n_ns(double *vec_n, SVECTOR *vec_s, double faktor)
{
  SVECTOR *f;
  for(f=vec_s;f;f=f->next)  
    add_vector_ns(vec_n,f,f->factor*faktor);
}

void append_svector_list(SVECTOR *a, SVECTOR *b) 
     /* appends SVECTOR b to the end of SVECTOR a. */
{
    SVECTOR *f;
    
    for(f=a;f->next;f=f->next);  /* find end of first vector list */
    f->next=b;                   /* append the two vector lists */
}

void mult_svector_list(SVECTOR *a, double factor) 
     /* multiplies the factor of each element in vector list with factor */
{
    SVECTOR *f;
    
    for(f=a;f;f=f->next)
      f->factor*=factor;
}

void setfactor_svector_list(SVECTOR *a, double factor) 
     /* sets the factor of each element in vector list to factor */
{
    SVECTOR *f;
    
    for(f=a;f;f=f->next)
      f->factor=factor;
}

SVECTOR* smult_s(SVECTOR *a, double factor) 
     /* scale sparse vector a by factor */
{
    SVECTOR *vec;
    register WORD *sum,*sumi;
    register WORD *ai;
    long veclength;
  
    ai=a->words;
    veclength=0;
    while (ai->wnum) {
      veclength++;
      ai++;
    }
    veclength++;

    sum=(WORD *)my_malloc(sizeof(WORD)*veclength);
    sumi=sum;
    ai=a->words;
    while (ai->wnum) {
	(*sumi)=(*ai);
	sumi->weight*=factor;
	if(sumi->weight != 0)
	  sumi++;
	ai++;
    }
    sumi->wnum=0;

    vec=create_svector_shallow(sum,a->userdefined,1.0);
    return(vec);
}

SVECTOR* shift_s(SVECTOR *a, long shift) 
     /* shifts the feature numbers by shift positions */
{
    SVECTOR *vec;
    register WORD *sum,*sumi;
    register WORD *ai;
    long veclength;
  
    ai=a->words;
    veclength=0;
    while (ai->wnum) {
      veclength++;
      ai++;
    }
    veclength++;

    sum=(WORD *)my_malloc(sizeof(WORD)*veclength);
    sumi=sum;
    ai=a->words;
    while (ai->wnum) {
	(*sumi)=(*ai);
	sumi->wnum+=shift;
	ai++;
	sumi++;
    }
    sumi->wnum=0;

  vec=create_svector_shallow(sum,a->userdefined,a->factor);
  return(vec);
}

int featvec_eq(SVECTOR *a, SVECTOR *b)
     /* tests two sparse vectors for equality */
{
    register WORD *ai,*bj;
    ai=a->words;
    bj=b->words;
    while (ai->wnum && bj->wnum) {
      if(ai->wnum > bj->wnum) {
	if((bj->weight) != 0)
	  return(0);
	bj++;
      }
      else if (ai->wnum < bj->wnum) {
	if((ai->weight) != 0)
	  return(0);
	ai++;
      }
      else {
	if((ai->weight) != (bj->weight)) 
	  return(0);
	ai++;
	bj++;
      }
    }
    return(1);
}

double model_length_s(MODEL *model) 
     /* compute length of weight vector */
{
  register long i,j;
  register double sum=0,alphai;
  register DOC *supveci;
  KERNEL_PARM *kernel_parm=&(model->kernel_parm);

  for(i=1;i<model->sv_num;i++) {  
    alphai=model->alpha[i];
    supveci=model->supvec[i];
    for(j=1;j<model->sv_num;j++) {
      sum+=alphai*model->alpha[j]
	   *kernel(kernel_parm,supveci,model->supvec[j]);
    }
  }
  return(sqrt(sum));
}

double model_length_n(MODEL *model) 
     /* compute length of weight vector */
{
  long     i,totwords=model->totwords+1;
  double   sum,*weight_n;
  SVECTOR  *weight;

  if(model->kernel_parm.kernel_type != LINEAR) {
    printf("ERROR: model_length_n applies only to linear kernel!\n");
    exit(1);
  }
  weight_n=create_nvector(totwords);
  clear_nvector(weight_n,totwords);
  for(i=1;i<model->sv_num;i++) 
    add_list_n_ns(weight_n,model->supvec[i]->fvec,model->alpha[i]);
  weight=create_svector_n(weight_n,totwords,NULL,1.0);
  sum=sprod_ss(weight,weight);
  free(weight_n);
  free_svector(weight);
  return(sqrt(sum));
}

void mult_vector_ns(double *vec_n, SVECTOR *vec_s, double faktor)
{
  register WORD *ai;
  ai=vec_s->words;
  while (ai->wnum) {
    vec_n[ai->wnum]*=(faktor*(double)ai->weight);
    ai++;
  }
}

void add_vector_ns(double *vec_n, SVECTOR *vec_s, double faktor)
{
  /* Note: SVECTOR lists are not followed, but only the first
           SVECTOR is used */
  register WORD *ai;
  ai=vec_s->words;
  while (ai->wnum) {
    vec_n[ai->wnum]+=(faktor*(double)ai->weight);
    ai++;
  }
}

double sprod_ns(double *vec_n, SVECTOR *vec_s)
{
  register double sum=0;
  register WORD *ai;
  ai=vec_s->words;
  while (ai->wnum) {
    sum+=(vec_n[ai->wnum]*(double)ai->weight);
    ai++;
  }
  return(sum);
}

void add_weight_vector_to_linear_model(MODEL *model)
     /* compute weight vector in linear case and add to model */
{
  long i;
  SVECTOR *f;

  model->lin_weights=create_nvector(model->totwords);
  clear_nvector(model->lin_weights,model->totwords);
  for(i=1;i<model->sv_num;i++) {
    for(f=(model->supvec[i])->fvec;f;f=f->next)  
      add_vector_ns(model->lin_weights,f,f->factor*model->alpha[i]);
  }
}


DOC *create_example(long docnum, long queryid, long slackid, 
		    double costfactor, SVECTOR *fvec)
{
  DOC *example;
  example = (DOC *)my_malloc(sizeof(DOC));
  example->docnum=docnum;
  example->kernelid=docnum;
  example->queryid=queryid;
  example->slackid=slackid;
  example->costfactor=costfactor;
  example->fvec=fvec;
  return(example);
}

void free_example(DOC *example, long deep)
{
  if(example) {
    if(deep) {
      if(example->fvec)
	free_svector(example->fvec);
    }
    free(example);
  }
}


int compare_randpair(const void *a, const void *b) 
{
  long va,vb;
  va=((RANDPAIR *)a)->sort;
  vb=((RANDPAIR *)b)->sort;
  return((va > vb) - (va < vb));
}

long *random_order(long n)
     /* creates an array of the integers [0..n-1] in random order */ 
{
  long *randarray=(long *)my_malloc(sizeof(long)*n);
  RANDPAIR *randpair=(RANDPAIR *)my_malloc(sizeof(RANDPAIR)*n);
  long i;

  for(i=0;i<n;i++) {
    randpair[i].val=i;
    randpair[i].sort=rand();
  }
  qsort(randpair,n,sizeof(RANDPAIR),compare_randpair);
  for(i=0;i<n;i++) {
    randarray[i]=randpair[i].val;
  }
  free(randpair);
  return(randarray);
}

void print_percent_progress(long *progress, long maximum, 
			    long percentperdot, char *symbol)
     /* every time this function gets called, progress is
	incremented. It prints symbol every percentperdot calls,
	assuming that maximum is the max number of calls */
{
  if((percentperdot*(*progress-1)/maximum) 
     != (percentperdot*(*progress)/maximum)) {
    printf(symbol);
    fflush(stdout);
  }
  (*progress)++;
}

/************ Some useful dense vector and matrix routines ****************/

MATRIX *create_matrix(int n, int m)
/* create matrix with n rows and m colums */
{
  int i;
  MATRIX *matrix;
  
  matrix=(MATRIX*)my_malloc(sizeof(MATRIX));
  matrix->n=n;
  matrix->m=m;
  matrix->element=(double **)my_malloc(sizeof(double *)*n);
  for(i=0;i<n;i++) {
    matrix->element[i]=(double *)my_malloc(sizeof(double)*m);
  }
  return(matrix);
}

MATRIX *realloc_matrix(MATRIX *matrix, int n, int m)
/* extends/shrinks matrix to n rows and m colums. Not that added elements are
   not initialized. */
{
  int i;

  if(!matrix) 
    return(create_matrix(n,m));

  for(i=n;i<matrix->n;i++) 
    free(matrix->element[i]);
  matrix->element=(double **)realloc(matrix->element,sizeof(double *)*n);
  for(i=matrix->n;i<n;i++) 
    matrix->element[i]=(double *)my_malloc(sizeof(double)*m);
  for(i=0;i<MIN(n,matrix->n);i++) {
    matrix->element[i]=(double *)realloc(matrix->element[i],sizeof(double)*m);
  }
  matrix->n=n;
  matrix->m=m;
  return(matrix);
}

double *create_nvector(int n)
/* creates a dense column vector with n+1 rows. unfortunately, there
   is part of the code that starts counting at 0, while the sparse
   vectors start counting at 1. So, it always allocates one extra
   row. */
{
  double *vector;
  
  vector=(double *)my_malloc(sizeof(double)*(n+1));

  return(vector);
}

void clear_nvector(double *vec, long int n)
{
  register long i;
  for(i=0;i<=n;i++) vec[i]=0;
}

MATRIX *copy_matrix(MATRIX *matrix)
/* create deep copy of matrix */
{
  int i,j;
  MATRIX *copy;
  
  copy=create_matrix(matrix->n,matrix->m);
  for(i=0;i<matrix->n;i++) {
    for(j=0;j<matrix->m;j++) {
      copy->element[i][j]=matrix->element[i][j];
    }
  }
  return(copy);
}

void free_matrix(MATRIX *matrix) 
/* deallocates memory */
{
  int i;

  for(i=0;i<matrix->n;i++) {
    free(matrix->element[i]);
  }
  free(matrix->element);
  free(matrix);
}

void free_nvector(double *vector) 
/* deallocates memory */
{
  free(vector);
}

MATRIX *transpose_matrix(MATRIX *matrix)
/* returns copy with transpose of matrix */
{
  int i,j;
  MATRIX *copy;
  
  copy=create_matrix(matrix->m,matrix->n);
  for(i=0;i<matrix->n;i++) {
    for(j=0;j<matrix->m;j++) {
      copy->element[j][i]=matrix->element[i][j];
    }
  }
  return(copy);
}


MATRIX *cholesky_matrix(MATRIX *A)
/* Given a positive-definite symmetric matrix A[0..n-1][0..n-1], this routine constructs its Cholesky decomposition, A = L ∑ LT . On input, only the upper triangle of A need be given; A is not modified. The Cholesky factor L is returned in the lower triangle. */ 
{
  int i,j,k,n;
  double sum;
  MATRIX *L;
  
  if(A->m != A->n) {
    printf("ERROR: Matrix not quadratic. Cannot compute Cholesky!\n");
    exit(1);
  }
  n=A->n;
  L=copy_matrix(A);

  for (i=0;i<n;i++) {
    for (j=i;j<n;j++) {
      for (sum=L->element[i][j],k=i-1;k>=0;k--) 
	sum -= L->element[i][k]*L->element[j][k];
      if (i == j) {
	if (sum <= 0.0) printf("Cholesky: Matrix not positive definite");
	L->element[i][i]=sqrt(sum);
      } 
      else L->element[j][i]=sum/L->element[i][i];
    }
  }
  /* set upper triange to zero */
  for (i=0;i<n;i++) 
    for (j=i+1;j<n;j++) 
      L->element[i][j]=0;

  return(L);
}

double *find_indep_subset_of_matrix(MATRIX *A, double epsilon)
/* Given a positive-semidefinite symmetric matrix A[0..n-1][0..n-1], this routine finds a subset of rows and colums that is linear independent. To do this, it constructs the Cholesky decomposition, A = L ∑ LT. On input, only the upper triangle of A need be given; A is not modified. The routine returns a vector in which non-zero elements indicate the linear independent subset. epsilon is the amount by which the diagonal entry of L has to be greater than zero. */ 
{
  int i,j,k,n;
  double sum,*indep;
  MATRIX *L;
  
  if(A->m != A->n) {
    printf("ERROR: Matrix not quadratic. Cannot compute Cholesky!\n");
    exit(1);
  }
  n=A->n;
  L=copy_matrix(A);

  for (i=0;i<n;i++) {
    for (j=i;j<n;j++) {
      for (sum=L->element[i][j],k=i-1;k>=0;k--) 
	sum -= L->element[i][k]*L->element[j][k];
      if (i == j) {
	if (sum <= epsilon) sum=0;
	L->element[i][i]=sqrt(sum);
      } 
      else 
	if(L->element[i][i] == 0)
	  L->element[j][i]=0;
	else
	  L->element[j][i]=sum/L->element[i][i];
    }
  }
  /* Gather non-zero diagonal elements */
  indep=create_nvector(n);
  for (i=0;i<n;i++) 
      indep[i]=L->element[i][i];

  free_matrix(L);
  return(indep);
}


MATRIX *invert_ltriangle_matrix(MATRIX *L)
/* Given a lower triangular matrix L, computes inverse L^-1 */
{
  int i,j,k,n;
  double sum;
  MATRIX *I;
  
  if(L->m != L->n) {
    printf("ERROR: Matrix not quadratic. Cannot invert triangular matrix!\n");
    exit(1);
  }
  n=L->n;
  I=copy_matrix(L);

  for (i=0;i<n;i++) {
    I->element[i][i]=1.0/L->element[i][i];
    for (j=i+1;j<n;j++) {
      sum=0.0;
      for (k=i;k<j;k++) sum -= I->element[j][k]*I->element[k][i];
      I->element[j][i]=sum/L->element[j][j];
    }
  }

  return(I);
}

double *prod_nvector_matrix(double *v, MATRIX *A)
/* For column vector v and matrix A (assumed to match in size), computes w^T=v^T*A */
{
  int i,j;
  double sum;
  double *w;
  
  w=create_nvector(A->m);

  for (i=0;i<A->m;i++) {
    sum=0.0;
    for (j=0;j<A->n;j++) {
      sum+=v[j]*A->element[j][i];
    }
    w[i]=sum;
  }

  return(w);
}

double *prod_matrix_nvector(MATRIX *A, double *v)
/* For column vector v and matrix A (assumed to match in size), computes w=A*v */
{
  int i,j;
  double sum;
  double *w;
  
  w=create_nvector(A->n);

  for (i=0;i<A->n;i++) {
    sum=0.0;
    for (j=0;j<A->m;j++) {
      sum+=v[j]*A->element[i][j];
    }
    w[i]=sum;
  }

  return(w);
}

double *prod_nvector_ltmatrix(double *v, MATRIX *A)
/* For column vector v and a lower triangular matrix A (assumed to
   match in size), computes w^T=v^T*A */
{
  int i,j;
  double sum;
  double *w;
  
  w=create_nvector(A->m);

  for (i=0;i<A->m;i++) {
    sum=0.0;
    for (j=i;j<A->n;j++) {
      sum+=v[j]*A->element[j][i];
    }
    w[i]=sum;
  }

  return(w);
}

double *prod_ltmatrix_nvector(MATRIX *A, double *v)
/* For column vector v and lower triangular matrix A (assumed to match
   in size), computes w=A*v */
{
  int i,j;
  double sum;
  double *w;
  
  w=create_nvector(A->n);

  for (i=0;i<A->n;i++) {
    sum=0.0;
    for (j=0;j<=i;j++) {
      sum+=v[j]*A->element[i][j];
    }
    w[i]=sum;
  }

  return(w);
}

MATRIX *prod_matrix_matrix(MATRIX *A, MATRIX *B)
/* For matrices A and B (assumed to match in size), computes C=A*B */
{
  int i,j,k;
  double sum;
  MATRIX *C;
  
  if(A->m != B->n) {
    printf("ERROR: Matrix size does not match. Cannot compute product!\n");
    exit(1);
  }
  C=create_matrix(A->n,B->m);

  for (i=0;i<A->n;i++) {
    for (j=0;j<B->m;j++) {
      sum=0.0;
      for (k=0;k<A->m;k++) {
	sum+=A->element[i][k]*B->element[k][j];
      }
      C->element[i][j]=sum;
    }
  }

  return(C);
}

void print_matrix(MATRIX *matrix)
/* prints matrix to STDOUT */
{
  int i,j;

  printf("\n");
  printf("\n");
  for(i=0;i<matrix->n;i++) {
    for(j=0;j<matrix->m;j++) {
      printf("%4.3f\t",matrix->element[i][j]);
    }
    printf("\n");
  }
}

/***************************** IO routines ***************************/
#if 0
void write_model(char *modelfile, MODEL *model)
{
  FILE *modelfl;
  long j,i,sv_num;
  SVECTOR *v;
  MODEL *compact_model=NULL;
 
  if(verbosity>=1) {
    printf("Writing model file..."); fflush(stdout);
  }

  /* Replace SV with single weight vector */
  if(0 && model->kernel_parm.kernel_type == LINEAR) {
    if(verbosity>=1) {
      printf("(compacting..."); fflush(stdout);
    }
    compact_model=compact_linear_model(model);
    model=compact_model;
    if(verbosity>=1) {
      printf("done)"); fflush(stdout);
    }
  }

  if ((modelfl = fopen (modelfile, "w")) == NULL)
  { perror (modelfile); exit (1); }
  fprintf(modelfl,"SVM-light Version %s\n",VERSION);
  fprintf(modelfl,"%ld # kernel type\n",
	  model->kernel_parm.kernel_type);
  fprintf(modelfl,"%ld # kernel parameter -d \n",
	  model->kernel_parm.poly_degree);
  fprintf(modelfl,"%.8g # kernel parameter -g \n",
	  model->kernel_parm.rbf_gamma);
  fprintf(modelfl,"%.8g # kernel parameter -s \n",
	  model->kernel_parm.coef_lin);
  fprintf(modelfl,"%.8g # kernel parameter -r \n",
	  model->kernel_parm.coef_const);
  fprintf(modelfl,"%s# kernel parameter -u \n",model->kernel_parm.custom);
  fprintf(modelfl,"%ld # highest feature index \n",model->totwords);
  fprintf(modelfl,"%ld # number of training documents \n",model->totdoc);
 
  sv_num=1;
  for(i=1;i<model->sv_num;i++) {
    for(v=model->supvec[i]->fvec;v;v=v->next) 
      sv_num++;
  }
  fprintf(modelfl,"%ld # number of support vectors plus 1 \n",sv_num);
  fprintf(modelfl,"%.8g # threshold b, each following line is a SV (starting with alpha*y)\n",model->b);

  for(i=1;i<model->sv_num;i++) {
    for(v=model->supvec[i]->fvec;v;v=v->next) {
      fprintf(modelfl,"%.32g ",model->alpha[i]*v->factor);
      for (j=0; (v->words[j]).wnum; j++) {
	fprintf(modelfl,"%ld:%.8g ",
		(long)(v->words[j]).wnum,
		(double)(v->words[j]).weight);
      }
      if(v->userdefined)
	fprintf(modelfl,"#%s\n",v->userdefined);
      else
	fprintf(modelfl,"#\n");
    /* NOTE: this could be made more efficient by summing the
       alpha's of identical vectors before writing them to the
       file. */
    }
  }
  fclose(modelfl);
  if(compact_model)
    free_model(compact_model,1);
  if(verbosity>=1) {
    printf("done\n");
  }
}
#endif

#if 0
MODEL *read_model(char *modelfile)
{
  FILE *modelfl;
  long i,queryid,slackid;
  double costfactor;
  long max_sv,max_words,ll,wpos;
  char *line,*comment;
  WORD *words;
  char version_buffer[100];
  MODEL *model;

  if(verbosity>=1) {
    printf("Reading model..."); fflush(stdout);
  }

  nol_ll(modelfile,&max_sv,&max_words,&ll); /* scan size of model file */
  max_words+=2;
  ll+=2;

  words = (WORD *)my_malloc(sizeof(WORD)*(max_words+10));
  line = (char *)my_malloc(sizeof(char)*ll);
  model = (MODEL *)my_malloc(sizeof(MODEL));

  if ((modelfl = fopen (modelfile, "r")) == NULL)
  { perror (modelfile); exit (1); }

  fscanf(modelfl,"SVM-light Version %s\n",version_buffer);
  if(strcmp(version_buffer,VERSION)) {
    perror ("Version of model-file does not match version of svm_classify!"); 
    exit (1); 
  }
  fscanf(modelfl,"%ld%*[^\n]\n", &model->kernel_parm.kernel_type);  
  fscanf(modelfl,"%ld%*[^\n]\n", &model->kernel_parm.poly_degree);
  fscanf(modelfl,"%lf%*[^\n]\n", &model->kernel_parm.rbf_gamma);
  fscanf(modelfl,"%lf%*[^\n]\n", &model->kernel_parm.coef_lin);
  fscanf(modelfl,"%lf%*[^\n]\n", &model->kernel_parm.coef_const);
  fscanf(modelfl,"%[^#]%*[^\n]\n", model->kernel_parm.custom);

  fscanf(modelfl,"%ld%*[^\n]\n", &model->totwords);
  fscanf(modelfl,"%ld%*[^\n]\n", &model->totdoc);
  fscanf(modelfl,"%ld%*[^\n]\n", &model->sv_num);
  fscanf(modelfl,"%lf%*[^\n]\n", &model->b);

  model->supvec = (DOC **)my_malloc(sizeof(DOC *)*model->sv_num);
  model->alpha = (double *)my_malloc(sizeof(double)*model->sv_num);
  model->index=NULL;
  model->lin_weights=NULL;

  for(i=1;i<model->sv_num;i++) {
    fgets(line,(int)ll,modelfl);
    if(!parse_document(line,words,&(model->alpha[i]),&queryid,&slackid,
		       &costfactor,&wpos,max_words,&comment)) {
      printf("\nParsing error while reading model file in SV %ld!\n%s",
	     i,line);
      exit(1);
    }
    model->supvec[i] = create_example(-1,
				      0,0,
				      0.0,
				      create_svector(words,comment,1.0));
  }
  fclose(modelfl);
  free(line);
  free(words);
  if(verbosity>=1) {
    fprintf(stdout, "OK. (%d support vectors read)\n",(int)(model->sv_num-1));
  }
  return(model);
}
#endif

MODEL *copy_model(MODEL *model)
{
  MODEL *newmodel;
  long  i;

  newmodel=(MODEL *)my_malloc(sizeof(MODEL));
  (*newmodel)=(*model);
  newmodel->supvec = (DOC **)my_malloc(sizeof(DOC *)*model->sv_num);
  newmodel->alpha = (double *)my_malloc(sizeof(double)*model->sv_num);
  newmodel->index = NULL; /* index is not copied */
  newmodel->supvec[0] = NULL;
  newmodel->alpha[0] = 0;
  for(i=1;i<model->sv_num;i++) {
    newmodel->alpha[i]=model->alpha[i];
    newmodel->supvec[i]=create_example(model->supvec[i]->docnum,
				       model->supvec[i]->queryid,0,
				       model->supvec[i]->costfactor,
				       copy_svector(model->supvec[i]->fvec));
  }
  if(model->lin_weights) {
    newmodel->lin_weights = (double *)my_malloc(sizeof(double)*(model->totwords+1));
    for(i=0;i<model->totwords+1;i++) 
      newmodel->lin_weights[i]=model->lin_weights[i];
  }
  return(newmodel);
}

MODEL *compact_linear_model(MODEL *model)
     /* Makes a copy of model where the support vectors are replaced
	with a single linear weight vector. */
     /* NOTE: It adds the linear weight vector also to newmodel->lin_weights */
     /* WARNING: This is correct only for linear models! */
{
  MODEL *newmodel;

  newmodel=(MODEL *)my_malloc(sizeof(MODEL));
  (*newmodel)=(*model);
  add_weight_vector_to_linear_model(newmodel);
  newmodel->supvec = (DOC **)my_malloc(sizeof(DOC *)*2);
  newmodel->alpha = (double *)my_malloc(sizeof(double)*2);
  newmodel->index = NULL; /* index is not copied */
  newmodel->supvec[0] = NULL;
  newmodel->alpha[0] = 0.0;
  newmodel->supvec[1] = create_example(-1,0,0,0,
				       create_svector_n(newmodel->lin_weights,
							newmodel->totwords,
							NULL,1.0));
  newmodel->alpha[1] = 1.0;
  newmodel->sv_num=2;

  return(newmodel);
}

void free_model(MODEL *model, int deep)
{
  long i;

  if(model->supvec) {
    if(deep) {
      for(i=1;i<model->sv_num;i++) {
	free_example(model->supvec[i],1);
      }
    }
    free(model->supvec);
  }
  if(model->alpha) free(model->alpha);
  if(model->index) free(model->index);
  if(model->lin_weights) free(model->lin_weights);
  free(model);
}

#if 0
void read_documents(char *docfile, DOC ***docs, double **label, 
		    long int *totwords, long int *totdoc)
{
  char *line,*comment;
  WORD *words;
  long dnum=0,wpos,dpos=0,dneg=0,dunlab=0,queryid,slackid,max_docs;
  long max_words_doc, ll;
  double doc_label,costfactor;
  FILE *docfl;

  if(verbosity>=1) {
    printf("Scanning examples..."); fflush(stdout);
  }
  nol_ll(docfile,&max_docs,&max_words_doc,&ll); /* scan size of input file */
  max_words_doc+=2;
  ll+=2;
  max_docs+=2;
  if(verbosity>=1) {
    printf("done\n"); fflush(stdout);
  }

  (*docs) = (DOC **)my_malloc(sizeof(DOC *)*max_docs);    /* feature vectors */
  (*label) = (double *)my_malloc(sizeof(double)*max_docs); /* target values */
  line = (char *)my_malloc(sizeof(char)*ll);

  if ((docfl = fopen (docfile, "r")) == NULL)
  { perror (docfile); exit (1); }

  words = (WORD *)my_malloc(sizeof(WORD)*(max_words_doc+10));
  if(verbosity>=1) {
    printf("Reading examples into memory..."); fflush(stdout);
  }
  dnum=0;
  (*totwords)=0;
  while((!feof(docfl)) && fgets(line,(int)ll,docfl)) {
    if(line[0] == '#') continue;  /* line contains comments */
    if(!parse_document(line,words,&doc_label,&queryid,&slackid,&costfactor,
		       &wpos,max_words_doc,&comment)) {
      printf("\nParsing error in line %ld!\n%s",dnum,line);
      exit(1);
    }
    (*label)[dnum]=doc_label;
    /* printf("docnum=%ld: Class=%f ",dnum,doc_label); */
    if(doc_label > 0) dpos++;
    if (doc_label < 0) dneg++;
    if (doc_label == 0) dunlab++;
    if((wpos>1) && ((words[wpos-2]).wnum>(*totwords))) 
      (*totwords)=(words[wpos-2]).wnum;
    if((*totwords) > MAXFEATNUM) {
      printf("\nMaximum feature number exceeds limit defined in MAXFEATNUM!\n");
      printf("LINE: %s\n",line);
      exit(1);
    }
    (*docs)[dnum] = create_example(dnum,queryid,slackid,costfactor,
				   create_svector(words,comment,1.0));
    /* printf("\nNorm=%f\n",((*docs)[dnum]->fvec)->twonorm_sq);  */
    dnum++;  
    if(verbosity>=1) {
      if((dnum % 100) == 0) {
	printf("%ld..",dnum); fflush(stdout);
      }
    }
  } 

  fclose(docfl);
  free(line);
  free(words);
  if(verbosity>=1) {
    fprintf(stdout, "OK. (%ld examples read)\n", dnum);
  }
  (*totdoc)=dnum;
}

int parse_document(char *line, WORD *words, double *label,
		   long *queryid, long *slackid, double *costfactor,
		   long int *numwords, long int max_words_doc,
		   char **comment)
{
  register long wpos,pos;
  long wnum;
  double weight;
  char featurepair[1000],junk[1000];

  (*queryid)=0;
  (*slackid)=0;
  (*costfactor)=1;

  pos=0;
  (*comment)=NULL;
  while(line[pos] ) {      /* cut off comments */
    if((line[pos] == '#') && (!(*comment))) {
      line[pos]=0;
      (*comment)=&(line[pos+1]);
    }
    if(line[pos] == '\n') { /* strip the CR */
      line[pos]=0;
    }
    pos++;
  }
  if(!(*comment)) (*comment)=&(line[pos]);
  /* printf("Comment: '%s'\n",(*comment)); */

  wpos=0;
  /* check, that line starts with target value or zero, but not with
     feature pair */
  if(sscanf(line,"%s",featurepair) == EOF) return(0);
  pos=0;
  while((featurepair[pos] != ':') && featurepair[pos]) pos++;
  if(featurepair[pos] == ':') {
	perror ("Line must start with label or 0!!!\n"); 
	printf("LINE: %s\n",line);
	exit (1); 
  }
  /* read the target value */
  if(sscanf(line,"%lf",label) == EOF) return(0);
  pos=0;
  while(space_or_null((int)line[pos])) pos++;
  while((!space_or_null((int)line[pos])) && line[pos]) pos++;
  while((pos+=read_word(line+pos,featurepair)) &&
	(featurepair[0]) && 
	(wpos<max_words_doc)) {
    /* printf("%s\n",featurepair); */
    if(sscanf(featurepair,"qid:%ld%s",&wnum,junk)==1) {
      /* it is the query id */
      (*queryid)=(long)wnum;
    }
    else if(sscanf(featurepair,"sid:%ld%s",&wnum,junk)==1) {
      /* it is the slack id */
      if(wnum > 0) 
	(*slackid)=(long)wnum;
      else {
	perror ("Slack-id must be greater or equal to 1!!!\n"); 
	printf("LINE: %s\n",line);
	exit (1); 
      }
    }
    else if(sscanf(featurepair,"cost:%lf%s",&weight,junk)==1) {
      /* it is the example-dependent cost factor */
      (*costfactor)=(double)weight;
    }
    else if(sscanf(featurepair,"%ld:%lf%s",&wnum,&weight,junk)==2) {
      /* it is a regular feature */
      if(wnum<=0) { 
	perror ("Feature numbers must be larger or equal to 1!!!\n"); 
	printf("LINE: %s\n",line);
	exit (1); 
      }
      if((wpos>0) && ((words[wpos-1]).wnum >= wnum)) { 
	perror ("Features must be in increasing order!!!\n"); 
	printf("LINE: %s\n",line);
	exit (1); 
      }
      (words[wpos]).wnum=wnum;
      (words[wpos]).weight=(FVAL)weight; 
      wpos++;
    }
    else {
      perror ("Cannot parse feature/value pair!!!\n"); 
      printf("'%s' in LINE: %s\n",featurepair,line);
      exit (1); 
    }
  }
  (words[wpos]).wnum=0;
  (*numwords)=wpos+1;
  return(1);
}

double *read_alphas(char *alphafile,long totdoc)
     /* reads the alpha vector from a file as written by the
        write_alphas function */
{
  FILE *fl;
  double *alpha;
  long dnum;

  if ((fl = fopen (alphafile, "r")) == NULL)
  { perror (alphafile); exit (1); }

  alpha = (double *)my_malloc(sizeof(double)*totdoc);
  if(verbosity>=1) {
    printf("Reading alphas..."); fflush(stdout);
  }
  dnum=0;
  while((!feof(fl)) && fscanf(fl,"%lf\n",&alpha[dnum]) && (dnum<totdoc)) {
    dnum++;
  }
  if(dnum != totdoc)
  { perror ("\nNot enough values in alpha file!"); exit (1); }
  fclose(fl);

  if(verbosity>=1) {
    printf("done\n"); fflush(stdout);
  }

  return(alpha);
}
#endif

void set_learning_defaults(LEARN_PARM *learn_parm, KERNEL_PARM *kernel_parm)
{
  learn_parm->type=CLASSIFICATION;
  strcpy (learn_parm->predfile, "trans_predictions");
  strcpy (learn_parm->alphafile, "");
  learn_parm->biased_hyperplane=1;
  learn_parm->sharedslack=0;
  learn_parm->remove_inconsistent=0;
  learn_parm->skip_final_opt_check=0;
  learn_parm->svm_maxqpsize=10;
  learn_parm->svm_newvarsinqp=0;
  learn_parm->svm_iter_to_shrink=-9999;
  learn_parm->maxiter=100000;
  learn_parm->kernel_cache_size=40;
  learn_parm->svm_c=0.0;
  learn_parm->eps=0.1;
  learn_parm->transduction_posratio=-1.0;
  learn_parm->svm_costratio=1.0;
  learn_parm->svm_costratio_unlab=1.0;
  learn_parm->svm_unlabbound=1E-5;
  learn_parm->epsilon_crit=0.001;
  learn_parm->epsilon_a=1E-15;
  learn_parm->compute_loo=0;
  learn_parm->rho=1.0;
  learn_parm->xa_depth=0;
  kernel_parm->kernel_type=LINEAR;
  kernel_parm->poly_degree=3;
  kernel_parm->rbf_gamma=1.0;
  kernel_parm->coef_lin=1;
  kernel_parm->coef_const=1;
  strcpy(kernel_parm->custom,"empty");
}

int check_learning_parms(LEARN_PARM *learn_parm, KERNEL_PARM *kernel_parm)
{
  if((learn_parm->skip_final_opt_check) 
     && (kernel_parm->kernel_type == LINEAR)) {
    printf("\nIt does not make sense to skip the final optimality check for linear kernels.\n\n");
    learn_parm->skip_final_opt_check=0;
  }    
  if((learn_parm->skip_final_opt_check) 
     && (learn_parm->remove_inconsistent)) {
    printf("\nIt is necessary to do the final optimality check when removing inconsistent \nexamples.\n");
    return(0);
  }    
  if((learn_parm->svm_maxqpsize<2)) {
    printf("\nMaximum size of QP-subproblems not in valid range: %ld [2..]\n",learn_parm->svm_maxqpsize); 
    return(0);
  }
  if((learn_parm->svm_maxqpsize<learn_parm->svm_newvarsinqp)) {
    printf("\nMaximum size of QP-subproblems [%ld] must be larger than the number of\n",learn_parm->svm_maxqpsize); 
    printf("new variables [%ld] entering the working set in each iteration.\n",learn_parm->svm_newvarsinqp); 
    return(0);
  }
  if(learn_parm->svm_iter_to_shrink<1) {
    printf("\nMaximum number of iterations for shrinking not in valid range: %ld [1,..]\n",learn_parm->svm_iter_to_shrink);
    return(0);
  }
  if(learn_parm->svm_c<0) {
    printf("\nThe C parameter must be greater than zero!\n\n");
    return(0);
  }
  if(learn_parm->transduction_posratio>1) {
    printf("\nThe fraction of unlabeled examples to classify as positives must\n");
    printf("be less than 1.0 !!!\n\n");
    return(0);
  }
  if(learn_parm->svm_costratio<=0) {
    printf("\nThe COSTRATIO parameter must be greater than zero!\n\n");
    return(0);
  }
  if(learn_parm->epsilon_crit<=0) {
    printf("\nThe epsilon parameter must be greater than zero!\n\n");
    return(0);
  }
  if(learn_parm->rho<0) {
    printf("\nThe parameter rho for xi/alpha-estimates and leave-one-out pruning must\n");
    printf("be greater than zero (typically 1.0 or 2.0, see T. Joachims, Estimating the\n");
    printf("Generalization Performance of an SVM Efficiently, ICML, 2000.)!\n\n");
    return(0);
  }
  if((learn_parm->xa_depth<0) || (learn_parm->xa_depth>100)) {
    printf("\nThe parameter depth for ext. xi/alpha-estimates must be in [0..100] (zero\n");
    printf("for switching to the conventional xa/estimates described in T. Joachims,\n");
    printf("Estimating the Generalization Performance of an SVM Efficiently, ICML, 2000.)\n");
  }
  return(1);
}

void nol_ll(char *file, long int *nol, long int *wol, long int *ll) 
     /* Grep through file and count number of lines, maximum number of
        spaces per line, and longest line. */
{
  FILE *fl;
  int ic;
  char c;
  long current_length,current_wol;

  if ((fl = fopen (file, "r")) == NULL)
  { perror (file); exit (1); }
  current_length=0;
  current_wol=0;
  (*ll)=0;
  (*nol)=1;
  (*wol)=0;
  while((ic=getc(fl)) != EOF) {
    c=(char)ic;
    current_length++;
    if(space_or_null((int)c)) {
      current_wol++;
    }
    if(c == '\n') {
      (*nol)++;
      if(current_length>(*ll)) {
	(*ll)=current_length;
      }
      if(current_wol>(*wol)) {
	(*wol)=current_wol;
      }
      current_length=0;
      current_wol=0;
    }
  }
  fclose(fl);
}

long minl(long int a, long int b)
{
  if(a<b)
    return(a);
  else
    return(b);
}

long maxl(long int a, long int b)
{
  if(a>b)
    return(a);
  else
    return(b);
}

double get_runtime(void)
{
  /* returns the current processor time in hundredth of a second */
  clock_t start;
  start = clock();
  return((double)start/((double)(CLOCKS_PER_SEC)/100.0));
}


# if defined _MSC_VER &&  _MSC_VER < 1800 

int isnan(double a)
{
  return(_isnan(a));
}

# endif 

int space_or_null(int c) {
  if (c==0)
    return 1;
  return isspace((unsigned char)c);
}

int read_word(char *in, char *out) {
  int found=0;
  while(isspace((int)(*in)) && (*in)) { /* skip over whitespace */
    in++;
    found++;
  }
  while(!space_or_null((int)(*in))) {   /* read non-whitespace string */
       (*out)=(*in);
    in++;
    found++;
    out++;
  }
  (*out)=0;
  return(found);
}

void *my_malloc(size_t size)
{
  void *ptr;
  if(size<=0) size=1; /* for AIX compatibility */
  ptr=(void *)malloc(size);
  if(!ptr) { 
    perror ("Out of memory!\n"); 
    exit (1); 
  }
  return(ptr);
}

void copyright_notice(void)
{
  printf("\nCopyright: Thorsten Joachims, thorsten@joachims.org\n\n");
  printf("This software is available for non-commercial use only. It must not\n");
  printf("be modified and distributed without prior permission of the author.\n");
  printf("The author is not responsible for implications from the use of this\n");
  printf("software.\n\n");
}
