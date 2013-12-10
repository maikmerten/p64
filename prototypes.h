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
prototypes.h

This file contains the functional prototypes for type checking.

************************************************************
*/


/* p64.c */

extern void p64EncodeSequence();
extern void p64DecodeSequence();
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

/* codec.c */

extern void EncodeAC();
extern void CBPEncodeAC();
extern void DecodeAC();
extern void CBPDecodeAC();
extern int DecodeDC();
extern void EncodeDC();

/* huffman.c */

extern void inithuff();
extern int Encode();
extern int Decode();
extern void PrintDhuff();
extern void PrintEhuff();
extern void PrintTable();

/* io.c */

extern void MakeIob();
extern void GlobalMC();
extern void SubOverlay();
extern void AddOverlay();
extern void SubCompensate();
extern void AddCompensate();
extern void HalfSubCompensate();
extern void HalfAddCompensate();
extern void LoadFilterMatrix();
extern void SubFCompensate();
extern void AddFCompensate();
extern void HalfSubFCompensate();
extern void HalfAddFCompensate();
extern void ClearIob();
extern void CopyIob2FS();
extern void ClearFS();
extern int ParityFS();
extern void InitFS();
extern void ReadIob();
extern void InstallIob();
extern void InstallFS();
extern void WriteIob();
extern void MoveTo();
extern int Bpos();
extern void ReadBlock();
extern void WriteBlock();
extern void PrintIob();

/* chendct.c */

extern void ChenDct();
extern void ChenIDct();

/* lexer.c */

extern void initparser();
extern void parser();

/* marker.c */

extern void WritePictureHeader();
extern void ReadPictureHeader();
extern void WriteGOBHeader();
extern int ReadHeaderHeader();
extern void ReadHeaderTrailer();
extern void ReadGOBHeader();
extern void WriteMBHeader();
extern int ReadMBHeader();

/* me.c */

extern void initmc();
extern void FastBME();
extern void BruteMotionEstimation();
extern void BMC();
extern MEM *MotionCompensation();

/* mem.c */

extern BLOCK MakeBlock();
extern void CopyBlock();
extern void CopyMem();
extern ClearMem();
extern int ParityMem();
extern void SetPointerBlock();
extern MEM *MakeMem();
extern void FreeMem();
extern MEM *LoadMem();
extern MEM *SaveMem();

/* stat.c */

extern void Statistics();

/* stream.c */

extern void mropen();
extern void mrclose();
extern void mwopen();
extern void mwclose();
extern int mgetb();
extern void mputv();
extern int mgetv();
extern long mwtell();
extern long mrtell();
extern void mwseek();
extern void mrseek();
extern int seof();

/* transform.c */

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
