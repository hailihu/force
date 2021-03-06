/**+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This file is part of FORCE - Framework for Operational Radiometric
Correction for Environmental monitoring.

Copyright (C) 2013-2019 David Frantz

FORCE is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

FORCE is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with FORCE.  If not, see <http://www.gnu.org/licenses/>.

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++**/

/**+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
This file contains functions for computing landscape metrics
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Copyright (C) 2013-2019 Franz Schug, David Frantz
Contact: franz.schug@geo.hu-berlin.de
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++**/


#include "lsm-hl.h"


stack_t **compile_lsm(ard_t *features, lsm_t *lsm, par_hl_t *phl, cube_t *cube, int *nproduct);
stack_t *compile_lsm_stack(stack_t *ard, int nb, bool write, char *prodname, par_hl_t *phl);

int test_objects(small *cld_, int nx, int ny, int **OBJ, int **SIZE, int *nobj);


/** This function compiles the stacks, in which LSM results are stored.
+++ It also sets metadata and sets pointers to instantly useable image
+++ arrays.
--- features: input features
--- lsm:      pointer to instantly useable LSM image arrays
--- phl:      HL parameters
--- cube:     datacube definition
--- nproduct: number of output stacks (returned)
+++ Return:   stacks for LSM results
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++**/
stack_t **compile_lsm(ard_t *features, lsm_t *lsm, par_hl_t *phl, cube_t *cube, int *nproduct){
stack_t **LSM = NULL;
int o, nprod = 10;
int error = 0;
enum{ _mpa_, _uci_, _fdi_, _edd_, _nbr_, _ems_, _avg_, _std_, _geo_, _max_ };
int prodlen[10] ={ phl->ftr.nfeature, phl->ftr.nfeature, phl->ftr.nfeature, phl->ftr.nfeature, phl->ftr.nfeature, phl->ftr.nfeature, phl->ftr.nfeature, phl->ftr.nfeature, phl->ftr.nfeature, phl->ftr.nfeature };
char prodname[10][NPOW_02] ={ "MPA", "UCI", "FDI", "EDD", "NBR", "EMS", "AVG", "STD", "GEO", "MAX" };

int prodtype[10] ={ _mpa_, _uci_, _fdi_, _edd_, _nbr_, _ems_, _avg_, _std_, _geo_, _max_ };

bool enable[10] ={ phl->lsm.ompa, phl->lsm.ouci, phl->lsm.ofdi, phl->lsm.oedd, phl->lsm.onbr,
                    phl->lsm.oems, phl->lsm.oavg, phl->lsm.ostd, phl->lsm.ogeo, phl->lsm.omax };

bool write[10]  ={ phl->lsm.ompa, phl->lsm.ouci, phl->lsm.ofdi, phl->lsm.oedd, phl->lsm.onbr,
                    phl->lsm.oems, phl->lsm.oavg, phl->lsm.ostd, phl->lsm.ogeo, phl->lsm.omax };

short ***ptr[10] ={ &lsm->mpa_, &lsm->uci_, &lsm->fdi_, &lsm->edd_, &lsm->nbr_, &lsm->ems_, &lsm->avg_, &lsm->std_, &lsm->geo_, &lsm->max_};


  alloc((void**)&LSM, nprod, sizeof(stack_t*));

  for (o=0; o<nprod; o++){
    if (enable[o]){
      if ((LSM[o] = compile_lsm_stack(features[0].DAT, prodlen[prodtype[o]], write[o], prodname[o], phl)) == NULL || (*ptr[o] = get_bands_short(LSM[o])) == NULL){
        printf("Error compiling %s product. ", prodname[o]); error++;
      }
    } else{
      LSM[o]  = NULL;
      *ptr[o] = NULL;
    }
  }

  if (error > 0){
    printf("%d compiling LSM product errors.\n", error);
    for (o=0; o<nprod; o++) free_stack(LSM[o]);
    free((void*)LSM);
    return NULL;
  }

  *nproduct = nprod;
  return LSM;
}


/** This function compiles a LSM stack
--- from:      stack from which most attributes are copied
--- nb:        number of bands in stack
--- write:     should this stack be written, or only used internally?
--- prodname:  product name
--- phl:       HL parameters
+++ Return:    stack for LSM result
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++**/
stack_t *compile_lsm_stack(stack_t *from, int nb, bool write, char *prodname, par_hl_t *phl){
int b;
stack_t *stack = NULL;
char fname[NPOW_10];
char dname[NPOW_10];
int nchar;

  if ((stack = copy_stack(from, nb, _DT_SHORT_)) == NULL) return NULL;

  set_stack_name(stack, "FORCE Landscape Metrics");
  set_stack_product(stack, prodname);

  //printf("dirname should be assemlbed in write_stack, check with L2\n");
  nchar = snprintf(dname, NPOW_10, "%s/X%04d_Y%04d", phl->d_higher,
    get_stack_tilex(stack), get_stack_tiley(stack));
  if (nchar < 0 || nchar >= NPOW_10){
    printf("Buffer Overflow in assembling dirname\n"); return NULL;}
  set_stack_dirname(stack, dname);

  nchar = snprintf(fname, NPOW_10, "%s_HL_LSM_%s", phl->lsm.base, prodname);
  if (nchar < 0 || nchar >= NPOW_10){
    printf("Buffer Overflow in assembling filename\n"); return NULL;}
  set_stack_filename(stack, fname);


  if (write){
    set_stack_open(stack, OPEN_BLOCK);
  } else{
    set_stack_open(stack, OPEN_FALSE);
  }
  set_stack_format(stack, phl->format);
  set_stack_par(stack, phl->params->log);

  for (b=0; b<nb; b++){
    set_stack_save(stack, b, true);
  }

  return stack;
}


/** public functions
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++**/


/** This function is the entry point to the landscape metrics module
--- features:  input features
--- mask:      mask image
--- nf:        number of features
--- phl:       HL parameters
--- cube:      datacube definition
--- nproduct:  number of output stacks (returned)
+++ Return:    stacks with TXT results
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++**/
stack_t **landscape_metrics(ard_t *features, stack_t *mask, int nf, par_hl_t *phl, cube_t *cube, int *nproduct){
lsm_t lsm;
stack_t **LSM;
small *mask_ = NULL;
int nprod = 0;
int f, i, j, p, t, nx, ny, nc, ni, nj, np, u;
int ki, kj; // iterator for KDIST
int ii, jj; // iterator for kernel relative to center
int nobj = 0;
short nodata;
float share = 0;
int ccl = 0;
int *CCL = NULL;
int *SIZE = NULL;
int *exists = NULL;
float **KDIST = NULL;
int numberOfuniquePatches = 0;
int *uniquePatches;
float *patchperimeters;
float *patchAreas;
float totalClassArea = 0;
float totaledgelength = 0;
double sumFractalDims = 0;
float logSum = 0;
float sumarea = 0;
float sumshare = 0;
int maxVal = 0;
int logCounter = 0;
int validDataPixels = 0;
int kernelSize = (phl->lsm.radius * 2 + 1) * (phl->lsm.radius * 2 + 1);
int width;
small *newFeatures = NULL;
double mx, vx;
float kfraction, sqkfraction;
float unit_area = 0;
float unit_perim = 0;

  cite_me(_CITE_LSM_);

  // import stacks
  nx = get_stack_chunkncols(features[0].DAT);
  ny = get_stack_chunknrows(features[0].DAT);
  nc = get_stack_chunkncells(features[0].DAT);
  nodata = get_stack_nodata(features[0].DAT, 0);

  // import mask (if available)
  if (mask != NULL){
    if ((mask_ = get_band_small(mask, 0)) == NULL){
      printf("Error getting processing mask.");
      *nproduct = 0;
      return NULL;}
  }

  // compile products + stacks
  if ((LSM = compile_lsm(features, &lsm, phl, cube, &nprod)) == NULL || nprod == 0){
    printf("Unable to compile LSM products!\n");
    *nproduct = 0;
    return NULL;
  }


  // pre-compute kernel distances
  width = phl->lsm.radius*2+1;
  distance_kernel(width, &KDIST);

  kfraction = 1.0/(float)kernelSize;
  sqkfraction = sqrt(kfraction);


  // based on parameter input: range of numbers that are considered one class
  // --> derive new array with binaries from input features
  // newFeatures is one-layered
  alloc((void**)&newFeatures, nc, sizeof(small));

  for (f=0; f<nf; f++){

    memset(newFeatures, 0, sizeof(small)*nc);

    #pragma omp parallel shared(newFeatures,lsm,phl,features,mask_,nc,f,nodata) default(none)
   {

      #pragma omp for
      for (p=0; p<nc; p++){
        
        // if the metric is selected, initiate it with nodata
        if (phl->lsm.ompa) lsm.mpa_[f][p] = nodata;
        if (phl->lsm.ouci) lsm.uci_[f][p] = nodata;
        if (phl->lsm.ofdi) lsm.fdi_[f][p] = nodata;
        if (phl->lsm.oedd) lsm.edd_[f][p] = nodata;
        if (phl->lsm.onbr) lsm.nbr_[f][p] = nodata;
        if (phl->lsm.oems) lsm.ems_[f][p] = nodata;
        if (phl->lsm.oavg) lsm.avg_[f][p] = nodata;
        if (phl->lsm.ostd) lsm.std_[f][p] = nodata;
        if (phl->lsm.ogeo) lsm.geo_[f][p] = nodata;
        if (phl->lsm.omax) lsm.max_[f][p] = nodata;

        if (mask_ != NULL && !mask_[p]) continue;
        if (!features[f].msk[p] && phl->ftr.exclude) continue;

        switch (phl->lsm.query[f]){
          case _QUERY_EQ_:
            if (features[f].dat[0][p] == phl->lsm.threshold[f]) newFeatures[p] = true;
            break;
          case _QUERY_GT_:
            if (features[f].dat[0][p] >= phl->lsm.threshold[f]) newFeatures[p] = true;
            break;
          case _QUERY_LT_:
            if (features[f].dat[0][p] <= phl->lsm.threshold[f]) newFeatures[p] = true;
            break;
          default:
            newFeatures[p] = false;
        }
      }

    }

    // make objects and delete small ones
    binary_to_objects(newFeatures, nx, ny, 3, &CCL, &SIZE, &nobj);
    if (nobj < 1) continue;


    #pragma omp parallel firstprivate(kernelSize,numberOfuniquePatches) private(p,u,uniquePatches,ccl,patchperimeters,patchAreas,totalClassArea,totaledgelength,sumFractalDims,mx,vx,validDataPixels,maxVal,logSum,logCounter,ii,jj,ni,nj,np,exists,t,share,kj,ki,sumarea,sumshare,unit_area,unit_perim) shared(ny,nx,nobj,f,nodata,lsm,mask_,CCL,features,phl,newFeatures,KDIST,kfraction,sqkfraction) default(none)
    {

      alloc((void**)&uniquePatches, kernelSize, sizeof(int));
      alloc((void**)&patchperimeters, nobj+1, sizeof(float));
      alloc((void**)&patchAreas,      nobj+1, sizeof(float));
      alloc((void**)&exists,          nobj+1, sizeof(int));

      #pragma omp for collapse(2) schedule(guided)
      for (i=0; i<ny; i++){
      for (j=0; j<nx; j++){

        p = i*nx+j;

        if (mask_ != NULL && !mask_[p]) continue;
        if (!features[f].msk[p] && phl->ftr.exclude) continue;
        if (!newFeatures[p] && !phl->lsm.allpx) continue;

        // loop through a kernel of given size
        for (u=0; u<numberOfuniquePatches; u++){
          patchperimeters[uniquePatches[u]] = 0;
          patchAreas[uniquePatches[u]] = 0;
          exists[uniquePatches[u]]     = 0;
          uniquePatches[u] = 0;
        }
        numberOfuniquePatches = 0;
        totalClassArea = 0;
        totaledgelength = 0;
        sumFractalDims = 0;
        logSum = 0;
        logCounter = 0;
        validDataPixels = 0;
        maxVal = 0;
        mx = vx = 0;


        for (ii=-phl->lsm.radius, ki=0; ii<=phl->lsm.radius; ii++, ki++){
        for (jj=-phl->lsm.radius, kj=0; jj<=phl->lsm.radius; jj++, kj++){

          ni = i+ii; nj = j+jj;
          np = ni*nx+nj;
          
          // if not in circular kernel, skip
          if (phl->lsm.kernel == _KERNEL_CIRCLE_ && 
              KDIST[ki][kj] > phl->lsm.radius) continue;


          // if invalid pixel
          if (ni < 0 || nj < 0 || ni >= ny || nj >= nx) continue;

          // if not in processing mask
          if (mask_ != NULL && !mask_[np]) continue;

          // if nodata
          if (!features[f].msk[np] && phl->ftr.exclude) continue;

          // if inactive pixel
          if (!newFeatures[np] && !phl->lsm.allpx) continue;

          //do not incorporate nodata values in sum
          if (features[f].dat[0][np] != nodata){

            validDataPixels++;
            if (validDataPixels == 1){
              mx = features[f].dat[0][np];
            } else {
              var_recurrence(features[f].dat[0][np], &mx, &vx, validDataPixels);
            }

            if (maxVal < features[f].dat[0][np]) maxVal = features[f].dat[0][np];

          }

          // natural log is undefined for 0 or negative values
          if (features[f].dat[0][np] > 0){
            logSum += (float)log(features[f].dat[0][np]);
            logCounter++;
          }


          ccl = CCL[np];

          if (ccl > 0 && !exists[ccl]){
            uniquePatches[numberOfuniquePatches] = ccl;
            numberOfuniquePatches++;
            exists[ccl] = true;
          }
          if (numberOfuniquePatches == 0) continue;
      
          // Increase patch area for that class
          patchAreas[ccl] += kfraction;
      
          // add total class area
          if (newFeatures[np]) totalClassArea += kfraction;


          // At that same position in the patchperimeters array, add edge length
          if (ii != phl->lsm.radius && ni+1 < ny &&
              newFeatures[np] != newFeatures[(ni+1)*nx+nj]){
              patchperimeters[ccl] += sqkfraction;
              totaledgelength += sqkfraction;
          }

          if (ii != -phl->lsm.radius && ni-1 >= 0 &&
              newFeatures[np] != newFeatures[(ni-1)*nx+nj]){
              patchperimeters[ccl] += sqkfraction;
              totaledgelength += sqkfraction;
          }

          if (jj != phl->lsm.radius && nj+1 < nx &&
              newFeatures[np] != newFeatures[ni*nx+(nj+1)]){
              patchperimeters[ccl] += sqkfraction;
              totaledgelength += sqkfraction;
        }
  
          if (jj != -phl->lsm.radius && nj-1 >= 0 &&
              newFeatures[np] != newFeatures[ni*nx+(nj-1)]){
              patchperimeters[ccl] += sqkfraction;
              totaledgelength += sqkfraction;
          }
        
        }
        }


        if (phl->lsm.ouci) lsm.uci_[f][p] = CCL[p];
        if (phl->lsm.oavg) lsm.avg_[f][p] = mx;
        if (phl->lsm.ogeo) lsm.geo_[f][p] = exp(logSum / (float)logCounter);
        if (phl->lsm.omax) lsm.max_[f][p] = maxVal;
        if (phl->lsm.ostd) lsm.std_[f][p] = standdev(vx, validDataPixels);


        // area weight. share of patch area in total class area
        sumarea = sumshare = 0;
      
        for (t=0; t<numberOfuniquePatches; t++){
          share = patchAreas[uniquePatches[t]] / totalClassArea;
      
          sumarea  += share * patchAreas[uniquePatches[t]];
          sumshare += share;
      
        }
        
        if (phl->lsm.ompa){
          if (sumshare > 0){
                lsm.mpa_[f][p] = (short)(sumarea/sumshare*10000);
          } else lsm.mpa_[f][p] = 0;
        }

  
        if (phl->lsm.oedd) lsm.edd_[f][p] = (short)(totaledgelength / sqrt(kernelSize) * 10000);
        if (phl->lsm.onbr) lsm.nbr_[f][p] = numberOfuniquePatches;
        if (phl->lsm.oems) lsm.ems_[f][p] = (short)(totalClassArea * totalClassArea * 10000);

        sumshare = 0;
        // weighted sum of fractal dimensions
        for (t=0; t<numberOfuniquePatches; t++){

          if (patchAreas[uniquePatches[t]]*kernelSize == 1) continue;

            unit_area  = patchAreas[uniquePatches[t]]*kernelSize;
            unit_perim = patchperimeters[uniquePatches[t]]*sqrt(kernelSize);

            sumshare += patchAreas[uniquePatches[t]];
            sumFractalDims += patchAreas[uniquePatches[t]] * 
              (2.0 * log(0.25 * unit_perim)) / log(unit_area);

        }

        //  weighted mean fractal index
        if (numberOfuniquePatches != 0){
          if (phl->lsm.ofdi) lsm.fdi_[f][p] = (short) (sumFractalDims / sumshare * 10000);
        } else{
          if (phl->lsm.ofdi) lsm.fdi_[f][p] = 0;
        }


      }
      }

      free((void*)uniquePatches);
      free((void*)patchperimeters);
      free((void*)patchAreas);
      free((void*)exists);

    }

    free((void*)CCL);
    free((void*)SIZE);

  }

  free((void*)newFeatures);
  free_2D((void**)KDIST, width);

  *nproduct = nprod;
  return LSM;
}

