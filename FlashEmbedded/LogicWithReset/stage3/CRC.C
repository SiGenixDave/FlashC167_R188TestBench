/***************************************************************************
*.b
*  Copyright (c) 2000 DaimlerChrysler Rail Systems (North America) Inc
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : CRC.C
*  Subsystem  : Third stage boot loader
*  Procedures : Calc_crc
*               Get_crc_from_flash
*               Binary_byte_to_ASCII
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
*  PROCEDURE NAME: Calc_crc
*
*  ABSTRACT:
*     Receive CRC information and confirm FLASH programming with CRC algorithm
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        TRUE
*        FALSE
*        CRC_PASS_STATE
*        CRC_FAIL_STATE
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
*        state        State_t              result of the CRC calculation
*
*  FUNCTIONAL DESCRIPTION:
*     This function is responsible for confirming the FLASH was programmed
*  successfully. First, the function must receive information from the PC
*  regarding the FLASH programming. This information consists of the following:
*
*        INFO                             NUMBER OF BYTES
*     ----------------------------------------------------
*     CRC width                                 1
*     CRC polynomial                            4
*     FLASH start address                       4
*     FLASH end address                         4
*     FLASH address where CRC is stored         4
*
*     Then the correct CRC lookup table is created (based on the CRC width
*  and the polynomial). The use of this table significantly speeds the CRC
*  algorithm.
*     Finally, the CRC algorithm is performed. The result of algorithm should
*  be 0x00 in all cases. If not, an error is flagged. Also, during the course
*  of running the calculation, the checksum should become non-zero at some
*  point (ensures non-zero values are being read from FLASH). If the checksum
*  never becomes non-zero an error is flagged.
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
******************************************************************************/

UINT_8  crc_8;  /* running checksum for CRC-8  calculation */
UINT_16 crc_16; /* running checksum for CRC-16 calculation */
UINT_32 crc_32; /* running checksum for CRC-32 calculation */

UINT_8 Calc_crc (struct interface_data_t *globs)
{


    UINT_8  crc_error;    /* TRUE if error in calculation */
    UINT_8  crc_not_zero; /* becomes TRUE if running checksum becomes nonzero */

    UINT_8  crc_table_8[256]; /* CRC-8  lookup table */
    UINT_16 crc_table_16[256];/* CRC-16 lookup table */
    UINT_32 crc_table_32[256];/* CRC-32 lookup table */

    UINT_32 *ptr_32;  /* used to construct 32 bit words */

    UINT_8 huge *flash_ptr; /* used to access programmed FLASH bytes */

    UINT_8  msw_hi_byte;  /* stores the MSByte of the MSWord of a 32 bit var */
    UINT_8  msw_lo_byte;  /* stores the LSByte of the MSWord of a 32 bit var */
    UINT_8  lsw_hi_byte;  /* stores the MSByte of the LSWord of a 32 bit var */
    UINT_8  lsw_lo_byte;  /* stores the LSByte of the LSWord of a 32 bit var */
    UINT_32 crc_poly;     /* constructed from received PC data (CRC polynomial) */
    UINT_32 flash_start_address;  /* constructed from received PC data (beginning
								  of FLASH) */
    UINT_32 flash_end_address;  /* constructed from received PC data (end of
								FLASH) */

    UINT_8  flash_data; /* data read from programmed FLASH */
    UINT_16 tab_index;  /* index into the CRC lookup table */
    UINT_32 i;  /* loop index parameter; used to construct CRC lookup table */
    UINT_32 x;  /* loop index parameter */

    UINT_8  state;  /* return value, state of the CRC calculation (PASS,FAIL) */

    /* Init vars */
    crc_8 = 0;
    crc_16 = 0;
    crc_32 = 0;
    crc_error = FALSE;
    crc_not_zero = FALSE;

    /* Get the CRC Width (8, 16, or 32) */
    globs->crc.width = (UINT_8)io_getbyte();

    /* Get the CRC Polynomial, FLASH start address, FLASH end address,
    and the address where the CRC is stored */
    for (i = 0; i < 4; i++)
    {
        msw_hi_byte  = (UINT_8)io_getbyte();
        msw_lo_byte  = (UINT_8)io_getbyte();
        lsw_hi_byte  = (UINT_8)io_getbyte();
        lsw_lo_byte  = (UINT_8)io_getbyte();

        /* Determine which 32 bit var to construct */
        switch (i)
        {
            case 0:
                ptr_32 = &crc_poly;
                break;

            case 1:
                ptr_32 = &flash_start_address;
                break;

            case 2:
                ptr_32 = &flash_end_address;
                break;

            case 3:
                ptr_32 = & (globs->crc.address);
                break;
        }

        /* Construct the 32 bit var */
        *ptr_32    = (UINT_32)msw_hi_byte << 24;
        *ptr_32   |= (UINT_32)msw_lo_byte << 16;
        *ptr_32   |= (UINT_32)lsw_hi_byte << 8;
        *ptr_32   |= (UINT_32)lsw_lo_byte;
    }


    /* Create CRC lookup table based on the CRC width and polynomial*/
    switch (globs->crc.width)
    {
        /* Create CRC-8 lookup table */
        case 8:
            for (i = 0; i < 256; i++)
            {
                crc_table_8[i] = i;
                for (x = 0; x < 8; x++)
                {
                    if (crc_table_8[i] & 0x80)
                    {
                        crc_table_8[i] = (crc_table_8[i] << 1) ^ crc_poly;
                    }
                    else
                    {
                        crc_table_8[i] <<= 1;
                    }
                }
            }
            break;

        /* Create CRC-16 lookup table */
        case 16:
            for (i = 0; i < 256; i++)
            {
                crc_table_16[i] = i;
                for (x = 0; x < 16; x++)
                {
                    if (crc_table_16[i] & 0x8000)
                    {
                        crc_table_16[i] = (crc_table_16[i] << 1) ^ crc_poly;
                    }
                    else
                    {
                        crc_table_16[i] <<= 1;
                    }
                }
            }
            break;

        /* Create CRC-32 lookup table */
        case 32:
            for (i = 0; i < 256; i++)
            {
                crc_table_32[i] = i;
                for (x = 0; x < 32; x++)
                {
                    if (crc_table_32[i] & 0x80000000)
                    {
                        crc_table_32[i] = (crc_table_32[i] << 1) ^ crc_poly;
                    }
                    else
                    {
                        /* Equation needs to be this ..... */
                        crc_table_32[i] = crc_table_32[i] << 1;
                        /* .... and not this
                        crc_table_32[i] << 1;
                        because a library function is invoked which does not run properly
                        for an unknown reason */
                    }
                }
            }
            break;

        /* Invalid width; inform PC of error */
        default:
            crc_error = TRUE;
            break;
    }

    /* Perform the algorithm until the end address is reached */
    flash_ptr  = (UINT_8 huge *)flash_start_address;


    while (flash_ptr <= (UINT_8 huge *)flash_end_address)
    {
        flash_data = *flash_ptr;
        /* CRC is not stored in FLASH, but entered on the command line. Therefore
        adjust the end address by "crc_width" bytes so the CRC calculation does
        not include the final bytes */
        if (globs->crc.address == 0)
        {
            if (flash_ptr > (UINT_8 huge *) (flash_end_address - (globs->crc.width >> 3)))
            {
                flash_data = 0;
            }
        }
        switch (globs->crc.width)
        {
            /* Perform CRC-8 calculation */
            case 8:
                tab_index = crc_8 >> 0;
                crc_8 = (crc_8 << 8) | flash_data;
                crc_8 ^= crc_table_8[tab_index];
                /* Verify running checksum becomes non-zero at some point */
                if (crc_8 != 0)
                {
                    crc_not_zero = TRUE;
                }
                break;

            /* Perform CRC-16 calculation */
            case 16:
                tab_index = crc_16 >> 8;
                crc_16 = (crc_16 << 8) | flash_data;
                crc_16 ^= crc_table_16[tab_index];
                /* Verify running checksum becomes non-zero at some point */
                if (crc_16 != 0)
                {
                    crc_not_zero = TRUE;
                }
                break;

            /* Perform CRC-32 calculation */
            case 32:
                tab_index = crc_32 >> 24;
                crc_32 = (crc_32 << 8) | flash_data;
                crc_32 ^= crc_table_32[tab_index];
                /* Verify running checksum becomes non-zero at some point */
                if (crc_32 != 0)
                {
                    crc_not_zero = TRUE;
                }
                break;

            default:
                break;
        }
        /* Increment FLASH address to next byte */
        flash_ptr++;
    }

    /* Determine if any errors occurred (final CRC != 0 or running CRC never became
    non-zero */
    if (globs->crc.address != 0)
    {
        switch (globs->crc.width)
        {
            case 8:
                if ((crc_8 != 0) || (crc_not_zero == FALSE))
                {
                    crc_error = TRUE;
                }
                break;
            case 16:
                if ((crc_16 != 0) || (crc_not_zero == FALSE))
                {
                    crc_error = TRUE;
                }
                break;
            case 32:
                if ((crc_32 != 0) || (crc_not_zero == FALSE))
                {
                    crc_error = TRUE;
                }
                break;

            default:
                break;
        }

        /* Return the result of the CRC calculation */
        if (crc_error == TRUE)
        {
            state = CRC_FAIL_STATE;
        }
        else
        {
            state = CRC_PASS_STATE;
        }
    }

    /* Always return PASS when user enters CRC on command line */
    else
    {
        state = CRC_PASS_STATE;
    }

    return state;

}


UINT_8 ResetPCB (struct interface_data_t *globs)
{
    globs->reset = 0xDEADBEEF;
    return RESET_COMPLETE;
}


/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Get_crc_from_flash
*
*  ABSTRACT:
*     Extract the stored CRC from FLASH
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
*     This function extracts the stored CRC from FLASH and outputs the ASCII
*  equivalent to the PC. The number of bytes sent to the PC depends on the
*  width of the CRC. For CRC-8, CRC-16, and CRC-32, one, two and four bytes
*  is/are respectively read from FLASH.
*     The function starts reading FLASH from the CRC address received from
*  the PC and the address is incremented (for CRC-16 and CRC-32) until all
*  of the CRC is read.
*     The extracted data is converted from binary to ASCII and sent to the
*  PC.
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
void Get_crc_from_flash (struct interface_data_t *globs)
{

    UINT_8  i;  /* loop index */

    UINT_8  huge *crc_data_ptr; /* access stored CRC in FLASH */
    UINT_8  crc_byte;           /* byte retrieved from FLASH */
    UINT_8  crc_hi_nibble;      /* ASCII MSNibble of byte; sent to PC */
    UINT_8  crc_lo_nibble;      /* ASCII MSNibble of byte; sent to PC */

    if (globs->crc.address != 0)
    {
        /* Get address in FLASH where CRC is stored */
        crc_data_ptr = (UINT_8 huge *)globs->crc.address;
    }
    else
    {
        switch (globs->crc.width)
        {
            case 8:
                crc_data_ptr = (UINT_8 huge *) (&crc_8);
                break;

            case 16:
                crc_data_ptr = (UINT_8 huge *) (&crc_16) + 1;
                break;

            case 32:
                crc_data_ptr = (UINT_8 huge *) (&crc_32) + 3;
                break;
        }
    }

    /* Number of bytes to access depends on the CRC width. The CRC width
    is divided by 8 (>> 3) to determine the number of bytes to read. */
    for (i = 0; i < (globs->crc.width >> 3); i++)
    {
        crc_byte = *crc_data_ptr;
        crc_hi_nibble = (crc_byte >> 4) & 0xf;
        crc_lo_nibble = crc_byte & 0xf;

        /* Convert to ASCII */
        crc_hi_nibble = Binary_byte_to_ASCII (crc_hi_nibble);
        crc_lo_nibble = Binary_byte_to_ASCII (crc_lo_nibble);

        /* Send to PC */
        io_putbyte (crc_hi_nibble);
        io_putbyte (crc_lo_nibble);

        if (globs->crc.address != 0)
        {
            /* Point at next byte in FLASH */
            crc_data_ptr++;
        }
        else
        {
            /* Point at next byte in FLASH */
            crc_data_ptr--;
        }
    }

}

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Binary_byte_to_ASCII
*
*  ABSTRACT:
*     Converts a binary byte to ASCII
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
*        binary_byte            UINT_8    binary byte to be converted to ASCII
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*        binary_byte            UINT_8    ASCII conversion of binary byte
*
*  FUNCTIONAL DESCRIPTION:
*      This function is used to convert a binary byte to its ASCII equivalent.
*  The value passed to the function is confirmed to be between 0 and 15 (0x00
*  and 0x0F ). If the value is not within this range, the NULL "0" char is
*  returned.
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
UINT_8 Binary_byte_to_ASCII (UINT_8 binary_byte)
{
    UINT_8 ascii_char;    /* ASCII equivalent of binary byte */

    if (binary_byte <= 9)
    {
        ascii_char = binary_byte + '0';
    }
    else if (binary_byte <= 15)
    {
        /* Bias the value by -10 */
        ascii_char = binary_byte - 10 + 'A';
    }
    else
    {
        ascii_char = 0;
    }

    return ascii_char;

}
