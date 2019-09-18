/***************************************************************************
*.b
*  Copyright (c) 2000 DaimlerChrysler Rail Systems (North America) Inc
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : BLKDATA.C
*  Subsystem  : Third stage boot loader
*  Procedures : Download_block_data
*               Get_total_bytes
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
#include "include.h"


/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Download_block_data
*
*  ABSTRACT:
*    Receive and store a block of data from the PC.
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        START_OF_DOWNLOAD_SRAM
*        MSB_OF_MSW_FLASH_ADDRESS
*        LSB_OF_MSW_FLASH_ADDRESS
*        MSB_OF_LSW_FLASH_ADDRESS
*        LSB_OF_LSW_FLASH_ADDRESS
*        MSB_OF_BLOCK_SIZE
*        LSB_OF_BLOCK_SIZE
*
*     Procedure Parameters:
*        globs        struct interface_data_t *    shared variables
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
*    This function is responsible for getting blocks of data from the PC
*  and storing the data sequentially in SRAM. A block of data consists of
*  a series of bytes sent from the PC (not greater than 64K in length).
*  The format of the block is as follows:
*
*         32 bit FLASH Address (where to start programming received data in
*                               the block)
*         16 bit block size    (number of data bytes in the block)
*         8  bit data          (sequential array of binary bytes to be
*                               programmed in FLASH starting at the
*                               FLASH address)
*
*         SRAM LOCATION                     Byte Meaning
*  -----------------------------------------------------------------
*     START_OF_DOWNLOAD_SRAM + 0          MSByte of MSW FLASH address
*     START_OF_DOWNLOAD_SRAM + 1          LSByte of MSW FLASH address
*     START_OF_DOWNLOAD_SRAM + 2          MSByte of LSW FLASH address
*     START_OF_DOWNLOAD_SRAM + 3          LSByte of LSW FLASH address
*     START_OF_DOWNLOAD_SRAM + 4          MSByte of Block size
*     START_OF_DOWNLOAD_SRAM + 5          LSByte of Block size
*     START_OF_DOWNLOAD_SRAM + 5 + x      All bytes to be programmed in FLASH
*
*  The block size is constructed after the sixth byte is received and is
*  used to determine when all bytes in the block have been received. The block
*  size does not include the 6 header bytes described above. After all the bytes
*  in the block have been received, the loop terminates and the function exits.
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
void Download_block_data (struct interface_data_t *globs)
{

    UINT_8  data;     /* byte received via serial port read */
    UINT_8  hi_byte;  /* stores the MSByte of the block byte count */
    UINT_8  lo_byte;  /* stores the LSByte of the block byte count */
    UINT_8  header_state;   /* Used in construction of byte count */
    UINT_8  huge *sram_ptr; /* Used to store downloaded block data in RAM */
    UINT_16 byte_count; /* After variable constructed, decremented to 0 */

    /* Initialize variables */
    header_state = MSB_OF_MSW_FLASH_ADDRESS;
    sram_ptr = (UINT_8 huge *)START_OF_DOWNLOAD_SRAM;

    /* Init so that code remains in loop until byte_count constructed */
    byte_count = 1;

    while (byte_count != 0)
    {

        data = (UINT_8)io_getbyte();
        /* Store the address and the number of bytes in the current block */
        switch (header_state)
        {
            /* First 4 bytes are the FLASH address where data is to be programmed */
            case MSB_OF_MSW_FLASH_ADDRESS:
            case LSB_OF_MSW_FLASH_ADDRESS:
            case MSB_OF_LSW_FLASH_ADDRESS:
            case LSB_OF_LSW_FLASH_ADDRESS:
                header_state++;
                break;
            /* Next 2 bytes are the number of bytes in the data block */
            case MSB_OF_BLOCK_SIZE:
                hi_byte = data;
                header_state++;
                break;
            case LSB_OF_BLOCK_SIZE:
                lo_byte = data;
                byte_count = (hi_byte << 8) | lo_byte;
                header_state++;
                break;
            default:
                byte_count--;
                break;
        }
        /* Store the received data */
        *sram_ptr++ = data;

        /* Adjust total byte count variable */
        globs->total_bytes--;
    }

}

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Get_total_bytes
*
*  ABSTRACT:
*     Receive from the PC the total number of bytes to expect
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        None
*
*     Procedure Parameters:
*        globs        struct interface_data_t *    shared variables
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
*     This function is responsible for receiving 4 bytes from the PC that
*  represent the total number of bytes to be downloaded (value includes these
*  4 bytes). This number does not include any command protocol bytes.
*
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
void Get_total_bytes (struct interface_data_t *globs)
{
    UINT_8 msw_hi_byte; /* stores the MSByte of the MSWord of a 32 bit var */
    UINT_8 msw_lo_byte; /* stores the LSByte of the MSWord of a 32 bit var */
    UINT_8 lsw_hi_byte; /* stores the MSByte of the LSWord of a 32 bit var */
    UINT_8 lsw_lo_byte; /* stores the LSByte of the LSWord of a 32 bit var */

    /* Initial all total byte count variables */
    globs->total_bytes          = 0;

    /* Get total byte count from PC */
    msw_hi_byte = (UINT_8)io_getbyte();
    msw_lo_byte = (UINT_8)io_getbyte();
    lsw_hi_byte = (UINT_8)io_getbyte();
    lsw_lo_byte = (UINT_8)io_getbyte();


    /* Construct the variable  */
    globs->total_bytes  = (UINT_32)msw_hi_byte << 24;
    globs->total_bytes |= (UINT_32)msw_lo_byte << 16;
    globs->total_bytes |= (UINT_32)lsw_hi_byte << 8;
    globs->total_bytes |= (UINT_32)lsw_lo_byte;

    /* Adjust the total byte count variable to account for total_bytes
    variable */
    globs->total_bytes -= 4;

}


