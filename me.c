/*************************************************************
Copyright (C) 1990, 1991, 1993 Andy C. Hung, all rights reserved.
PUBLIC DOMAIN LICENSE: Stanford University Portable Video Research
Group. If you use this software, you agree to the following: This
program package is purely experimental, and is licensed "as is".
Permission is granted to use, modify, and distribute this program
without charge for any purpose, provided this license/ disclaimer
notice appears in the copies.  No warranty or maintenance is given,
either expressed or implied.  In no event shall the author(s) be
liable to you or a third party for any special, incidental,
consequential, or other damages, arising out of the use or inability
to use the program for any purpose (or the loss of data), even if we
have been advised of such possibilities.  Any public reference or
advertisement of this source code should refer to it as the Portable
Video Research Group (PVRG) code, and not by any author(s) (or
Stanford University) name.
*************************************************************/
/*
************************************************************
me.c

This file does much of the motion estimation and compensation.

************************************************************
*/

/*LABEL me.c */

#include "globals.h"

/*PUBLIC*/

extern void initmc();
extern void FastBME();
extern void BruteMotionEstimation();
extern void BMC();
extern MEM *MotionCompensation();

/*PRIVATE*/

static int VAR;
static int VAROR;
static int MWOR;

int MeVAR[1024];
int MeVAROR[1024];
int MeMWOR[1024];
int MX;
int MY;
int MV;
int OMV;
int MeX[1024];
int MeY[1024];
int MeVal[1024];
int MeOVal[1024];
int MeN=0;

int SearchLimit = 15;
BLOCK nb,rb;

#define COMPARISON >=  /* This is to compare for short-circuit exit */

/*START*/

/*BFUNC

initmc() initializes the block structures used for copying areas
of memory.

EFUNC*/

void initmc()
{
  BEGIN("initmc");

  nb = MakeBlock();
  rb = MakeBlock();
}

/*BFUNC
 
 ComputeError() gives an error score regarding how well a 16x16 block matches
 a target. Computation stops once the error reaches or surpasses the
 best known solution.
 
 EFUNC*/

__inline int ComputeError(unsigned char *bptr, unsigned char *cptr, MEM *rm, MEM *cm) {
    int i,residue,error;
    error = 0;
    for(i=0;i<16;i++) {
        residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
	residue=(*(bptr++)-*(cptr++));
	if (residue<0) {error-=residue;} else {error+=residue;}
        // break if the error exceeds the best known solution
	if (error COMPARISON MV) break;
	bptr += (rm->width - 16);
	cptr += (cm->width - 16);
    }
    return error;
}


/*BFUNC

FastBME() does a fast brute-force motion estimation with two indexes
into two memory structures. The motion estimation has a short-circuit
abort to speed up calculation.

EFUNC*/

void FastBME(int rx, int ry, MEM *rm, int cx, int cy, MEM *cm) {

    BEGIN("FastBME");
    int px, py, dx, dy, i, j, data, error;
    unsigned char *baseptr, *bptr, *cptr;

    // set up to check (0,0) vector
    MX = MY = MV = 0;
    bptr = rm->data + rx + (ry * rm->width);
    baseptr = cm->data + cx + (cy * cm->width);
    cptr = baseptr;
    
    // make sure we don't short-circuit when computing the error score
    MV = 65536;
    
    // determine error for (0,0)
    OMV = MV = ComputeError(bptr, cptr, rm, cm);

    // do an exhaustive search
    for (dx = -SearchLimit / 2; dx < SearchLimit / 2; ++dx) {
        px = rx + dx;
        for (dy = -SearchLimit / 2; dy < SearchLimit / 2; ++dy) {
            py = ry + dy;

            // only test vector if we're within frame boundaries
            if ((px >= 0) && (px < rm->width - 16) &&
                    (py >= 0) && (py < rm->height - 16)) {

                error = 0;
                bptr = rm->data + px + (py * rm->width);
                cptr = baseptr;
                error = ComputeError(bptr, cptr, rm, cm);

                if (error < MV) {
                    MV = error;
                    MX = dx;
                    MY = dy;
                }
            }
        }
    }

    // gather statistics for search result, used in decision-making
    bptr = rm->data + (MX + rx) + ((MY + ry) * rm->width);
    cptr = baseptr;
    for (VAR = 0, VAROR = 0, MWOR = 0, i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            data = *(bptr) - *(cptr);
            VAR += data*data;
            VAROR += *(bptr)*(*(bptr));
            MWOR += *(bptr);
            bptr++;
            cptr++;
        }
        bptr += (rm->width - 16);
        cptr += (cm->width - 16);
    }
    VAR = VAR / 256;
    VAROR = (VAROR / 256)-(MWOR / 256)*(MWOR / 256);
}

/*BFUNC

StepBME() implements the Three Step Search (TSS) motion search algorithm
proposed by Koga et al. 1981

EFUNC*/

void StepBME(int rx, int ry, MEM *rm, int cx, int cy, MEM *cm) {

    BEGIN("StepBME");
    int px, py, dx, dy, i, j, data, error, step, dirx, diry, bestx, besty;
    unsigned char *baseptr, *bptr, *cptr;

    // set up to check (0,0) vector
    MX = MY = MV = 0;
    bptr = rm->data + rx + (ry * rm->width);
    baseptr = cm->data + cx + (cy * cm->width);
    cptr = baseptr;

    // make sure we don't short-circuit when computing the error score
    MV = 65536;

    // determine error for (0,0)
    OMV = MV = ComputeError(bptr, cptr, rm, cm);

    // initialize step search at (0,0)
    bestx = rx;
    besty = ry;

    for (step = 16; step >= 1; step /= 2) {
        for (diry = -1; diry <= 1; ++diry) {
            py = besty + (diry * step);
            dy = py - ry;
            for (dirx = -1; dirx <= 1; ++dirx) {
                if(dirx == 0 && diry == 0) {
                    // dont' test center again, we already did that
                    continue;
                }
                
                px = bestx + (dirx * step);
                dx = px - rx;

                // only test vector if we're within frame boundaries
                // and the vector components are within -15..15
                if ((px >= 0) && (px < rm->width - 16) &&
                        (py >= 0) && (py < rm->height - 16) &&
                         dx >= -15 && dx <= 15 && dy >= -15 && dy <= 15) {

                    bptr = rm->data + px + (py * rm->width);
                    cptr = baseptr;
                    error = ComputeError(bptr, cptr, rm, cm);

                    if (error < MV) {
                        MV = error;
                        MX = dx;
                        MY = dy;
                    }
                }
            }
        }
        // center search for next iteration on best candidate so far
        bestx = rx + MX;
        besty = ry + MY;
    }

    // gather statistics for search result, used in decision-making
    bptr = rm->data + (MX + rx) + ((MY + ry) * rm->width);
    cptr = baseptr;
    for (VAR = 0, VAROR = 0, MWOR = 0, i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            data = *(bptr) - *(cptr);
            VAR += data*data;
            VAROR += *(bptr)*(*(bptr));
            MWOR += *(bptr);
            bptr++;
            cptr++;
        }
        bptr += (rm->width - 16);
        cptr += (cm->width - 16);
    }
    VAR = VAR / 256;
    VAROR = (VAROR / 256)-(MWOR / 256)*(MWOR / 256);
}
     
     
/*BFUNC

MotionEstimation() does a motion estimation on all
aligned 16x16 blocks in two memory structures.

EFUNC*/

void MotionEstimation(pmem,fmem)
     MEM *pmem;
     MEM *fmem;
{
  BEGIN("MotionEstimation");
  int x,y;

  for(MeN=0,y=0;y<fmem->height;y+=16)
    {
      for(x=0;x<fmem->width;x+=16)
	{
	  //FastBME(x,y,pmem,x,y,fmem);
          StepBME(x,y,pmem,x,y,fmem);
	  MeVAR[MeN] = VAR;
	  MeVAROR[MeN] = VAROR;
	  MeMWOR[MeN] = MWOR;
	  MeX[MeN] = MX;
	  MeY[MeN] = MY;
	  MeVal[MeN] = MV;
	  MeOVal[MeN] = OMV;
	  MeN++;
	}
    }
}

/*BFUNC

BMC() does a motion compensated copy from one memory structure to
another memory structure with appropriate indexes.

EFUNC*/

void BMC(rx,ry,rm,cx,cy,cm)
     int rx;
     int ry;
     MEM *rm;
     int cx;
     int cy;
     MEM *cm;
{
  BEGIN("BMC");

  SetPointerBlock(rx,ry,cm,nb);
  SetPointerBlock(cx+MX,cy+MY,rm,rb);
  CopyBlock(rb,nb);
}

/*BFUNC

MotionCompensation() does a full motion compensation of all the blocks
based on the motion vectors created by BruteMotionEstimation. Not
actually used in the program. If (omem) is null, it creates a new
memory structure.

EFUNC*/

MEM *MotionCompensation(mem,omem)
     MEM *mem;
     MEM *omem;
{
  BEGIN("MotionCompensation");
  int x,y;
  int count;
  MEM *temp;

  if (omem) {temp=omem;}
  else {temp = MakeMem(mem->width,mem->height);}
  for(count=0,y=0;y<mem->height;y+=16)
    {
      for(x=0;x<mem->width;x+=16)
	{
	  SetPointerBlock(x,y,temp,nb);
	  SetPointerBlock(x+MeX[count],y+MeY[count],mem,rb);
	  count++;
	  CopyBlock(rb,nb);
	}
    }
  return(temp);
}

/*END*/
