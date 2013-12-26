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
transform.c

This file contains the reference DCT, the zig-zag and quantization
algorithms.

************************************************************
*/

/*LABEL transform.c */

#include "globals.h"
#include "dct.h"
#include <math.h>

/*PUBLIC*/

extern void ReferenceDct();
extern void ReferenceIDct();
extern void TransposeMatrix();
extern void CCITTQuantize();
extern void CCITTFlatQuantize();
extern void ICCITTFlatQuantize();
extern void ICCITTQuantize();
extern void BoundDctMatrix();
extern void BoundIDctMatrix();
extern void FlatBoundQuantizeMatrix();
extern void ZigzagMatrix();
extern void IZigzagMatrix();
extern void PrintMatrix();
extern void ClearMatrix();

static void DoubleReferenceDct1D();
static void DoubleReferenceIDct1D();
static void DoubleTransposeMatrix();

/*PRIVATE*/

static int transpose_index[] =
{0,  8, 16, 24, 32, 40, 48, 56,
 1,  9, 17, 25, 33, 41, 49, 57,
 2, 10, 18, 26, 34, 42, 50, 58,
 3, 11, 19, 27, 35, 43, 51, 59,
 4, 12, 20, 28, 36, 44, 52, 60, 
 5, 13, 21, 29, 37, 45, 53, 61,
 6, 14, 22, 30, 38, 46, 54, 62,
 7, 15, 23, 31, 39, 47, 55, 63};

static int zigzag_index[] =
{0,  1,  5,  6, 14, 15, 27, 28,
 2,  4,  7, 13, 16, 26, 29, 42,
 3,  8, 12, 17, 25, 30, 41, 43,
 9, 11, 18, 24, 31, 40, 44, 53,
10, 19, 23, 32, 39, 45, 52, 54,
20, 22, 33, 38, 46, 51, 55, 60,
21, 34, 37, 47, 50, 56, 59, 61,
35, 36, 48, 49, 57, 58, 62, 63};


#define MakeMatrix() (int *) calloc(BLOCKSIZE,sizeof(int))
#define FixedMultiply(s,x,y)  x = ((x * y) >> s);
#define DCT_OFFSET 128

/*START*/

/*BFUNC

FastDivide() is a very bizarre little helper to let the compiler construct
fast integer divides for the most common divisors.

EFUNC*/


__inline int FastDivide(int divident, int divisor) {
    switch(divisor) {
        case 1: return divident;
        case 2: return divident / 2;
        case 3: return divident / 3;
        case 4: return divident / 4;
        case 5: return divident / 5;
        case 6: return divident / 6;
        case 7: return divident / 7;
        case 8: return divident / 8;
        case 9: return divident / 9;
        case 10: return divident / 10;
        case 11: return divident / 11;
        case 12: return divident / 12;
        case 13: return divident / 13;
        case 14: return divident / 14;
        case 15: return divident / 15;
        case 16: return divident / 16;
        case 17: return divident / 17;
        case 18: return divident / 18;
        case 19: return divident / 19;
        case 20: return divident / 20;
        case 21: return divident / 21;
        case 22: return divident / 22;
        case 23: return divident / 23;
        case 24: return divident / 24;
        case 25: return divident / 25;
        case 26: return divident / 26;
        case 27: return divident / 27;
        case 28: return divident / 28;
        case 29: return divident / 29;
        case 30: return divident / 30;
        case 31: return divident / 31;
    }
    return divident / divisor;     
}
/*BFUNC

ReferenceDct() does a reference DCT on the input (matrix) and output
(new matrix).

EFUNC*/

void ReferenceDct(int *matrix, int *newmatrix) {
    BEGIN("ReferenceDct");
    int *mptr;
    double *sptr, *dptr;
    double sourcematrix[BLOCKSIZE], destmatrix[BLOCKSIZE];

    for (sptr = sourcematrix, mptr = matrix; mptr < matrix + BLOCKSIZE; mptr++) {
        *(sptr++) = (double) *mptr;
    }
    for (dptr = destmatrix, sptr = sourcematrix;
            sptr < sourcematrix + BLOCKSIZE; sptr += BLOCKWIDTH) {
        DoubleReferenceDct1D(sptr, dptr);
        dptr += BLOCKWIDTH;
    }
    DoubleTransposeMatrix(destmatrix, sourcematrix);
    for (dptr = destmatrix, sptr = sourcematrix;
            sptr < sourcematrix + BLOCKSIZE; sptr += BLOCKWIDTH) {
        DoubleReferenceDct1D(sptr, dptr);
        dptr += BLOCKWIDTH;
    }
    DoubleTransposeMatrix(destmatrix, sourcematrix);
    for (sptr = sourcematrix, mptr = newmatrix;
            mptr < newmatrix + BLOCKSIZE; sptr++) { /* NB: Inversion on counter */
        *(mptr++) = (int) (*sptr > 0 ? (*(sptr) + 0.5) : (*(sptr) - 0.5));
    }
}
     
/*BFUNC

DoubleReferenceDCT1D() does a 8 point dct on an array of double
input and places the result in a double output.

EFUNC*/

static void DoubleReferenceDct1D(double *ivect, double *ovect) {
    BEGIN("DoubleReferenceDct1D");
    double *mptr, *iptr, *optr;

    for (mptr = DctMatrix, optr = ovect; optr < ovect + BLOCKWIDTH; optr++) {
        for (*optr = 0, iptr = ivect; iptr < ivect + BLOCKWIDTH; iptr++) {
            *optr += *iptr * (*(mptr++));
        }
    }
}

/*BFUNC

ReferenceIDct() is used to perform a reference 8x8 inverse dct.  It is
a balanced IDCT. It takes the input (matrix) and puts it into the
output (newmatrix).

EFUNC*/

void ReferenceIDct(int *matrix, int *newmatrix) {
    BEGIN("ReferenceIDct");
    int *mptr;
    double *sptr, *dptr;
    double sourcematrix[BLOCKSIZE], destmatrix[BLOCKSIZE];

    for (sptr = sourcematrix, mptr = matrix; mptr < matrix + BLOCKSIZE; mptr++) {
        *(sptr++) = (double) *mptr;
    }
    for (dptr = destmatrix, sptr = sourcematrix;
            sptr < sourcematrix + BLOCKSIZE; sptr += BLOCKWIDTH) {
        DoubleReferenceIDct1D(sptr, dptr);
        dptr += BLOCKWIDTH;
    }
    DoubleTransposeMatrix(destmatrix, sourcematrix);
    for (dptr = destmatrix, sptr = sourcematrix;
            sptr < sourcematrix + BLOCKSIZE; sptr += BLOCKWIDTH) {
        DoubleReferenceIDct1D(sptr, dptr);
        dptr += BLOCKWIDTH;
    }
    DoubleTransposeMatrix(destmatrix, sourcematrix);
    for (sptr = sourcematrix, mptr = newmatrix; mptr < newmatrix + BLOCKSIZE; sptr++) { /* NB: Inversion on counter */
        *(mptr++) = (int) (*sptr > 0 ? (*(sptr) + 0.5) : (*(sptr) - 0.5));
    }
}

/*BFUNC

DoubleReferenceIDct1D() does an 8 point inverse dct on ivect and
puts the output in ovect.

EFUNC*/

static void DoubleReferenceIDct1D(double *ivect, double *ovect) {
    BEGIN("DoubleReferenceIDct1D");
    double *mptr, *iptr, *optr;

    for (mptr = IDctMatrix, optr = ovect; optr < ovect + BLOCKWIDTH; optr++) {
        for (*optr = 0, iptr = ivect; iptr < ivect + BLOCKWIDTH; iptr++) {
            *optr += *iptr * (*(mptr++));
        }
    }
}

/*BFUNC

TransposeMatrix transposes an input matrix and puts the output in
newmatrix.

EFUNC*/

void TransposeMatrix(int *matrix, int *newmatrix) {
    BEGIN("TransposeMatrix");
    int *tptr;

    for (tptr = transpose_index; tptr < transpose_index + BLOCKSIZE; tptr++) {
        *(newmatrix++) = matrix[*tptr];
    }
}

/*BFUNC

DoubleTransposeMatrix transposes a double input matrix and puts the
double output in newmatrix.

EFUNC*/

static void DoubleTransposeMatrix(double *matrix, double *newmatrix) {
    BEGIN("DoubleTransposeMatrix");
    int *tptr;

    for (tptr = transpose_index; tptr < transpose_index + BLOCKSIZE; tptr++) {
        *(newmatrix++) = matrix[*tptr];
    }
}
     
/*BFUNC

CCITTQuantize() quantizes the input matrix with a DC quantize step
and an AC quantize step.

EFUNC*/

__inline void CCITTQuantize(int *matrix, int dcqfact, int acqfact) {

    BEGIN("CCITTQuantize");
    int *mptr, coeff;
#ifdef VERSION_1_0
    dcqfact++;
    acqfact++;
    dcqfact <<= 1;
    acqfact <<= 1;
    *matrix = *matrix / dcqfact;
    for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
        *mptr = *mptr / acqfact;
    }
#else
    if (dcqfact & 1) { /* Odd */
        *matrix = FastDivide(*matrix, (dcqfact << 1));
    } else { /* Even */
        coeff = *matrix;
        coeff += (coeff > 0) ? 1 : -1;
        coeff = FastDivide(coeff, (dcqfact << 1));
        *matrix = coeff;
    }

    if (acqfact & 1) /* Odd */ {
        acqfact <<= 1;
        for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; ++mptr) {
            *mptr = FastDivide(*mptr, acqfact);
        }
    } else {
        acqfact <<= 1;
        for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; ++mptr) {
            coeff = *mptr;
            coeff += (coeff > 0) ? 1 : -1;
            coeff = FastDivide(coeff, acqfact);
            *mptr = coeff;
        }
    }
#endif
}
/*BFUNC

CCITTFlatQuantize() quantizes the input matrix by a dc factor (flat)
and an acqfactor (thresholded).

EFUNC*/

void CCITTFlatQuantize(int *matrix, int dcqfact, int acqfact) {
    BEGIN("CCITTFlatQuantize");
    int *mptr;

    if (*matrix > 0) {
        *matrix = FastDivide((*matrix + dcqfact / 2), dcqfact);
    } else {
        *matrix = FastDivide((*matrix - dcqfact / 2), dcqfact);
    }

#ifdef VERSION_1_0
    acqfact++;
    acqfact <<= 1;
    for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
        *mptr = *mptr / acqfact;
    }
#else
    if (acqfact & 1) /* Odd */ {
        acqfact <<= 1;
        for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
            *mptr = FastDivide(*mptr, acqfact);
        }
    } else {
        acqfact <<= 1;
        for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
            if (*mptr > 0) {
                *mptr = FastDivide((*mptr + 1), acqfact);
            } else {
                *mptr = FastDivide((*mptr - 1), acqfact);
            }
        }
    }
#endif
}
/*BFUNC

ICCITTFlatQuantize() does an inverse flat quantize on the dc element
and an inverse thresholded quantize on the ac elements of the input
matrix.

EFUNC*/

void ICCITTFlatQuantize(int *matrix, int dcqfact, int acqfact) {
    BEGIN("ICCITTFlatQuantize");
    int *mptr;

    *matrix = *matrix*dcqfact;
#ifdef VERSION_1_0
    acqfact++;
    for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
        if (*mptr > 0) {
            *mptr = ((*mptr << 1) + 1) * acqfact;
        } else if (*mptr < 0) {
            *mptr = ((*mptr << 1) - 1) * acqfact;
        }
    }
#else
    if (acqfact & 1) /* Odd */ {
        for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
            if (*mptr > 0) {
                *mptr = ((*mptr << 1) + 1) * acqfact;
            } else if (*mptr < 0) {
                *mptr = ((*mptr << 1) - 1) * acqfact;
            }
        }
    } else {
        for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
            if (*mptr > 0) {
                *mptr = (((*mptr << 1) + 1) * acqfact) - 1;
            } else if (*mptr < 0) {
                *mptr = (((*mptr << 1) - 1) * acqfact) + 1;
            }
        }
    }
#endif
}
/*BFUNC

ICCITTQuantize() does an inverse quantize on the dc element and the ac
element with their selective q values, respectively.

EFUNC*/

void ICCITTQuantize(int *matrix, int dcqfact, int acqfact) {
    BEGIN("ICCITTQuantize");
    int *mptr;

#ifdef VERSION_1_0
    dcqfact++;
    acqfact++;
    if (*matrix > 0) {
        *matrix = (2 * *matrix + 1) * dcqfact;
    } else if (*matrix < 0) {
        *matrix = (2 * *matrix - 1) * dcqfact;
    }
    for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
        if (*mptr > 0) {
            *mptr = (2 * *mptr + 1) * acqfact;
        } else if (*mptr < 0) {
            *mptr = (2 * *mptr - 1) * acqfact;
        }
    }
#else
    if (dcqfact & 1) {
        if (*matrix > 0) {
            *matrix = ((*matrix << 1) + 1) * dcqfact;
        } else if (*matrix < 0) {
            *matrix = ((*matrix << 1) - 1) * dcqfact;
        }
    } else {
        if (*matrix > 0) {
            *matrix = (((*matrix << 1) + 1) * dcqfact) - 1;
        } else if (*matrix < 0) {
            *matrix = (((*matrix << 1) - 1) * dcqfact) + 1;
        }
    }
    if (acqfact & 1) /* Odd */ {
        for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
            if (*mptr > 0) {
                *mptr = ((*mptr << 1) + 1) * acqfact;
            } else if (*mptr < 0) {
                *mptr = ((*mptr << 1) - 1) * acqfact;
            }
        }
    } else {
        for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
            if (*mptr > 0) {
                *mptr = (((*mptr << 1) + 1) * acqfact) - 1;
            } else if (*mptr < 0) {
                *mptr = (((*mptr << 1) - 1) * acqfact) + 1;
            }
        }
    }
#endif
}

/*BFUNC

BoundDctMatrix() clips the Dct matrix such that it is no larger than
a 10 bit word.

EFUNC*/

void BoundDctMatrix(int *matrix) {
    BEGIN("BoundDctMatrix");
    int *mptr;

    if (*matrix > 2047) {
        *matrix = 2047;
    }
    for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
        if (*mptr < -1023) {
            *mptr = -1023;
        } else if (*mptr > 1023) {
            *mptr = 1023;
        }
    }
}

/*BFUNC

BoundIDctMatrix bounds the inverse dct matrix so that no pixel has a
value greater than 255 or less than 0.

EFUNC*/

void BoundIDctMatrix(int *matrix) {
    BEGIN("BoundIDctMatrix");
    int *mptr;

    for (mptr = matrix; mptr < matrix + BLOCKSIZE; mptr++) {
        if (*mptr < 0) {
            *mptr = 0;
        } else if (*mptr > 255) {
            *mptr = 255;
        }
    }
}
/*BFUNC

FlatBoundQuantizeMatrix() bounds a quantized matrix generated by
CCITTFlatQuantize().

EFUNC*/

void FlatBoundQuantizeMatrix(int *matrix) {
    BEGIN("FlatBoundQuantizeMatrix");
    int *mptr;

    if (*matrix > 254) {
        *matrix = 254;
    } else if (*matrix < 1) {
        *matrix = 1;
    }
    for (mptr = matrix + 1; mptr < matrix + BLOCKSIZE; mptr++) {
        if (*mptr < -127) {
            *mptr = -127;
        } else if (*mptr > 127) { /* Should this be a -127? */
            *mptr = 127;
        }
    }
}
/*BFUNC

BoundQuantizeMatrix() bounds a quantized matrix generated by
CCITTQuantize().

EFUNC*/

void BoundQuantizeMatrix(int *matrix) {
    BEGIN("BoundQuantizeMatrix");
    int *mptr;

    for (mptr = matrix; mptr < matrix + BLOCKSIZE; mptr++) {
        if (*mptr < -127) {
            *mptr = -127;
        } else if (*mptr > 127) { /* Should this be a -127? */
            *mptr = 127;
        }
    }
}

/*BFUNC

IZigzagMatrix() performs an inverse zig-zag translation on the
input imatrix and places the output in omatrix.

EFUNC*/

void IZigzagMatrix(int *imatrix, int *omatrix) {
    BEGIN("IZigzagMatrix");
    int *tptr;

    for (tptr = zigzag_index; tptr < zigzag_index + BLOCKSIZE; tptr++) {
        *(omatrix++) = imatrix[*tptr];
    }
}
/*BFUNC

ZigzagMatrix() performs a zig-zag translation on the input imatrix
and puts the output in omatrix.

EFUNC*/

void ZigzagMatrix(int *imatrix, int *omatrix) {
    BEGIN("ZigzagMatrix");
    int *tptr;

    for (tptr = zigzag_index; tptr < zigzag_index + BLOCKSIZE; tptr++) {
        omatrix[*tptr] = *(imatrix++);
    }
}
/*BFUNC

PrintMatrix() prints an 8x8 matrix in row/column form. 

EFUNC*/

void PrintMatrix(int *matrix) {
    BEGIN("PrintMatrix");
    int i, j;

    if (matrix) {
        for (i = 0; i < BLOCKHEIGHT; i++) {
            for (j = 0; j < BLOCKWIDTH; j++) {
                printf("%6d ", *(matrix++));
            }
            printf("\n");
        }
    } else {
        printf("Null\n");
    }
}

/*BFUNC

ClearMatrix() sets all the elements of a matrix to be zero.

EFUNC*/

void ClearMatrix(int *matrix) {
    BEGIN("ClearMatrix");
    int *mptr;

    for (mptr = matrix; mptr < matrix + BLOCKSIZE; mptr++) {
        *mptr = 0;
    }
}

/*END*/
