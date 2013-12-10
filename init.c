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

#include "globals.h"
#include "ctables.h"

DHUFF *MBADHuff;
DHUFF *MVDDHuff;
DHUFF *CBPDHuff;
DHUFF *T1DHuff;
DHUFF *T2DHuff;
DHUFF *T3DHuff;

EHUFF *MBAEHuff;
EHUFF *MVDEHuff;
EHUFF *CBPEHuff;
EHUFF *T1EHuff;
EHUFF *T2EHuff;
EHUFF *T3EHuff;

inithuff()
{
  MBADHuff = MakeDHUFF();
  MVDDHuff = MakeDHUFF();
  CBPDHuff = MakeDHUFF();
  T1DHuff = MakeDHUFF();
  T2DHuff = MakeDHUFF();
  T3DHuff = MakeDHUFF();

  MBAEHuff = MakeEHUFF(40);
  MVDEHuff = MakeEHUFF(40);
  CBPEHuff = MakeEHUFF(70);
  T1EHuff = MakeEHUFF(8192);
  T2EHuff = MakeEHUFF(8192);
  T3EHuff = MakeEHUFF(20);

/*  printf("Loading MBA\n");*/
  LoadDTable(MBACoeff,MBADHuff);
  LoadETable(MBACoeff,MBAEHuff);
/*  printf("Loading MVD\n");*/
  LoadDTable(MVDCoeff,MVDDHuff);
  LoadETable(MVDCoeff,MVDEHuff);
/*  printf("Loading CBP\n");*/
  LoadDTable(CBPCoeff,CBPDHuff);
  LoadETable(CBPCoeff,CBPEHuff);
/*  printf("Loading T1\n");*/
  LoadDTable(TCoeff1,T1DHuff);
  LoadETable(TCoeff1,T1EHuff);
/*  printf("Loading T2\n");*/
  LoadDTable(TCoeff2,T2DHuff);
  LoadETable(TCoeff2,T2EHuff);
/*  printf("Loading T3\n");*/
  LoadDTable(TYPE3Coeff,T3DHuff);
  LoadETable(TYPE3Coeff,T3EHuff);

/*  PrintEhuff(T1EHuff);*/
}

