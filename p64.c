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
p64.c

This is the file that has the "main" routine and all of the associated
high-level routines.  This has been kludged from the JPEG encoder, so
there is more extensibility than is really necessary.

************************************************************
*/

/*LABEL p64.c */

#include <stdio.h>
#include "globals.h"

/*PUBLIC*/

int main();
extern void p64EncodeSequence();
extern void p64EncodeFrame();
extern void p64EncodeGOB();
extern void p64DecodeSequence();
extern int p64DecodeGOB();
extern void PrintImage();
extern void PrintFrame();
extern void MakeImage();
extern void MakeFrame();
extern void MakeFstore();
extern void MakeStat();
extern void MakeRate();
extern void SetCCITT();
extern void Help();
extern void MakeFileNames();
extern void VerifyFiles();

static void ExecuteQuantization();
static void CallOracle();
static void p64EncodeMDU();
static void ReadCompressMDU();
static void WriteMDU();
static void DecodeSaveMDU();
static int DecompressMDU();

/*PRIVATE*/

#define SwapFS(fs1,fs2) {FSTORE *ftemp;ftemp = fs1;fs1 = fs2;fs2 = ftemp;}

IMAGE *CImage=NULL;
FRAME *CFrame=NULL;
FSTORE *CFS=NULL;
FSTORE *OFS=NULL;
STAT *CStat=NULL;
STAT *RStat=NULL;
RATE *RCStore=NULL;

int BlockJ[] = {0,0,0,0,1,2};
int BlockV[] = {0,0,1,1,0,0};
int BlockH[] = {0,1,0,1,0,0};

char *DefaultSuffix[]={".Y",".U",".V"};

/* CCITT p*64 Marker information */

int TemporalReference=1;
int TemporalOffset=0;
int PType=0x0;
int Type2=0x0;
int MType=0x0;
int GQuant=8;
int MQuant=8;
int MVDH=0;
int MVDV=0;
int VAR=0;
int VAROR=0;
int MWOR=0;
int LastMVDV=0;
int LastMVDH=0;
int CBP=0x3f;
int ParityEnable=0;
int PSpareEnable=0;
int GSpareEnable=0;
int Parity=0;
int PSpare=0;
int GSpare=0;
int GRead=0;
int MBA=0;
int LastMBA=0;

int LastMType=0; /* Last MType */

/* System Definitions */

int ImageType=IT_NTSC;

int NumberMDU=0;
int CurrentMDU=0;
int NumberGOB=0;
int CurrentGOB=0;

int CurrentFrame=0;
int StartFrame=0;
int LastFrame=0;
int PreviousFrame=0;
int NumberFrames=0;
int TransmittedFrames=0;
int FrameRate=30;

int FrameSkip=1;

/* Stuff for RateControl */

int FileSizeBits=0;
int Rate=0;
int BufferOffset=0;  /*Number of bits assumed for initial buffer. */
int QDFact=1;
int QOffs=1;
int QUpdateFrequency=11;
int QUse=0;
int QSum=0;

/* Some internal parameters for rate control */

#define DEFAULT_QUANTIZATION 8

int InitialQuant=0;

/* DCT Coefficient Thresholds for coding blocks */

int CBPThreshold=1;  /* abs threshold before we use CBP */

/* MC Threshold for coding blocks through filter*/

#define D_FILTERTHRESHOLD 6

/* Intra forced every so many blocks */

#define SEQUENCE_INTRA_THRESHOLD 131

/* Parser stuff */

extern double Memory[];

/* Stuff for encoding (temporary integer storage) */

static int inputbuf[10][64];
static int output[64];

/* Stuff for Motion Compensation */

extern int SearchLimit;
extern int bit_set_mask[];
extern int MeX[];
extern int MeY[];
extern int MeVal[];
extern int MeOVal[];
extern int MeVAR[1024];
extern int MeVAROR[1024];
extern int MeMWOR[1024];

/* Book-keeping stuff */

int ErrorValue=0;
int Loud=MUTE;
int Trace=NULL;
int Verbose=0;
int ForceCIF=0; /* Forces CIF format - superset will hold any other format */

int Oracle=0;
int MQuantEnable;
int UseQuant;
int OracleGQuant,OracleMQuant;
int OracleMQuantEnable,OracleMType;

/* Statistics */

int NumberNZ=0;
int FirstFrameBits=0;
int NumberOvfl=0;
extern int MotionVectorBits;
extern int MacroAttributeBits;
extern int CodedBlockBits;
int YCoefBits=0;
int UCoefBits=0;
int VCoefBits=0;
extern int EOBBits;
int TotalBits,LastBits;
int MacroTypeFrequency[10];
int YTypeFrequency[10];
int UVTypeFrequency[10];

unsigned char **LastIntra;  /* Used for intra forcing (once per 132 frames) */

/* Coding control */

int QuantMType[] =    {0,1,0,1,0,0,1,0,0,1}; /* Quantization used */
int CBPMType[] =      {0,0,1,1,0,1,1,0,1,1}; /* CBP used in coding */
int IntraMType[] =    {1,1,0,0,0,0,0,0,0,0}; /* Intra coded macroblock */
int MFMType[] =       {0,0,0,0,1,1,1,1,1,1}; /* Motion forward vector used */
int FilterMType[] =   {0,0,0,0,0,0,0,1,1,1}; /* Filter flags */
int TCoeffMType[] =   {1,1,1,1,0,1,1,0,1,1}; /* Transform coeff. coded */

/* DCT Stuff */
/* Functional Declarations */

vFunc *UseDct = ChenDct;
vFunc *UseIDct = ChenIDct;

#define DefaultDct (*UseDct)
#define DefaultIDct (*UseIDct)

#define BufferContents() (mwtell() + BufferOffset -\
			  (((CurrentGOB*NumberMDU)+CurrentMDU)\
			   *Rate*FrameSkip\
			  /(NumberGOB*NumberMDU*FrameRate)))
#define BufferSize() (Rate/4) /*In bits */

/*START*/
/*BFUNC

main() is the first routine called by program activation. It parses
the input command line and sets parameters accordingly.

EFUNC*/

int main(argc,argv)
     int argc;
     char **argv;
{
  BEGIN("main");
  int i,p,s;

  MakeImage();   /* Initialize storage */
  MakeFrame();
  MakeFstore();
  inithuff();    /* Put Huffman tables on */
  initmc();      /* Put Motion Comp stuff on */
  if (argc==1)
    {
      Help();
      exit(-1);
    }
  for(s=0,p=0,i=1;i<argc;i++)
    {
      if (!strcmp("-NTSC",argv[i]))
	{
	  ImageType = IT_NTSC;
	}
      else if (!strcmp("-CIF",argv[i]))
	{
	  ImageType = IT_CIF;
	}
      else if (!strcmp("-QCIF",argv[i]))
	{
	  ImageType = IT_QCIF;
	}
      else if (*(argv[i]) == '-')
 	{
	  switch(*(++argv[i]))
 	    {
	    case 'a':
	      CurrentFrame = atoi(argv[++i]);
	      StartFrame=CurrentFrame;
	      break;
	    case 'b':
	      LastFrame = atoi(argv[++i]);
	      break;
 	    case 'c': 
	      ForceCIF=1;
 	      break; 
 	    case 'd': 
	      CImage->p64Mode |= P_DECODER;
 	      break; 
	    case 'f':
	      FrameRate = atoi(argv[++i]);
	      break;
	    case 'i':
	      SearchLimit = atoi(argv[++i]);
	      BoundValue(SearchLimit,1,31,"SearchLimit");
	      break;
	    case 'k':
	      FrameSkip = atoi(argv[++i]);
	      break;
	    case 'l':
	      Loud = atoi(argv[++i]);
	      break;
	    case 'o':
	      Oracle=1;
	      break;
	    case 'p':
	      ParityEnable=1;
	      break;
	    case 'q':
	      InitialQuant=atoi(argv[++i]);
	      BoundValue(InitialQuant,1,31,"InitialQuant");
	      break;
	    case 'r':
	      Rate = (atoi(argv[++i]));
	      break;
	    case 's':
	      CImage->StreamFileName = argv[++i];
	      break;
	    case 'v':
	      Verbose=1;
	      break;
	    case 'x':
	      FileSizeBits = (atoi(argv[++i]));
	      break;
	    case 'y':
	      UseDct = ReferenceDct;
	      UseIDct = ReferenceIDct;
	      break;
	    case 'z':
	      strcpy(CFrame->ComponentFileSuffix[s++],argv[++i]);
	      break;
	    default:
	      WHEREAMI();
	      printf("Illegal Option %c\n",*argv[i]);
	      exit(ERROR_BOUNDS);
	      break;
	    }
	}
      else
	{
	  strcpy(CFrame->ComponentFilePrefix[p++],argv[i]);
	}
    }
  if (!CImage->StreamFileName)
    {
      if (!(CImage->StreamFileName =
	    (char *) calloc(strlen(CFrame->ComponentFilePrefix[0])+6,
			    sizeof(char))))
	{
	  WHEREAMI();
	  printf("Cannot allocate string for StreamFileName.\n");
	  exit(ERROR_MEMORY);
	}
      sprintf(CImage->StreamFileName,
	      "%s.p64",CFrame->ComponentFilePrefix[0]);
    }
  if (Oracle)
    {
      initparser();
      parser();
    }
  switch(ImageType)
    {
    case IT_NTSC:
      PType=0x04;
      PSpareEnable=1;
      PSpare=0x8c;
      break;
    case IT_CIF:
      PType=0x04;
      break;
    case IT_QCIF:
      PType=0x00;
      break;
    default:
      WHEREAMI();
      printf("Image Type not supported: %d\n",ImageType);
      break;
    }
  if(!(GetFlag(CImage->p64Mode,P_DECODER)))
    {
      SetCCITT();
      if (CurrentFrame>LastFrame)
	{
	  WHEREAMI();
	  printf("Need positive number of frames.\n");
	  exit(ERROR_BOUNDS);
	}
      NumberFrames = LastFrame-CurrentFrame+1;
      p64EncodeSequence();
    }
  else
    {
      p64DecodeSequence();
    }
  exit(ErrorValue);
}

/*BFUNC

ExecuteQuantization() is used to find the appropriate quantization for
the current sequence.  It calls the oracle if the flag is set.

EFUNC*/

static void ExecuteQuantization()
{
  BEGIN("ExecuteQuantization");
  int CurrentSize;

  CurrentSize=BufferContents();
  OracleGQuant = (CurrentSize/QDFact) + QOffs;
  OracleMQuant=OracleGQuant;
  if (Verbose)
    {
      printf("BufferContents: %d  New Q1: %d\n",
	     CurrentSize,GQuant);
    }
  if (Oracle)  /* If oracle, then consult oracle */
    {
      CallOracle(1);
      OracleGQuant = (int)  Memory[L_GQUANT];
      OracleMQuant = (int)  Memory[L_MQUANT];
    }
  if (OracleGQuant<1) OracleGQuant=1;
  if (OracleGQuant>31) OracleGQuant=31;
  if (OracleMQuant<1) OracleMQuant=1;
  if (OracleMQuant>31) OracleMQuant=31;
}

/*BFUNC

CallOracle() calls the program interpreter by setting all the relevant
parameters and executing the program designated by value.

EFUNC*/

static void CallOracle(value)
     int value;
{
  BEGIN("CallOracle");

  Memory[L_GQUANT] = (double) GQuant;
  Memory[L_MQUANT] = (double) MQuant;
  Memory[L_MQUANTENABLE] = (double) MQuantEnable;
  Memory[L_MTYPE] = (double) MType; /* Suggested MType */
  Memory[L_BD] = (double) MeOVal[Bpos(CurrentGOB,
				      CurrentMDU,0,0)];
  Memory[L_DBD] = (double) MeVal[Bpos(CurrentGOB,
				      CurrentMDU,0,0)];
  Memory[L_VAROR] = (double) MeVAROR[Bpos(CurrentGOB,
					  CurrentMDU,0,0)];
  Memory[L_VAR] = (double) MeVAR[Bpos(CurrentGOB,
				      CurrentMDU,0,0)];
  Memory[L_MWOR] = (double) MeMWOR[Bpos(CurrentGOB,
					CurrentMDU,0,0)];
  Memory[L_RATE] = (double) Rate;
  Memory[L_BUFFERSIZE] = (double) BufferSize();
  Memory[L_BUFFERCONTENTS] = (double) BufferContents();
  Memory[L_QDFACT] = (double) QDFact;
  Memory[L_QOFFS] = (double) QOffs;
  Execute(value);
}

/*BFUNC

p64EncodeSequence() encodes the sequence defined by the CImage and
CFrame structures.

EFUNC*/

void p64EncodeSequence()
{
  BEGIN("p64EncodeSequence");
  
  MakeStat();
  MakeRate();
  MakeIob(READ_IOB);
  InitFS(CFS);
  ClearFS(CFS);
  InitFS(OFS);
  ClearFS(OFS);
  swopen(CImage->StreamFileName);
  if (Loud > MUTE)
    {
      PrintImage();
      PrintFrame();
    }
  if (FileSizeBits)  /* Rate is determined by bits/second. */
    Rate=(FileSizeBits*FrameRate)/(FrameSkip*(LastFrame-CurrentFrame+1));
  if (Rate)
    {
      QDFact = (Rate/320);
      QOffs = 1;
      if (!InitialQuant)
	{
	  InitialQuant = 10000000/Rate;
	  if (InitialQuant>31) InitialQuant=31;
	  else if (InitialQuant<1) InitialQuant=1;
	  printf("Rate: %d   QDFact: %d  QOffs: %d\n",
		 Rate,QDFact,QOffs);
	  printf("Starting Quantization: %d\n",InitialQuant);
	}
    }
  if (!InitialQuant)  InitialQuant=DEFAULT_QUANTIZATION;
  GQuant=MQuant=InitialQuant;
  BufferOffset=0;
  TotalBits=0;
  NumberOvfl=0;
  FirstFrameBits=0;
  printf("START>SEQUENCE\n");
  TransmittedFrames=0;
  while(CurrentFrame <= LastFrame)
    {
      p64EncodeFrame();
    }
  /*limit file growth*/
  if (CurrentFrame>LastFrame+1) {CurrentFrame=LastFrame+1;}
  TemporalReference = CurrentFrame % 32;
  WritePictureHeader();
  swclose();
/*
  SaveMem(CFS->fs[0]->mem,"XX");
  SaveMem(OFS->fs[0]->mem,"YY");
*/
  printf("END>SEQUENCE\n");
  printf("Bits for first frame: %d   Number of buffer overflows: %d\n",
	 FirstFrameBits,NumberOvfl);
}


/*BFUNC

p64EncodeFrame() encodes a single image frame.

EFUNC*/

void p64EncodeFrame()
{
  BEGIN("p64EncodeFrame");
  int x;
  
  printf("START>Frame: %d\n",CurrentFrame);
  MakeFileNames();
  VerifyFiles();
  ReadIob();
  InstallFS(0,CFS);
  if (CurrentFrame!=StartFrame)
    GlobalMC();
  TemporalReference = CurrentFrame % 32;
  WritePictureHeader();

  for(x=0;x<10;x++) /* Initialize Statistics */
    {
      MacroTypeFrequency[x]=0;
      YTypeFrequency[x]=0;
      UVTypeFrequency[x]=0;
    }
  MotionVectorBits=MacroAttributeBits=0;
  YCoefBits=UCoefBits=VCoefBits=EOBBits=0;
  QUse=QSum=0;
  NumberNZ=0;

  for(CurrentGOB=0;CurrentGOB<NumberGOB;CurrentMDU=0,CurrentGOB++)
    p64EncodeGOB();

  RCStore[CurrentFrame-StartFrame].position=TotalBits;
  RCStore[CurrentFrame-StartFrame].baseq = GQuant;
  x = mwtell();
  LastBits = x - TotalBits;
  TotalBits = x;
  printf("Total No of Bits: %8d  Bits for Frame: %8d\n",
	 TotalBits,LastBits);
  if (Rate)
    {
      printf("Buffer Contents: %8d  out of: %8d\n",
	     BufferContents(),
	     BufferSize());
    }
  printf("MB Attribute Bits: %6d  MV Bits: %6d   EOB Bits: %6d\n",
	 MacroAttributeBits,MotionVectorBits,EOBBits);
  printf("Y Bits: %7d  U Bits: %7d  V Bits: %7d  Total Bits: %7d\n",
	 YCoefBits,UCoefBits,VCoefBits,(YCoefBits+UCoefBits+VCoefBits));
  printf("MV StepSize: %f  MV NumberNonZero: %f  MV NumberZero: %f\n",
	 (double) ((double) QSum)/((double)(QUse)),
	 (double) ((double) NumberNZ)/
	 ((double)(NumberGOB*NumberMDU*6)),
	 (double) ((double) (NumberGOB*NumberMDU*6*64)- NumberNZ)/
	 ((double)(NumberGOB*NumberMDU*6)));
  RCStore[CurrentFrame-StartFrame].size = LastBits;
  printf("Code MType: ");
  for(x=0;x<10;x++) printf("%5d",x);
  printf("\n");
  printf("Macro Freq: ");
  for(x=0;x<10;x++) printf("%5d",MacroTypeFrequency[x]);
  printf("\n");
  printf("Y     Freq: ");
  for(x=0;x<10;x++) printf("%5d",YTypeFrequency[x]);
  printf("\n");
  printf("UV    Freq: ");
  for(x=0;x<10;x++) printf("%5d",UVTypeFrequency[x]);
  printf("\n");

  SwapFS(CFS,OFS);

  Statistics();
  printf("END>Frame: %d\n",CurrentFrame);
  if (Rate)
    {
      if (CurrentFrame==StartFrame)
	{/* Begin Buffer at 0.5 size */
	  FirstFrameBits = TotalBits;
	  BufferOffset = (BufferSize()/2) - BufferContents();
	  printf("First Frame Reset Buffer by delta bits: %d\n",
		 BufferOffset);
	}
      /* Take off standard deduction afterwards. */
      BufferOffset -= (Rate*FrameSkip/FrameRate);
    }
  else if (CurrentFrame==StartFrame)
    FirstFrameBits = TotalBits;
  CurrentGOB=0;TransmittedFrames++;
  CurrentFrame+=FrameSkip;    /* Change GOB & Frame at same time */
}


/*BFUNC

p64EncodeGOB() encodes a group of blocks within a frame.

EFUNC*/

void p64EncodeGOB()
{
  BEGIN("p64EncodeGOB");
  double xValue,yValue;

  MQuantEnable=0;                         /* Disable MQuant */
  if ((Rate)&&(CurrentFrame!=StartFrame)) /* Change Quantization */
    { 
      ExecuteQuantization();
      GQuant=OracleGQuant;
    }
  switch (ImageType)
    {
    case IT_NTSC:
    case IT_CIF:
      GRead=CurrentGOB;
      break;
    case IT_QCIF:
      GRead=(CurrentGOB<<1);
      break;
    default:
      WHEREAMI();
      printf("Unknown Image Type: %d\n",ImageType);
      break;
    }
  WriteGOBHeader();
  
  LastMBA = -1; MType=0;
  for(;CurrentMDU<NumberMDU;CurrentMDU++)
    { /* MAIN LOOP */
      LastMType=MType;
      if ((Rate)&&(CurrentMDU)&&!(CurrentMDU%QUpdateFrequency)&&
	  (CurrentFrame!=StartFrame))
	{  /* Begin Buffer control */
	  ExecuteQuantization();
	  if (OracleMQuant!= GQuant)
	    {
	      if (MQuantEnable=OracleMQuantEnable)
		MQuant=OracleMQuant;
	    }
	  else MQuantEnable=0;
	}
      xValue = (double) MeOVal[Bpos(CurrentGOB,CurrentMDU,0,0)];
      yValue = (double) MeVal[Bpos(CurrentGOB,CurrentMDU,0,0)];
      xValue = xValue/256;
      yValue = yValue/256;
      MVDH = MeX[Bpos(CurrentGOB,CurrentMDU,0,0)];
      MVDV = MeY[Bpos(CurrentGOB,CurrentMDU,0,0)];
      VAR  = MeVAR[Bpos(CurrentGOB,CurrentMDU,0,0)];
      VAROR = MeVAROR[Bpos(CurrentGOB,CurrentMDU,0,0)];
      MWOR = MeMWOR[Bpos(CurrentGOB,CurrentMDU,0,0)];
      if (CurrentFrame!=StartFrame)   /* Intra vs. Inter decision */
	{
	  if ((VAR < 64) || (VAROR > VAR))
	    {                               /* (MC+Inter)mode */
	      if ((xValue < 1.0) ||
		  ((xValue < 3.0) && (yValue > (xValue*0.5))) ||
		  ((yValue > (xValue/1.1))))
		MType = 2;                    /* Inter mode */
	      else if (VAR < (double) D_FILTERTHRESHOLD) /* MC mode */
		MType = 5;                    /* No Filter MC */
	      else
		MType = 8;                    /* Filter MC */
	    }
	  else MType = 0; /*Intramode */
	  if (MQuantEnable)
	    {
	      MType++;
	    }
	  if (Oracle)  /* If oracle, then consult oracle */
	    {
	      CallOracle(0);
	      /* Maybe also look at Q? */
	      /* MQuant = (int)  Memory[L_MQuant]; */
	      /* MQuantEnable = (int)  Memory[L_Q2ENABLE]; */
	      MType = (int) Memory[L_MTYPE];             /* Oracle's Type 3 */
	    }
	}
      else
	MType = 0; /* We always start with Intramode */
      if (LastIntra[CurrentGOB][CurrentMDU]>SEQUENCE_INTRA_THRESHOLD)
	MType=0;  /* Code intra every 132 blocks */

      /* printf("[State %d]",MType);*/
      if ((Rate)&&(BufferContents()>BufferSize()))
	{
	  MVDH=MVDV=0; /* Motion vectors 0 */
	  MType=4;     /* No coefficient transmission */
	  NumberOvfl++;
	  WHEREAMI();
	  printf("Buffer Overflow!\n");
	}
      p64EncodeMDU();
    }
}

/*BFUNC

p64EncodeMDU(MDU,)
     ) encodes the MDU by read/compressing the MDU; encodes the MDU by read/compressing the MDU, then
writing it, then decoding it and accumulating statistics.

EFUNC*/

static void p64EncodeMDU()
{
  BEGIN("p64EncodeMDU");

  ReadCompressMDU();
  WriteMDU();
  DecodeSaveMDU();

  QUse++;                  /* Accumulate statistics */
  QSum+=UseQuant;
  if (MType < 10)
    MacroTypeFrequency[MType]++;
  else
    {
      WHEREAMI();
      printf("Illegal MType: %d\n",MType);
    }
}

/*BFUNC

ReadCompressMDU(MDU,)
     ) reads in the MDU; reads in the MDU, and attempts to compress it.
If the chosen MType is invalid, it finds the closest match.

EFUNC*/

static void ReadCompressMDU()
{
  BEGIN("ReadCompressMDU");
  int c,j,h,v,x;
  int *input;
  int total,accum,pmask;

  while(1)                        /* READ AND COMPRESS */
    {
      if (QuantMType[MType])
	{
	  UseQuant=MQuant;
	  GQuant=MQuant; /* Future MB Quant is now MQuant */
	}
      else UseQuant=GQuant;
      for(c=0;c<6;c++)
	{
	  input = &inputbuf[c][0];
	  j = BlockJ[c];
	  v = BlockV[c];
	  h = BlockH[c];
	  if (TCoeffMType[MType])
	    {
	      InstallIob(j);
	      MoveTo(CurrentGOB,CurrentMDU,h,v);
	      ReadBlock(input);
	      if (!IntraMType[MType])
		{
		  InstallFS(j,CFS);
		  MoveTo(CurrentGOB,CurrentMDU,h,v);
		  if (FilterMType[MType])
		    {
		      if (j)
			HalfSubFCompensate(input);
		      else
			SubFCompensate(input);
		    }
		  else if (MFMType[MType])
		    {
		      if (j)
			HalfSubCompensate(input);
		      else
			SubCompensate(input);
		    }
		  else
		    SubOverlay(input);
		}
	      DefaultDct(input,output);
	      BoundDctMatrix(output);
	      if (IntraMType[MType])
		{
		  CCITTFlatQuantize(output,8,UseQuant);
		  FlatBoundQuantizeMatrix(output);
		}
	      else
		{
		  CCITTQuantize(output,UseQuant,UseQuant);
		  BoundQuantizeMatrix(output);
		}
	      ZigzagMatrix(output,input);
	    }
	  else
	    for(x=0;x<64;x++) input[x] = 0;
	}
      if (!CBPMType[MType]) CBP = 0x3f;  /* VERIFY MType CBP */
      else
	{
	  for(pmask=0,CBP=0,total=0,c=0;c<6;c++)
	    {
	      input = &inputbuf[c][0];
	      for(accum=0,x=0;x<64;x++) accum += abs(input[x]);
	      if ((accum)&&(pmask==0)) pmask|=bit_set_mask[5-c];
	      if (accum>CBPThreshold) CBP |= bit_set_mask[5-c];
	      total+= accum;
	    }
	  if (!CBP)
	    {
	      if (pmask) CBP=pmask;
	      else 
		{
		  if (!FilterMType[MType])
		    {CBP=0;MType=4;continue;}
		  else {CBP=0;MType=7;continue;}
		}
	    }
	}
      if (IntraMType[MType]) LastIntra[CurrentGOB][CurrentMDU]=0;
      else LastIntra[CurrentGOB][CurrentMDU]++;
      return;                              /* IF HERE, THEN EXIT LOOP */
    }                                     /* GOOD ENCODING TYPE */
}

/*BFUNC

WriteMDU() writes out the MDU to the stream. The input buffer and
MType must already be set once this function is called.

EFUNC*/

static void WriteMDU()
{
  BEGIN("WriteMDU");
  int c,j,x;
  int *input;

  MBA = (CurrentMDU-LastMBA);     /* WRITE */
  WriteMBHeader();
  LastMBA = CurrentMDU;
  for(c=0;c<6;c++)
    {
      j = BlockJ[c];
      input = &inputbuf[c][0];
      if ((CBP & bit_set_mask[5-c])&&(TCoeffMType[MType]))
	{
	  if(j) {UVTypeFrequency[MType]++;}
	  else {YTypeFrequency[MType]++;}
	  CodedBlockBits=0;
	  if (CBPMType[MType])
	    {
	      CBPEncodeAC(0,input);
	    }
	  else
	    {
	      EncodeDC(*input);
	      EncodeAC(1,input);
	    }
	  if(!j){YCoefBits+=CodedBlockBits;}
	  else if(j==1){UCoefBits+=CodedBlockBits;}
	  else{VCoefBits+=CodedBlockBits;}
	  IZigzagMatrix(input,output);
	  if (IntraMType[MType])
	    ICCITTFlatQuantize(output,8,UseQuant);
	  else
	    ICCITTQuantize(output,UseQuant,UseQuant);
	  DefaultIDct(output,input);
	}
      else for(x=0;x<64;x++) input[x]=0;
    }
}

/*BFUNC

DecodeSaveMDU() does a decode on the MDU that was just encoded/decoded
and left on the inputbuf array.  The device is OFS if encoding mode is
on, else it is the Iob if decoding mode is on.

EFUNC*/

static void DecodeSaveMDU()
{
  BEGIN("DecodeSaveMDU");
  int c,j,h,v;
  int *input;

  for(c=0;c<6;c++)
    {
      j = BlockJ[c];
      v = BlockV[c];
      h = BlockH[c];
      input = &inputbuf[c][0];

      if (!IntraMType[MType])     /* DECODE */
	{
	  InstallFS(j,CFS);
	  MoveTo(CurrentGOB,CurrentMDU,h,v);
	  if (FilterMType[MType])
	    {
	      if (j)
		HalfAddFCompensate(input);
	      else
		AddFCompensate(input);
	    }
	  else if (MFMType[MType])
	    {
	      if (j)
		HalfAddCompensate(input);
	      else
		AddCompensate(input);
	    }
	  else
	    AddOverlay(input);
	}
      BoundIDctMatrix(input);       /* SAVE */
      if (!(GetFlag(CImage->p64Mode,P_DECODER)))
	InstallFS(j,OFS);
      else
	InstallIob(j);
      MoveTo(CurrentGOB,CurrentMDU,h,v);
      WriteBlock(input);
    }
}

/*BFUNC

p64DecodeSequence() decodes the sequence defined in the CImage and
CFrame structures.

EFUNC*/

void p64DecodeSequence()
{
  BEGIN("p64DecodeSequence");
  int SelfParity;
  int Active;
  int EndFrame=0;
  
  sropen(CImage->StreamFileName);
  if (ReadHeaderHeader()) /* nonzero on error or eof */
    {
      srclose();
      exit(ErrorValue);
    }
  Active=0;
  while(1)
    {
      if (!EndFrame)
	ReadHeaderTrailer();
      if ((GRead < 0)||(EndFrame))  /* End Of Frame */
	{
	  if (!EndFrame)
	    ReadPictureHeader();
	  else
	    TemporalReference++;
	  if (Active)
	    {
	      CopyIob2FS(CFS);
	      while(((CurrentFrame+TemporalOffset)%32) !=
		    TemporalReference)
		{
		  printf("END> Frame: %d\n",CurrentFrame);
		  MakeFileNames();
		  WriteIob();
		  CurrentFrame++;
		}
	      /* Might still be "Filler Frame" sent at the end of file */
	      if (ParityEnable)
		{
		  SelfParity = ParityFS(CFS);
		  if (Parity != SelfParity)
		    {
		      printf("Bad Parity: Self: %x  Sent: %x\n",
			     SelfParity,Parity);
		    }
		}
	    }
	  else
	    {	      /* First Frame */
	      if (ForceCIF)
		ImageType=IT_CIF;
	      else
		{
		  if (PType&0x04)
		    {
		      if (PSpareEnable&&PSpare==0x8c) ImageType=IT_NTSC;
		      else ImageType=IT_CIF;
		    }
		  else ImageType=IT_QCIF;
		}
	      SetCCITT();
	      if (Loud > MUTE)
		{
		  PrintImage();
		  PrintFrame();
		}
	      MakeIob(WRITE_IOB);
	      InitFS(CFS);
	      ClearFS(CFS);
	      TemporalOffset=(TemporalReference-CurrentFrame)%32;
	      Active=1;
	    }
	  if ((EndFrame)||(ReadHeaderHeader())) /* nonzero on error or eof */
	    break; /* Could be end of file */
	  printf("START>Frame: %d\n",CurrentFrame); /* Frame is for real */
	  continue;
	}
      EndFrame = p64DecodeGOB();                     /* Else decode the GOB */
    }
  srclose();
}

/*BFUNC

p64DecodeGOB() decodes the GOB block of the current frame.

EFUNC*/

int p64DecodeGOB()
{
  BEGIN("p64DecodeGOB");

  ReadGOBHeader();             /* Read the group of blocks header  */
  switch(ImageType)
    {
    case IT_NTSC:
    case IT_CIF:
      CurrentGOB = GRead;
      break;
    case IT_QCIF:
      CurrentGOB = (GRead>>1);
      break;
    default:
      WHEREAMI();
      printf("Unknown Image Type: %d.\n",ImageType);
      break;
    }
  if (CurrentGOB > NumberGOB)
    {
      WHEREAMI();
      printf("Buffer Overflow: Current:%d  Number:%d\n",
	     CurrentGOB, NumberGOB);
      return;
    }
  LastMBA = -1;               /* Reset the MBA and the other predictors  */
  LastMVDH = 0;
  LastMVDV = 0;
  while(ReadMBHeader()==0)
    {
      if (DecompressMDU()) return(1);
      DecodeSaveMDU();
    }
  return(0);
}

/*BFUNC

DecompressMDU() decompresses the current MDU of which the header has
already been read off of the stream.  It leaves the decoded result in
the inputbuf array.  It returns a 1 if an end of file has occurred.

EFUNC*/

static int DecompressMDU()
{
  BEGIN("DecompressMDU");
  int c,j,x;
  int *input;

  LastMBA = LastMBA + MBA;
  CurrentMDU = LastMBA;
  if (CurrentMDU >= NumberMDU)
    {
      if ((CurrentGOB == NumberGOB-1)&&(seof()))
	return(1);

      WHEREAMI();
      printf("Apparent MDU out of range: %d > %d.\n",CurrentMDU,NumberMDU);
      printf("CurrentGOB: %d LastMBA %d, MBA: %d\n",
	     CurrentGOB,LastMBA,MBA);
      printf("at bit position %d in stream\n",mrtell());
      CurrentMDU=0;
      LastMBA=0;
      return(0);
    }
  if (!CBPMType[MType]) CBP = 0x3f;
  if (QuantMType[MType])
    {
      UseQuant=MQuant;
      GQuant=MQuant;
    }
  else UseQuant=GQuant;
  for(c=0;c<6;c++)
    {
      j=BlockJ[c];
      input = &inputbuf[c][0];

      if ((CBP & bit_set_mask[5-c])&&(TCoeffMType[MType]))
	{
	  if (CBPMType[MType])
	    CBPDecodeAC(0,input);		      
	  else
	    {
	      *input = DecodeDC();
	      DecodeAC(1,input);
	    }
	  if (Loud > TALK)
	    {
	      printf("Cooked Input\n");
	      PrintMatrix(input);
	    }
	  IZigzagMatrix(input,output);
	  if (IntraMType[MType])
	    ICCITTFlatQuantize(output,8,UseQuant);
	  else
	    ICCITTQuantize(output,UseQuant,UseQuant);
	  DefaultIDct(output,input);
	}
      else  for(x=0;x<64;x++) input[x]=0; 
    }
  return(0);
}

/*BFUNC

PrintImage() prints the image structure to stdout.

EFUNC*/

void PrintImage()
{
  BEGIN("PrintImage");

  printf("*** Image ID: %x ***\n",CImage);
  if (CImage)
    {
      if (CImage->StreamFileName)
	{
	  printf("StreamFileName %s\n",CImage->StreamFileName);
	}
      printf("InternalMode: %d   Height: %d   Width: %d\n",
	     CImage->p64Mode,CImage->Height,CImage->Width);
    }
}

/*BFUNC

PrintFrame() prints the frame structure to stdout.

EFUNC*/

void PrintFrame()
{
  BEGIN("PrintFrame");
  int i;

  printf("*** Frame ID: %x ***\n",CFrame);
  if (CFrame)
    {
      printf("NumberComponents %d\n",
	     CFrame->NumberComponents);
      for(i=0;i<CFrame->NumberComponents;i++)
	{
	  printf("Component: FilePrefix: %s FileSuffix: %s\n",
		 ((*CFrame->ComponentFilePrefix[i]) ?
		  CFrame->ComponentFilePrefix[i] : "Null"),
		 ((*CFrame->ComponentFileSuffix[i]) ?
		  CFrame->ComponentFileSuffix[i] : "Null"));
	  printf("Height: %d  Width: %d\n",
		 CFrame->Height[i],CFrame->Width[i]);
	  printf("HorizontalFrequency: %d  VerticalFrequency: %d\n",
		 CFrame->hf[i],CFrame->vf[i]);
	  InstallIob(i);
	  PrintIob();
	}
    }
}

/*BFUNC

MakeImage() makes an image structure and installs it as the current
image.

EFUNC*/

void MakeImage()
{
  BEGIN("MakeImage");

  if (!(CImage = MakeStructure(IMAGE)))
    {
      WHEREAMI();
      printf("Cannot make an image structure.\n");
    }
  CImage->StreamFileName = NULL;
  CImage->p64Mode = 0;
  CImage->Height = 0;
  CImage->Width = 0;
}

/*BFUNC

MakeFrame() makes a frame structure and installs it as the current
frame structure.

EFUNC*/

void MakeFrame()
{
  BEGIN("MakeFrame");
  int i;

  if (!(CFrame = MakeStructure(FRAME)))
    {
      WHEREAMI();
      printf("Cannot make an frame structure.\n");
    }
  CFrame->NumberComponents = 3;
  for(i=0;i<MAXIMUM_SOURCES;i++)
    {
      CFrame->Height[i] = 0;
      CFrame->Width[i] = 0;
      CFrame->hf[i] = 1;
      CFrame->vf[i] = 1;
      *CFrame->ComponentFileName[i]='\0';
      *CFrame->ComponentFilePrefix[i]='\0';
      *CFrame->ComponentFileSuffix[i]='\0';
    }
}

/*BFUNC

MakeFstore() makes and installs the frame stores for the motion
estimation and compensation.

EFUNC*/


void MakeFstore()
{
  int i;

  CFS = (FSTORE *) malloc(sizeof(FSTORE));
  CFS->NumberComponents = 0;
  for(i=0;i<MAXIMUM_SOURCES;i++)
    {
      CFS->fs[i] = NULL;
    }
  OFS = (FSTORE *) malloc(sizeof(FSTORE));
  OFS->NumberComponents = 0;
  for(i=0;i<MAXIMUM_SOURCES;i++)
    {
      OFS->fs[i] = NULL;
    }
}

/*BFUNC

MakeStat() makes the statistics structure to hold all of the current
statistics. (CStat and RStat).

EFUNC*/


void MakeStat()
{
  CStat = MakeStructure(STAT);
  RStat = MakeStructure(STAT);
}

/*BFUNC

MakeRate() makes some statistics and book-keeping structures for
advanced rate tracking through the frames.

EFUNC*/

void MakeRate()
{
  RCStore = (RATE *) calloc(NumberFrames,sizeof(RATE));
}

/*BFUNC

SetCCITT() sets the CImage and CFrame parameters for CCITT coding.

EFUNC*/

void SetCCITT()
{
  BEGIN("SetCCITT");
  int i;

  if (*CFrame->ComponentFilePrefix[0]=='\0')
    {
      WHEREAMI();
      printf("A file prefix should be specified.\n");
      exit(ERROR_BOUNDS);
    }
  for(i=0;i<3;i++)
    {
      if (*CFrame->ComponentFilePrefix[i]=='\0')
	{
	  strcpy(CFrame->ComponentFilePrefix[i],
		 CFrame->ComponentFilePrefix[0]);
	}
      if (*CFrame->ComponentFileSuffix[i]=='\0')
	{
	  strcpy(CFrame->ComponentFileSuffix[i],
		 DefaultSuffix[i]);
	}
    }
  CFS->NumberComponents = 3;
  OFS->NumberComponents = 3;
  CFrame->NumberComponents = 3;
  CFrame->hf[0] = 2;
  CFrame->vf[0] = 2;
  CFrame->hf[1] = 1;
  CFrame->vf[1] = 1;
  CFrame->hf[2] = 1;
  CFrame->vf[2] = 1;
  switch(ImageType)
    {
    case IT_NTSC:
      NumberGOB = 10;  /* Parameters for NTSC design */
      NumberMDU = 33;
      CImage->Width = 352;
      CImage->Height = 240;
      CFrame->Width[0] = 352;
      CFrame->Height[0] = 240;
      CFrame->Width[1] = 176;
      CFrame->Height[1] = 120;
      CFrame->Width[2] = 176;
      CFrame->Height[2] = 120;
      break;
    case IT_CIF:
      NumberGOB = 12;  /* Parameters for NTSC design */
      NumberMDU = 33;
      CImage->Width = 352;
      CImage->Height = 288;
      CFrame->Width[0] = 352;
      CFrame->Height[0] = 288;
      CFrame->Width[1] = 176;
      CFrame->Height[1] = 144;
      CFrame->Width[2] = 176;
      CFrame->Height[2] = 144;
      break;
    case IT_QCIF:
      NumberGOB = 3;  /* Parameters for NTSC design */
      NumberMDU = 33;
      CImage->Width = 176;
      CImage->Height = 144;
      CFrame->Width[0] = 176;
      CFrame->Height[0] = 144;
      CFrame->Width[1] = 88;
      CFrame->Height[1] = 72;
      CFrame->Width[2] = 88;
      CFrame->Height[2] = 72;
      break;
    default:
      WHEREAMI();
      printf("Unknown ImageType: %d\n",ImageType);
      exit(ERROR_BOUNDS);
      break;
    }

  LastIntra = (unsigned char **) calloc(NumberGOB,sizeof(unsigned char *));
  for(i=0;i<NumberGOB;i++)
    {
      /* Should be assigned to all zeroes */
      LastIntra[i] = (unsigned char *) calloc(NumberMDU,sizeof(unsigned char));
      memset(LastIntra[i],0,NumberMDU);  /* just in case */
    }
}

/*BFUNC

Help() prints out help information about the p64 program.

EFUNC*/

void Help()
{
  BEGIN("Help");

  printf("p64  [-d [-c]] [-NTSC] [-CIF] [-QCIF]\n");
  printf("     [-a StartNumber] [-b EndNumber]\n");
  printf("     [-f FrameRate] [-k FrameskipNumber] [-o] [-p]\n");
  printf("     [-i MCSearchLimit] [-q Quantization] [-v] [-y]\n");
  printf("     [-r Target Rate] [-x Target Filesize]\n");
  printf("     [-s StreamFile] [-z ComponentFileSuffix i]\n");
  printf("     ComponentFilePrefix1 [ComponentFilePrefix2 ComponentFilePrefix3]\n");
  printf("-NTSC (352x240)  -CIF (352x288) -QCIF (176x144) base filesizes.\n");
  printf("-a is the start filename index. [inclusive] Defaults to 0.\n");
  printf("-b is the end filename index. [inclusive] Defaults to 0.\n");
  printf("-c forces cif large-frame decoding (can be used with all input modes).\n");
  printf("-d enables the decoder\n");
  printf("-f gives the frame rate (default 30).\n");
  printf("-i gives the MC search area: between 1 and 31 (default 15).\n");
  printf("-k is the frame skip index. Frames/s = FrameRate/FrameSkip.\n");
  printf("-o enables the interpreter.\n");
  printf("-p enables parity checking (disabled for 1992 CCITT specs).\n");
  printf("-q denotes Quantization, between 1 and 31.\n");
  printf("-r gives the target rate in bps.\n");
  printf("-s denotes StreamFile, which defaults to ComponentFilePrefix1.p64\n");
  printf("-v denotes verbose mode, showing quantization changes.\n");
  printf("-x gives the target filesize in kilobits. (overrides -r option.)\n");
  printf("-y enables Reference DCT.\n");
  printf("-z gives the ComponentFileSuffixes (repeatable).\n");
}

/*BFUNC

MakeFileNames() creates the filenames for the component files
from the appropriate prefix and suffix commands.

EFUNC*/

void MakeFileNames()
{
  BEGIN("MakeFileNames");
  int i;

  for(i=0;i<3;i++)
    {
      sprintf(CFrame->ComponentFileName[i],"%s%d%s",
	      CFrame->ComponentFilePrefix[i],
	      CurrentFrame,
	      CFrame->ComponentFileSuffix[i]);
    }
}

/*BFUNC

VerifyFiles() checks to see if the component files are present and
of the correct length.

EFUNC*/

void VerifyFiles()
{
  BEGIN("VerifyFiles");
  int i,FileSize;
  FILE *test;  
  
  for(i=0;i<CFrame->NumberComponents;i++)
    {
      if ((test = fopen(CFrame->ComponentFileName[i],"r")) == NULL)
	{
	  WHEREAMI();
	  printf("Cannot Open FileName %s\n",
		 CFrame->ComponentFileName[i]);
	  exit(ERROR_BOUNDS);
	}
      fseek(test,0,2);
      FileSize = ftell(test);
      rewind(test);
      if (CFrame->Height[i] == 0)
	{
	  if (CFrame->Width[i] == 0)
	    {
	      WHEREAMI();
	      printf("Bad File Specification for file %s\n",
		     CFrame->ComponentFileName[i]);
	    }
	  else
	    {
	      CFrame->Height[i] = FileSize / CFrame->Width[i];
	      printf("Autosizing Height to %d\n",
		      CFrame->Height[i]);
	    }
	}
      if (FileSize != CFrame->Width[i] * CFrame->Height[i]) 
	{
	  WHEREAMI();
	  printf("Inaccurate File Sizes: Estimated %d: %s: %d \n",
		 CFrame->Width[i] * CFrame->Height[i],
		 CFrame->ComponentFileName[i],
		 FileSize);
	  exit(ERROR_BOUNDS);
	}
      fclose(test);
    }
}

/*END*/
