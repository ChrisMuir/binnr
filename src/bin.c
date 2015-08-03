#include "R.h"
#include "stdio.h"
#include "variable.h"
#include "queue.h"
#include "xtab.h"
#include "bin.h"
#include "limits.h"

#define RETURN_R

// called from R and handles passing of data to and from
SEXP bin(SEXP x, SEXP y, SEXP miniv, SEXP mincnt, SEXP maxbin, SEXP monotonicity) {
  
  double *dx = REAL(x);
  double *dy = REAL(y);
  double min_iv = *REAL(miniv);
  int min_cnt = *INTEGER(mincnt), max_bin = *INTEGER(maxbin);
  int mono = *INTEGER(monotonicity);

  struct variable* v = variable_factory(dx, LENGTH(x));
  struct xtab* xtab = xtab_factory(v, dy); // create the xtab
  
  struct queue* q = queue_factory(); // create the queue
  struct work w = {0, xtab->size - 1}; // last index is one less than the size
  enqueue(q, w);
  
  // create a vector to store the split rows and init to zero
  size_t* breaks = calloc(xtab->size, sizeof(size_t));
  double* grand_tots = get_xtab_totals(xtab, 0, xtab->size);
  int num_bins = 1;
  size_t pivot = INT_MIN;
  //size_t pivot = 400;
  double pivot_tot_asc[2] = {0,0};
  double pivot_tot_dsc[2] = {0,0};
  
  // bin the variable until it's done
  while(!is_empty(q)) {
    find_best_split(q, xtab, breaks, grand_tots, &num_bins, min_iv, min_cnt, max_bin, mono, &pivot, pivot_tot_asc, pivot_tot_dsc);
  }

  // return breaks in an R object
#ifdef RETURN_R
  SEXP out = PROTECT(allocVector(REALSXP, num_bins + 1));
  size_t j = 0;
  REAL(out)[0] = R_NegInf;
  for(size_t i = 0; i < xtab->size; i++) {
    if (breaks[i] == 1) {
      j++;
      REAL(out)[j] = xtab->values[i];
    }
    REAL(out)[j + 1] = R_PosInf;
  }
#endif 
  
  // Release resources
  release_variable(v);
  release_xtab(xtab);
  release_queue(q);
  free(breaks);
  free(grand_tots);
  
#ifdef RETURN_R 
  UNPROTECT(1);
  return out;
#endif
  
  return R_NilValue;
}

void find_best_split(
  struct queue* q,
  struct xtab* xtab,
  size_t* breaks,
  double* grand_tot,
  int* num_bins,
  double min_iv,
  int min_cnt,
  int max_bin,
  int mono,
  size_t* pivot,
  double pivot_tot_asc[2],
  double pivot_tot_dsc[2]
) {
  
  struct work w = dequeue(q); // take work from queue
  double* tot = get_xtab_totals(xtab, w.start, w.stop + 1);
  double asc_zero = 0;
  double asc_ones = 0;
  double dsc_zero = 0;
  double dsc_ones = 0;
  double best_iv = -1;
  int valid = 0;
  int woe_sign = 0;
  int mono_sign = (mono > 0) ? 1 : -1;
  size_t best_split_idx;
  int best_woe_sign = 0;
  struct iv iv, pivot_iv;
  
  Rprintf("New Split: %d, %d\n", w.start, w.stop);
  
  for (size_t i = w.start; i <= w.stop; i++) {
    valid = 0;
    
    asc_zero += xtab->zero_ct[i];
    asc_ones += xtab->ones_ct[i];
    
    dsc_zero = tot[0] - asc_zero;
    dsc_ones = tot[1] - asc_ones;
    
    iv = calc_iv(asc_zero, asc_ones, dsc_zero, dsc_ones, grand_tot);
    
    woe_sign = (iv.asc_woe > iv.dsc_woe) ? 1 : -1;
       
    if ((asc_zero + asc_ones) < min_cnt) { // minsplit
      valid = -1;
    } else if ((dsc_zero + dsc_ones) < min_cnt) { // minsplit
      valid = -1;
    } else if (iv.iv < min_iv) { // min iv
      valid = -1;
    } else if (isinf(iv.iv)) { // infinite iv
      valid = -1;
    } else if (abs(mono) == 1) {
      if (woe_sign != mono_sign) {
        valid = -1;
      }
    } else if (abs(mono) == 2 & *pivot != INT_MIN) {
      pivot_iv =
        (i > *pivot) ?
        calc_iv(asc_zero, asc_ones, dsc_zero, dsc_ones, pivot_tot_dsc):
        calc_iv(asc_zero, asc_ones, dsc_zero, dsc_ones, pivot_tot_asc);
      
      woe_sign = (pivot_iv.asc_woe > pivot_iv.dsc_woe) ? 1 : -1;
      
      valid = (i > *pivot) ?
        ((woe_sign != -mono_sign) ? -1 : 0):
        ((woe_sign !=  mono_sign) ? -1 : 0);
      }
    
    if (valid != -1 & iv.iv > best_iv) {
      best_iv = iv.iv;
      best_split_idx = i;
      best_woe_sign = (iv.asc_woe > iv.dsc_woe) ? 1 : -1;
    }
  }
  
  // add two pieces of work to the queue
  if (best_iv > -1 & *num_bins < max_bin) {
    (*num_bins)++;
    breaks[best_split_idx] = 1; // update breaks array
    Rprintf("IV: %f\n", best_iv);
    
    if (best_woe_sign != mono_sign & *pivot == INT_MIN) {
      *pivot = best_split_idx;
      pivot_tot_asc = get_xtab_totals(xtab, 0, *pivot + 1);
      pivot_tot_dsc = get_xtab_totals(xtab, *pivot + 1, xtab->size);
      Rprintf("Asc: %f, %f\n", pivot_tot_asc[0], pivot_tot_asc[1]);
      Rprintf("Dsc: %f, %f\n", pivot_tot_dsc[0], pivot_tot_dsc[1]);
      Rprintf("Dsc: %f, %f\n", grand_tot[0], grand_tot[1]);
    }
    
    struct work w1 = {w.start, best_split_idx};
    struct work w2 = {best_split_idx + 1, w.stop};
    
    enqueue(q, w1); // add work to queue
    enqueue(q, w2);
  }
  
  free(tot);
}

struct iv calc_iv(double asc_zero, double asc_ones, double dsc_zero, double dsc_ones, double* tots) {
  struct iv iv = {0};
  double asc_woe = 0;
  double dsc_woe = 0;
  double asc_iv  = 0;
  double dsc_iv  = 0;
  
  asc_woe = log((asc_zero/tots[0])/(asc_ones/tots[1]));
  dsc_woe = log((dsc_zero/tots[0])/(dsc_ones/tots[1]));
  asc_iv  = asc_woe * (asc_zero/tots[0] - asc_ones/tots[1]);
  dsc_iv  = dsc_woe * (dsc_zero/tots[0] - dsc_ones/tots[1]);
  
  iv.asc_woe = asc_woe;
  iv.dsc_woe = dsc_woe;
  iv.iv = asc_iv + dsc_iv;
  
  return iv;
}