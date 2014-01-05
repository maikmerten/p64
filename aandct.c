/*
 * Written by Maik Merten, source released into the public domain by the author.
 * No warranties or guarantees of any kind.
 * 
 * 
 * Implements the DCT approximation as defined by Arai, Agui, and Nakajima (AAN).
 * 
 * Originally described in "A Fast DCT-SQ Scheme for Images", published in
 * IEICE TRANSACTIONS (1976-1990)   Vol.E71   No.11   pp.1095-1097
 * Publication Date: 1988/11/25
 * 
 */

#include "globals.h"

/*PUBLIC*/

extern void AanDct();
extern void AanIDct();

/*PRIVATE*/

#define FLOAT double

#define A1 0.707106781186548 // cos(pi*4/16)
#define A2 0.541196100146197 // cos(pi*6/16)*sqrt(2)
#define A3 A1
#define A4 1.30656296487638 // cos(pi*2/16)*sqrt(2)
#define A5 0.382683432365090 // cos(pi*6/16)

#define S0 0.353553390593274 // 1/(2*sqrt(2))
#define S1 0.254897789552080 // 1/(4 * cos(1 * pi/16))
#define S2 0.270598050073099 // 1/(4 * cos(2 * pi/16))
#define S3 0.300672443467523 // 1/(4 * cos(3 * pi/16))
#define S4 0.353553390593274 // 1/(4 * cos(4 * pi/16))
#define S5 0.449988111568208 // 1/(4 * cos(5 * pi/16))
#define S6 0.653281482438188 // 1/(4 * cos(6 * pi/16))
#define S7 1.28145772387075  // 1/(4 * cos(7 * pi/16))

/*START*/

/*BFUNC

AaNDCT() implements the AAN forward dct.

EFUNC*/

void AanDct(int* x, int* y) {
    int idx0,idx1,idx2,idx3,idx4,idx5,idx6,idx7;
    int pass,offset,increment,i;
    FLOAT t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,tmp;
    FLOAT pass0out[64];
    
    printf("\n\n");
    
    for(pass=0; pass < 2; ++pass) {

        if(pass == 0) {
            // rows
            increment = 1;
            offset = 8;
        } else {
            // columns
            increment = 8;
            offset = 1;
        }

        for(i=0; i < 8; ++i) {
            
            idx0 = (i * offset);
            idx1 = idx0 + increment;
            idx2 = idx1 + increment;
            idx3 = idx2 + increment;
            idx4 = idx3 + increment;
            idx5 = idx4 + increment;
            idx6 = idx5 + increment;
            idx7 = idx6 + increment;
            
            if(pass == 0) {
                printf("row %d %d %d %d %d %d %d %d\n", idx0,idx1,idx2,idx3,idx4,idx5,idx6,idx7);
            } else {
                printf("col %d %d %d %d %d %d %d %d\n", idx0,idx1,idx2,idx3,idx4,idx5,idx6,idx7);
            }
            
            if(pass == 0) {
                t0 = (FLOAT) x[idx0];
                t1 = (FLOAT) x[idx1];
                t2 = (FLOAT) x[idx2];
                t3 = (FLOAT) x[idx3];
                t4 = (FLOAT) x[idx4];
                t5 = (FLOAT) x[idx5];
                t6 = (FLOAT) x[idx6];
                t7 = (FLOAT) x[idx7];
            } else {
                t0 = pass0out[idx0];
                t1 = pass0out[idx1];
                t2 = pass0out[idx2];
                t3 = pass0out[idx3];
                t4 = pass0out[idx4];
                t5 = pass0out[idx5];
                t6 = pass0out[idx6];
                t7 = pass0out[idx7];
            }

            // phase 1
            t8 = t0 + t7;
            t9 = t1 + t6;
            t10 = t2 + t5;
            t11 = t3 + t4;
            t12 = (-t4) + t3;
            t13 = (-t5) + t2;
            t14 = (-t6) + t1;
            t15 = (-t7) + t0;
            
            // phase 2
            t0 = t8 + t11;
            t1 = t9 + t10;
            t2 = t9 + (-t10);
            t3 = t8 + (-t11);
            t4 = (-t12) + (-t13);
            t5 = t13 + t14;
            t6 = t14 + t15;
            t7 = t15;
            
            // phase 3
            t8 = t0 + t1;
            t9 = t0 + (-t1);
            t10 = t2 + t3;
            t11 = t3;
            t12 = t4;
            t13 = t5;
            t14 = t6;
            t15 = t7;
            
            // phase 4
            t0 = t8;
            t1 = t9;
            t2 = t10 * A1;
            t3 = t11;
            tmp = (t12 + t14) * A5;
            t4 = (t12 * A2) + tmp;
            t5 = t13 * A3;
            t6 = (t14 * A4) + tmp;
            t7 = t15;
            
            // phase 5
            t8 = t0;
            t9 = t1;
            t10 = (t2 + t3);
            t11 = (-t2) + t3;
            t12 = t4;
            t13 = t5 + t7;
            t14 = t6;
            t15 = (-t5) + t7;
            
            // phase 6
            t0 = t8;
            t1 = t9;
            t2 = t10;
            t3 = t11;
            t4 = t12 + t15;
            t5 = t13 + t14;
            t6 = t13 + (-t14);
            t7 = (-t12) + t15;
            
            
            if(pass == 0) {
                pass0out[idx0] = t0 * S0;
                pass0out[idx4] = t1 * S1;
                pass0out[idx2] = t2 * S2;
                pass0out[idx6] = t3 * S3;
                pass0out[idx5] = t4 * S4;
                pass0out[idx1] = t5 * S5;
                pass0out[idx7] = t6 * S6;
                pass0out[idx3] = t7 * S7;
            } else {
                y[idx0] = (int) (t0 * S0 + 0.5);
                y[idx4] = (int) (t1 * S1 + 0.5);
                y[idx2] = (int) (t2 * S2 + 0.5);
                y[idx6] = (int) (t3 * S3 + 0.5);
                y[idx5] = (int) (t4 * S4 + 0.5);
                y[idx1] = (int) (t5 * S5 + 0.5);
                y[idx7] = (int) (t6 * S6 + 0.5);
                y[idx3] = (int) (t7 * S7 + 0.5);
            }
            
        }
    }

}


/*BFUNC

AanIDCT() implements the AAN inverse dct.

EFUNC*/

void AanIDct(int* x, int* y) {
    
}

/*END*/

