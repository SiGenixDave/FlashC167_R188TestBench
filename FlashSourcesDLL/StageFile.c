/***************************************************************************
*.b
*  Copyright (c) 2000-2013 Bombardier Transportation
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : StageFile.c
*  Subsystem  : PC (MS-DOS)
*  Procedures :
*
*  Abstract   : Used to interface between pre .NET code and .NET code
*  Compiler   :
*
*  EPROM Drawing:
*.b
*****************************************************************************
* History:
*  01 Nov 2014 D.Smail
*    Created
* Revised:
**************************************************************************/

#include "INCLUDE.H"

static char stage1[100];
static char stage2[500];
static char stage3[20000];

__declspec (dllexport) void __stdcall CopyStage1HexData (char* aString, long int aSize)
{
    strcpy (stage1, aString);
}
__declspec (dllexport) void __stdcall CopyStage2HexData (char* aString, long int aSize)
{
    strcpy (stage2, aString);
}
__declspec (dllexport) void __stdcall CopyStage3HexData (char* aString, long int aSize)
{
    strcpy (stage3, aString);
}

char *GetStage1 (void)
{
    return stage1;
}

char *GetStage2 (void)
{
    return stage2;
}

char *GetStage3 (void)
{
    return stage3;
}
