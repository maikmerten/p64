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
marker.c

This file contains most of the marker information.

************************************************************
*/

/*LABEL marker.c */

#include "globals.h"
#include "marker.h"

/*PUBLIC*/

extern void WritePictureHeader();
extern void ReadPictureHeader();
extern void WriteGOBHeader();
extern int ReadHeaderHeader();
extern void ReadHeaderTrailer();
extern void ReadGOBHeader();
extern void WriteMBHeader();
extern int ReadMBHeader();

/*PRIVATE*/

extern int TemporalReference;
extern int PType;
extern int Type2;
extern int MType;
extern int GQuant;
extern int MQuant;
extern int MVDH;
extern int MVDV;
extern int CBP;
extern int ParityEnable;
extern int PSpareEnable;
extern int GSpareEnable;
extern int Parity;
extern int PSpare;
extern int GSpare;
extern int GRead;
extern int MBA;
extern int LastMBA;

extern int LastMVDV;
extern int LastMVDH;

extern int LastMType;

extern int QuantMType[];
extern int CBPMType[];
extern int MFMType[];


extern FSTORE *CFS;

extern int bit_set_mask[];
extern int extend_mask[];

extern DHUFF *MBADHuff;
extern DHUFF *MVDDHuff;
extern DHUFF *CBPDHuff;
extern DHUFF *T3DHuff;

extern EHUFF *MBAEHuff;
extern EHUFF *MVDEHuff;
extern EHUFF *CBPEHuff;
extern EHUFF *T3EHuff;

extern int NumberBitsCoded;

int MacroAttributeBits=0;
int MotionVectorBits=0;

/*START*/
/*BFUNC

WritePictureHeader() writes the header of picture out to the stream.
One of these is necessary before every frame is transmitted.

EFUNC*/

void WritePictureHeader()
{
  BEGIN("WritePictureHeader");

  mputv(PSC_LENGTH,PSC);
  mputv(5,TemporalReference);
#ifdef VERSION_1.0
  mputv(13,PType);
  if (ParityEnable)
    {
      mputb(1);
      mputv(8,ParityFS(CFS));
    }
  else
    mputb(0);  /* No Parity */
  if (PSpareEnable)
    {
      mputb(1);
      mputv(16,PSpare);
    }
  else
    mputb(0);  /* No Spare */
#else
  mputv(6,PType);
  if (PSpareEnable)
    {
      mputb(1);
      mputv(8,PSpare);
    }
  mputb(0);  /* No Spare */
#endif
}

/*BFUNC

ReadPictureHeader() reads the header off of the stream. It assumes
that the first PSC has already been read in. (Necessary to tell the
difference between a new picture and another GOB.)

EFUNC*/

void ReadPictureHeader()
{
  BEGIN("ReadPictureHeader");

  TemporalReference = mgetv(5);

#ifdef VERSION_1.0
  PType = mgetv(13);
  if (mgetb())
    {
      ParityEnable = 1;
      Parity = mgetv(8);
    }
  else
    ParityEnable = 0;
  if (mgetb())
    {
      PSpareEnable=1;
      PSpare = mgetv(16);
    }
  else
    PSpareEnable = 0;
#else
  PType = mgetv(6);
  for(PSpareEnable = 0;mgetb();)
    {
      PSpareEnable=1;
      PSpare = mgetv(8);
    }
#endif
}

/*BFUNC

WriteGOBHeader() writes a GOB out to the stream.

EFUNC*/

void WriteGOBHeader()
{
  BEGIN("WriteGOBHeader");

  mputv(GBSC_LENGTH,GBSC);
  mputv(4,GRead+1);
#ifdef VERSION_1.0
  mputv(6,Type2);
#endif
  mputv(5,GQuant);
#ifdef VERSION_1.0
  if (GSpareEnable)
    {
      mputb(1);
      mputv(16,GSpare);
    }
  else
    mputb(0);
#else
  if (GSpareEnable)
    {
      mputb(1);
      mputv(8,GSpare);
    }
  mputb(0);
#endif
}

/*BFUNC

ReadHeaderTrailer() reads the trailer of the PSC or GBSC code. It is
used to determine whether it is just a GOB or a new picture.

EFUNC*/

void ReadHeaderTrailer()
{
  BEGIN("ReadHeaderTrailer");

  GRead = mgetv(4)-1;
}

/*BFUNC

ReadHeaderHeader() reads the header header off of the stream. This is
a precursor to the GOB read or the PSC read. It returns -1 on error.

EFUNC*/

int ReadHeaderHeader()
{
  BEGIN("ReadHeaderHeader");
  int input;

  if ((input = mgetv(GBSC_LENGTH)) != GBSC)
    {
      if (seof()==0)
	{
	  WHEREAMI();
	  printf("Illegal GOB Start Code. Read: %d\n",input);
	}
      return(-1);
    }
  return(0);
}


/*BFUNC

ReadGOBHeader() reads the GOB information off of the stream. We assume
that the first bits have been read in by ReadHeaderHeader... or some
such routine.

EFUNC*/

void ReadGOBHeader()
{
  BEGIN("ReadGOBHeader");

#ifdef VERSION_1.0
  Type2 = mgetv(6);
#endif
  GQuant = mgetv(5);
#ifdef VERSION_1.0
  if (mgetb())
    {
      GSpareEnable = 1;
      GSpare = mgetv(16);
    }
  else
    GSpareEnable=0;
#else
  for(GSpareEnable=0;mgetb();)
    {
      GSpareEnable = 1;
      GSpare = mgetv(8);
    }
#endif
}

/*BFUNC

WriteMBHeader() writes a macro-block out to the stream.

EFUNC*/

void WriteMBHeader()
{
  BEGIN("WriteMBHeader");
  int TempH,TempV,Start;
  
  Start=swtell();
  if (!Encode(MBA,MBAEHuff))
    {
      WHEREAMI();
      printf("Attempting to write an empty Huffman code.\n");
      exit(ERROR_HUFFMAN_ENCODE);
    }
  if (!Encode(MType,T3EHuff))
    {
      WHEREAMI();
      printf("Attempting to write an empty Huffman code.\n");
      exit(ERROR_HUFFMAN_ENCODE);
    }
  if (QuantMType[MType])
    mputv(5,MQuant);

  NumberBitsCoded=0;
  if (MFMType[MType])
    {
      if ((!MFMType[LastMType])||(MBA!=1)||
	  (LastMBA==-1)||(LastMBA==10)||(LastMBA==21))
	{
	  if (!Encode(MVDH&0x1f,MVDEHuff)||
	       !Encode(MVDV&0x1f,MVDEHuff))
	    {
	      WHEREAMI();
	      printf("Cannot encode motion vectors.\n");
	    }
	}
      else
	{
	  TempH = MVDH - LastMVDH;
	  if (TempH < -16) TempH += 32;
	  if (TempH > 15) TempH -= 32;
	  TempV = MVDV - LastMVDV;
	  if (TempV < -16) TempV += 32;
	  if (TempV > 15) TempV -= 32;
	  if (!Encode(TempH&0x1f,MVDEHuff)||!Encode(TempV&0x1f,MVDEHuff))
	    {
	      WHEREAMI();
	      printf("Cannot encode motion vectors.\n");
	    }
	}
      LastMVDV = MVDV;
      LastMVDH = MVDH;
    }
  else
    {
      LastMVDV=LastMVDH=MVDV=MVDH=0; /* Redundant in most cases */
    }

  MotionVectorBits+=NumberBitsCoded;
  if (CBPMType[MType])
    {
      if (!Encode(CBP,CBPEHuff))
	{
	  WHEREAMI();
	  printf("CBP write error\n");
	  exit(-1);
	}
    }
  MacroAttributeBits+=(swtell()-Start);
}

/*BFUNC

ReadMBHeader() reads the macroblock header from the stream.

EFUNC*/

int ReadMBHeader()
{
  BEGIN("ReadMBHeader");
  int ReadMVDH,ReadMVDV;

  do
    {
      MBA = Decode(MBADHuff);
    }
  while(MBA == 34);  /* Get rid of stuff bits */
  if (MBA == 35) return(-1); /* Start of Picture Headers */

  LastMType = MType;
  MType = Decode(T3DHuff);
  if (QuantMType[MType])
    MQuant = mgetv(5);
  if (MFMType[MType])
    {
      if ((!MFMType[LastMType])||(MBA!=1)||
	  (LastMBA==-1)||(LastMBA==10)||(LastMBA==21))
	  {
	    MVDH = Decode(MVDDHuff);
	    if (MVDH & bit_set_mask[4])
	      MVDH |= extend_mask[4];
	    MVDV = Decode(MVDDHuff);
	    if (MVDV & bit_set_mask[4])
	      MVDV |= extend_mask[4];
	  }
      else
	{
	  ReadMVDH = Decode(MVDDHuff);
	  if (ReadMVDH & bit_set_mask[4])
	    ReadMVDH |= extend_mask[4];
	  MVDH += ReadMVDH;
	  
	  ReadMVDV = Decode(MVDDHuff);
	  if (ReadMVDV & bit_set_mask[4])
	    ReadMVDV |= extend_mask[4];
	  MVDV += ReadMVDV;

	  if (MVDH < -16) MVDH += 32;
	  if (MVDH > 15) MVDH -= 32;
	  if (MVDV < -16) MVDV += 32;
	  if (MVDV > 15) MVDV -= 32;
	}
    }
  else
    {
      MVDV=MVDH=0;  /* Theoretically redundant */
    }
  if (CBPMType[MType])
    CBP = Decode(CBPDHuff);
  return(0);
}

/*END*/
