/***************************************************************************
*.b
*  Copyright (c) 2000-2013 Bombardier Transportation
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : SerialInterface.c
*  Subsystem  : PC (MS-DOS)
*  Procedures :
*
*  Abstract   : Used to interface between Win32 (unmanaged) code and
*               managed .NET code
*  Compiler   :
*
*  EPROM Drawing:
*.b
*****************************************************************************
* History:
*  01 Apr 2000 D.Smail
*    Created
* Revised:
**************************************************************************/


typedef void (__stdcall *TxCharFnPtr) (unsigned char tx);
typedef int (__stdcall *RxCharFnPtr) (void);

static TxCharFnPtr txCharPtr;
static RxCharFnPtr rxCharPtr;

// Allow DLL to call .NET functions though a function pointer
__declspec (dllexport) void SetTxCharCallback (TxCharFnPtr func)
{
    txCharPtr = func;
}

// Allow DLL to call .NET functions though a function pointer
__declspec (dllexport) void SetRxCharCallback (RxCharFnPtr func)
{
    rxCharPtr = func;
}

// Wrap C function around a function pointer to .NET
void a_putc (unsigned char tx)
{
    txCharPtr (tx);
}

// Wrap C function around a function pointer to .NET
int a_getc (void)
{
    return rxCharPtr();
}
