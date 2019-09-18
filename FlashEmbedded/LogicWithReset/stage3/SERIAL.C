/***************************************************************************
*.b
*  Copyright (c) 2000 DaimlerChrysler Rail Systems (North America) Inc
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : SERIAL.C
*  Subsystem  : Third stage boot loader
*  Procedures : io_getbyte
*               io_putbyte
*
*  Abstract   :
*  Compiler   :
*
*  EPROM Drawing:
*.b
*****************************************************************************
* History:
*  01 May 2000 D.Smail
*    Created
* Revised:
**************************************************************************/

#include <reg167.h>

#include "cpu_dep.h"

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: io_getbyte
*
*  ABSTRACT:
*    Returns the byte read from the serial port
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        S0RIR
*        SORBUF
*
*     Procedure Parameters:
*        None
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*        byte received in the serial port
*
*  FUNCTIONAL DESCRIPTION:
*     This function waits for a character to be received in the serial port.
*  Once a character is received, the character is returned to the calling
*  function.
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
UINT_8 io_getbyte (void)
{
    while (! S0RIR);          /* wait until byte received */
    S0RIR = 0;                /* ready to receive next byte */
    return (S0RBUF);          /* return byte from receive */
}

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: io_putbyte
*
*  ABSTRACT:
*    Places a byte in the serial port to be sent to PC
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        S0TIR
*        SOTBUF
*
*     Procedure Parameters:
*        byte               UINT_8            byte to be transmitted
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*        None
*
*  FUNCTIONAL DESCRIPTION:
*     This function writes the character passed to this function to the
*  transmit register of the serial port. The function returns after the
*  transmit buffer is empty to ensure a subsequent call to this function
*  does not overwrite the buffer.
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
void io_putbyte (UINT_8 byte)
{
    S0TIR = 0;                /* set ready to transmit */
    S0TBUF = byte;            /* put byte in transmit data buffer */
    while (! S0TIR);          /* wait until transmit buffer empty */
}


