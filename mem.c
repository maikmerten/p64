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
mem.c

This file contains the basic memory manipulation structures.

************************************************************
*/

/*LABEL mem.c */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mem.h"

/*PUBLIC*/

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

/*PRIVATE*/

#define DATALENGTH 100000 /*Maximum internal buffer*/

/*START*/

/*BFUNC

MakeBlock() returns a block structure created. This is an actual
structure, not just a pointer.

EFUNC*/

BLOCK MakeBlock()
{
  BEGIN("MakeBlock");
  BLOCK temp;

  if (!(temp=(BLOCK)calloc(16,sizeof(unsigned char *))))
    {
      WHEREAMI();
      printf("Cannot allocate Block structure.\n");
      exit(ERROR_MEMORY);
    }
  return(temp);
}

/*BFUNC

CopyBlock() copies the block contents of one block to another (b2 to
b1 as in typical assignment). Note that a block is considered 16x16 in
this case.

EFUNC*/

void CopyBlock(b1,b2)
     BLOCK b1;
     BLOCK b2;
{
  BEGIN("CopyBlock");
  int i;

  for(i=0;i<16;i++) {memcpy(b2[i],b1[i],16);}
}

/*BFUNC

CopyMem() copies the entire contents of m2 to m1. 

EFUNC*/

void CopyMem(m1,m2)
     MEM *m1;
     MEM *m2;
{
  BEGIN("CopyMem");

  memcpy(m2->data,m1->data,m1->width*m1->height);
}

/*BFUNC

ClearMem() clears a memory structure by setting it to all zeroes.

EFUNC*/

ClearMem(m1)
     MEM *m1;
{
  BEGIN("ClearMem");

  memset(m1->data,0,m1->width*m1->height);
}

/*BFUNC

ParityMem() returns the bitwise parity of the entire memory structure.
This is calculated by XORing all of its component bytes to form an end
component.

EFUNC*/

int ParityMem(mem)
     MEM *mem;
{
  BEGIN("ParityMem");
  int parity;
  register unsigned char *ptr,*top;

  top = mem->data + (mem->width*mem->height);
  for(parity=0,ptr=mem->data;ptr<top;)
    {
      parity ^= *(ptr++);
    }
  return(parity);
}

/*BFUNC

SetPointerBlock() sets the block to represent the given pixel position
within the memory structure.

EFUNC*/

void SetPointerBlock(px,py,mem,block)
     int px;
     int py;
     MEM *mem;
     BLOCK block;
{
  BEGIN("SetPointerBlock");
  int i;

  if ((mem->width < 16) || (mem->height < 16))
    {
      WHEREAMI();
      printf("Attempt to retrieve pointers from too small an mem.\n");
      exit(ERROR_BOUNDS);
    }
  if (px+16 > mem->width) {px = mem->width-16;}
  if (py+16 > mem->height) {py = mem->height-16;}
  for(i=0;i<16;i++) {block[i] = mem->data + px + ((py+i)*mem->width);}
}

/*BFUNC

MakeMem() creates a memory structure out of a given width and height.

EFUNC*/

MEM *MakeMem(width,height)
     int width;
     int height;
{
  BEGIN("MakeMem");
  MEM *temp;

  if (!(temp=MakeStructure(MEM)))
    {
      WHEREAMI();
      printf("Cannot create Memory structure.\n");
      exit(ERROR_MEMORY);
    }
  temp->len = width*height;
  temp->width = width;
  temp->height = height;
  if (!(temp->data=(unsigned char *)calloc(width*height,
					   sizeof(unsigned char))))
    {
      WHEREAMI();
      printf("Cannot allocate data storage for Memory structure.\n");
      exit(ERROR_MEMORY);
    }
  return(temp);
}

/*BFUNC

FreeMem() frees a memory structure.

EFUNC*/

void FreeMem(mem)
     MEM *mem;
{
  BEGIN("FreeMem");

  free(mem->data);
  free(mem);
}


/*BFUNC

LoadMem(width,height,)
     ) loads an Mem with a designated width; loads an Mem with a designated width, height, and
filename into a designated memory structure. If the memory structure
is NULL, one is created for it.

EFUNC*/

MEM *LoadMem(filename,width,height,omem)
     char *filename;
     int width;
     int height;
     MEM *omem;
{
  BEGIN("LoadMem");
  int length;
  MEM *temp;
  FILE *inp;

  if ((inp = fopen(filename,"r")) == NULL)
    {
      WHEREAMI();
      printf("Cannot open filename %s.\n",filename);
      exit(ERROR_BOUNDS);
    }
  fseek(inp,0,2);
  length = ftell(inp);
  rewind(inp);
  if ((width*height) != length)
    {
      WHEREAMI();
      printf("Bad Height and Width\n");
      exit(ERROR_BOUNDS);
    }
  if (omem) {temp=omem;}
  else {temp = MakeMem(width,height);}
  fread(temp->data,sizeof(unsigned char),temp->width*temp->height,inp);
  fclose(inp);
  return(temp);
}


/*BFUNC

SaveMem() saves the designated memory structure to the appropriate
filename.

EFUNC*/

MEM *SaveMem(filename,mem)
     char *filename;
     MEM *mem;
{
  BEGIN("SaveMem");
  FILE *out;

  if ((out = fopen(filename,"w")) == NULL)
    {
      WHEREAMI();
      printf("Cannot open filename %s.\n",filename);
      exit(ERROR_BOUNDS);
    }
  fwrite(mem->data,sizeof(unsigned char),mem->width*mem->height,out);
  fclose(out);
  return(mem);
}


/*END*/
