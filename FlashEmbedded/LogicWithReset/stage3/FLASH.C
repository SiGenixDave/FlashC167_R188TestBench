/***************************************************************************
*.b
*  Copyright (c) 2000 DaimlerChrysler Rail Systems (North America) Inc
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : FLASH.C
*  Subsystem  : Third stage boot loader
*  Procedures : Erase_flash
*               Erase_flash_chip
*               Program_flash
*               Autoselect_flash
*               Erase_AMD29040
*               Erase_INTEL28F800
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
*  28 Jan 2002 D.Smail
**************************************************************************/
#include <reg167.h>

#include "cpu_dep.h"
#include "flash.h"
#include "include.h"

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Erase_flash
*
*  ABSTRACT:
*     Called to erase the Logic FLASH
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        FLASH_ERASE_SUCCESS
*        ERR_FLASH_ERASE
*        ERR_FLASH_CLEAR
*        FLASH_ERASE_ERROR
*
*     Procedure Parameters:
*        globs                  struct interface_data_t *
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*        state                  State_t
*
*  FUNCTIONAL DESCRIPTION:
*     Function is responsible for calling the ERASE function and return
*  the state (success/fail) of the erase operation.
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
State_t Erase_flash (struct interface_data_t *globs)
{

    State_t state;         /* returned value; erasing success (PASS,FAIL) */

    UINT_16 flash_status; /* variable determines if any errors occurred during
						  erase */

    /* Initialize state. Becomes FLASH_ERASE_ERROR if any sector erase fails */
    state = FLASH_ERASE_SUCCESS;

    /* Erase flash  */
    flash_status = Erase_flash_chip (globs);
    if ((flash_status == ERR_FLASH_ERASE) || (flash_status == ERR_FLASH_CLEAR))
    {
        state = FLASH_ERASE_ERROR;
    }
    return state;


#if _DEBUG
    // Use this code (or similar) to output info to debug port
    io_putbyte (((man_id >> 4) & 0xf) + 0x30);
    io_putbyte (((man_id >> 0) & 0xf) + 0x30);
    io_putbyte (((dev_code >> 12) & 0xf) + 0x30);
    io_putbyte (((dev_code >> 8) & 0xf) + 0x30);
    io_putbyte (((dev_code >> 4) & 0xf) + 0x30);
    io_putbyte (((dev_code >> 0) & 0xf) + 0x30);
#endif

}

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Erase_flash_chip
*
*  ABSTRACT:
*
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        ERR_FLASH_CLEAR
*        AMD_29F040_DEV_ID
*        INTEL_28F800T_DEV_ID
*
*     Procedure Parameters:
*        globs                  struct interface_data_t *
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*        ret_val                UINT_16
*
*  FUNCTIONAL DESCRIPTION:
*     This function calls the appropriate erase algorithm function based on the
*   the type of FLASH detected on the Logic. The return value is based on
*   whether the erase was successful or not.
*
.b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
*  16 Jul 2001 D.Smail
*     Changed to account for the Bottom boot Intel FLASH
******************************************************************************/
UINT_16 Erase_flash_chip (struct interface_data_t *globs)
{

    UINT_16 ret_val;      /* return value */

    ret_val = ERR_FLASH_CLEAR;

    /* disable interrupts */
    IEN = 0;
    _nop();

    /* AMD 29040 Flash Detected */
    if ((globs->device_type == AMD_29F040_DEV_ID)      ||
            (globs->device_type == SST_39SF040_DEV_ID)     ||
            (globs->device_type == ATMEL_49F8192A_DEV_ID)  ||
            (globs->device_type == ATMEL_49F8192AT_DEV_ID))
    {
        ret_val = Erase_AMD29040 (globs);
    }
    /* Intel 28F800 FLASH (Top boot) detected */
    else if (globs->device_type == INTEL_28F800T_DEV_ID ||
             globs->device_type == INTEL_28F800B_DEV_ID)
    {
        ret_val = Erase_INTEL28F800 (globs);
    }
    else if (globs->device_type == M29W800_DEV_ID)
    {
        ret_val = Erase_M29W800 (globs);
    }

    /* restore interrupts */
    //IEN = 1;
    _nop();

    return (ret_val);
}


/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Program_flash
*
*  ABSTRACT:
*     Programs a block of data into FLASH; depends on whether detection of
*   FLASH type is AMD29040 or Intel 28F800
*
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        START_OF_DOWNLOAD_SRAM
*        ERR_FLASH_CLEAR
*        AMD_29F040_DEV_ID
*        AMD_UNLOCK_CMD1
*        AMD_UNLOCK_CMD2
*        AMD_PROGRAM_CMD
*        INTEL_28F800T_DEV_ID
*        INTEL_CLEAR_REGISTER
*        INTEL_PROGRAM_CMD
*        ERR_FLASH_PROG
*        ERR_FLASH_NONE
*        AMD_READ_RESET
*        INTEL_READ_ARRAY
*        FLASH_PROGRAM_ERROR
*        FLASH_PROGRAM_SUCCESS
*
*     Procedure Parameters:
*        globs                  struct interface_data_t *
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*        state                  State_t
*
*  FUNCTIONAL DESCRIPTION:
*     This function is responsible for reading the information stored in SRAM (FLASH
*  starting address and block size) sent from the PC. It then calls the correct
*  programming algorithm (AMD 29F040 or Intel 28F800) to program a word at a time.
*  Error checking is performed to verify the FLASH was successfully programmed.
*  (See the data sheets for the appropriate parts.)
*
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
*  16 Jul 2001 D.Smail
*     Changed to account for the Bottom boot Intel FLASH
*  23 Oct 2001 D.Smail
*     Changed to Bottom boot Intel FLASH algorithm for efficiency
******************************************************************************/
State_t Program_flash (struct interface_data_t *globs)
{

    UINT_32 upper_address; /* stores the segment in 32 bit format so when combining
						   segment and offset, a 32 bit address can be created */

    UINT_16 segment;  /* stores the address segment */
    UINT_16 offset;   /* stores the address offset */

    UINT_16 flash_status; /* reflects the state of the FLASH programming algorithm */
    UINT_16 flash_data;   /* code data that is written to FLASH */
    UINT_16 status;       /* variable stores the FLASH status; errors detected
						  from this variable   */
    UINT_16 status1;       /* variable stores the FLASH status; errors detected
						   from this variable   */
    UINT_16 block_size;   /* determines the number of bytes in the block just
						  received */

    UINT_8  huge *sram_ptr;    /* Pointer in SRAM, extracts code */
    UINT_16 huge *flash_ptr;   /* Pointer in FLASH, code from SRAM written to this
							   location */

    UINT_16 count;

    State_t state;  /* Returned value; determines success or failure of
					programming to calling function */

    /* Set the pointer in SRAM to get info about starting FLASH address and size */
    sram_ptr = (UINT_8 huge *)START_OF_DOWNLOAD_SRAM;

    /* Get FLASH segment from SRAM */
    segment = (((UINT_16) * sram_ptr << 8) | * (sram_ptr + 1));
    sram_ptr += 2;

    /* Get offset from RAM */
    offset = (((UINT_16) * sram_ptr << 8) | * (sram_ptr + 1));

    /* Create start address in FLASH */
    upper_address = segment;
    flash_ptr = (UINT_16 huge *) ((upper_address << 16) + offset);

    sram_ptr += 2;
    /* Determine the number of bytes to program */
    block_size = 0;
    block_size = (((UINT_16) * sram_ptr << 8) | * (sram_ptr + 1));
    sram_ptr += 2;

    while (block_size > 0)
    {
        flash_status = ERR_FLASH_CLEAR;

        if (block_size == 1)
        {
            /* If block size is odd, then the last byte is the next byte
            that needs to be programmed.  This is assumed to be the byte
            at the even address. The byte at the odd address is set to
            0xff (blank). */
            flash_data = (0xff00 | *sram_ptr);
            block_size -= 1;
            sram_ptr   += 1;
        }
        else
        {
            /* Create the word to program */
            flash_data = (((UINT_16) * (sram_ptr + 1) << 8) | *sram_ptr);
            sram_ptr += 2;
            block_size -= 2;
        }

        /* Setup the program FLASH command for the detected FLASH part */
        if ((globs->device_type == AMD_29F040_DEV_ID)       ||
                (globs->device_type == SST_39SF040_DEV_ID)      ||
                (globs->device_type == ATMEL_49F8192A_DEV_ID)   ||
                (globs->device_type == ATMEL_49F8192AT_DEV_ID))
        {
            /* start of inline flash byte program */
            * (globs->amd_addr_1) = AMD_UNLOCK_CMD1;
            * (globs->amd_addr_2) = AMD_UNLOCK_CMD2;
            * (globs->amd_addr_3) = AMD_PROGRAM_CMD;
        }
        else if (globs->device_type == INTEL_28F800T_DEV_ID ||
                 globs->device_type == INTEL_28F800B_DEV_ID)
        {
            *flash_ptr = INTEL_CLEAR_REGISTER;
            *flash_ptr = INTEL_PROGRAM_CMD;
        }
        else if (globs->device_type == M29W800_DEV_ID)
        {
            UINT_16 huge *m29w800_ptr;

            m29w800_ptr = (UINT_16 huge *)0x100AAA;
            *m29w800_ptr = 0xAA;

            m29w800_ptr = (UINT_16 huge *)0x100554;
            *m29w800_ptr = 0x55;

            m29w800_ptr = (UINT_16 huge *)0x100AAA;
            *m29w800_ptr = 0xA0;

        }

        /* Write the data to FLASH */
        *flash_ptr = flash_data;

        count = 0;

        /* Wait for a success or failure */
        do
        {
            status = *flash_ptr;      /* Read back status from flash */

            /* AMD & Atmel Programming Algorithm */
            if ((globs->device_type == AMD_29F040_DEV_ID) || (globs->device_type == SST_39SF040_DEV_ID))
            {
                /* lower bit 7 of flash_data -> ready */
                if ((status & 0x0080) != (flash_data & 0x0080))
                {
                    /* bit 5 == 1 -> possible ERROR */
                    if ((status & 0x20) == 0x20)
                    {
                        status1 = *flash_ptr;
                        /* check bit 7 once more */
                        /* if bit 7 of status != bit 7 of flash_data -> ERROR */
                        if ((status1 & 0x80) != (flash_data & 0x80))
                        {
                            flash_status = ERR_FLASH_PROG;
                        }
                    }
                }

                if ((status & 0x8000) != (flash_data & 0x8000))
                {
                    /* bit 5 == 1 -> possible ERROR */
                    if ((status & 0x2000) == 0x2000)
                    {
                        status1 = *flash_ptr;
                        /* check bit 7 once more */
                        /* if bit 7 of status != bit 7 of flash_data -> ERROR */
                        if ((status1 & 0x8000) != (flash_data & 0x8000))
                        {
                            flash_status = ERR_FLASH_PROG;
                        }
                    }
                }
                if ((status & 0x8080) == (flash_data & 0x8080))
                {
                    flash_status = ERR_FLASH_NONE;
                }
            }

            else if ((globs->device_type == ATMEL_49F8192A_DEV_ID)   ||
                     (globs->device_type == ATMEL_49F8192AT_DEV_ID))
            {

                /* After successful programing, bit 7 is the inverse of the bit 7
                just programmed */
                if ((status & 0x0080) != (flash_data & 0x8080))
                {
                    flash_status = ERR_FLASH_NONE;
                }
            }


            /* INTEL 28F800T Programming algorithm */
            else if (globs->device_type == INTEL_28F800T_DEV_ID ||
                     globs->device_type == INTEL_28F800B_DEV_ID)
            {
                /* Wait for bit 7 to become active */
                if (status & 0x0080)
                {
                    /* Check bit 3 & 4 to see if an error occurred */
                    if (status & 0x0018)
                    {
                        flash_status = ERR_FLASH_PROG;
                    }
                    else
                    {
                        flash_status = ERR_FLASH_NONE;
                    }
                }
            }

            else if (globs->device_type == M29W800_DEV_ID)
            {
                /* lower bit 7 of flash_data -> ready */
                if ((status & 0x0080) == (flash_data & 0x0080))
                {
                    flash_status = ERR_FLASH_NONE;
                }
                else
                {
                    /* bit 5 == 1 -> possible ERROR */
                    if ((status & 0x20) == 0x20)
                    {
                        status1 = *flash_ptr;
                        /* check bit 7 once more */
                        /* if bit 7 of status != bit 7 of flash_data -> ERROR */
                        if ((status1 & 0x80) != (flash_data & 0x80))
                        {
                            flash_status = ERR_FLASH_PROG;
                        }
                        else
                        {
                            flash_status = ERR_FLASH_NONE;
                        }

                    }
                }

            }


            count++;

        }
        while (flash_status == ERR_FLASH_CLEAR);


        /* Reset device to read mode */
        if ((globs->device_type == AMD_29F040_DEV_ID)       ||
                (globs->device_type == SST_39SF040_DEV_ID)      ||
                (globs->device_type == ATMEL_49F8192A_DEV_ID)   ||
                (globs->device_type == ATMEL_49F8192AT_DEV_ID))

        {
            * (globs->amd_addr_1) = AMD_READ_RESET;
        }

        /* Get out of loop if an error in programming FLASH occurred */
        if (flash_status == ERR_FLASH_PROG)
        {
            break;
        }

        /* Increment the address where to write to FLASH */
        flash_ptr++;
    }

    if (globs->device_type == INTEL_28F800T_DEV_ID ||
            globs->device_type == INTEL_28F800B_DEV_ID)
    {
        *flash_ptr = INTEL_READ_ARRAY;
    }

    if (flash_status != ERR_FLASH_NONE)
    {
        state = FLASH_PROGRAM_ERROR;
    }
    else
    {
        state = FLASH_PROGRAM_SUCCESS;
    }
    return state;
}

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: autoselect_flash
*
*  ABSTRACT:
*     Called to determine the type of FLASH installed on the Logic
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        UNKNOWN_FLASH_ID
*        AMD_UNLOCK_CMD1
*        AMD_UNLOCK_CMD2
*        INTELLIGENT_ID_CMD
*        AMD_MAN_ID
*        AMD_29F040_DEV_ID
*        AMD_READ_RESET
*        AMD_29F040_ID
*        INTEL_CLEAR_REGISTER
*        INTEL_ID_CMD
*        INTEL_MAN_ID
*        INTEL_28F800T_DEV_ID
*        INTEL_28F800T_ID
*
*     Procedure Parameters:
*        globs                      struct interface_data_t *
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*        return_value               State_t
*
*  FUNCTIONAL DESCRIPTION:
*     The purpose of this function is to identify the type of FLASH installed
*  on the Logic. This needs to be done so that the correct programming
*  algorithm is selected when FLASH is written. See the data sheets
*  for the FLASH chip to identify the algorithm required to ascertain the
*  Manufacturer and chip ID.
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
*  16 Jul 2001 D.Smail
*     Changed to account for the Bottom boot Intel FLASH
******************************************************************************/
State_t Autoselect_flash (struct interface_data_t *globs)
{
    UINT_16 man_id = 0x1234;          /* Manufacturer's ID */
    UINT_16 dev_code = 0x5678;        /* Device Code */

    UINT_16 huge *intel_ptr;
    UINT_16 huge *m29w_ptr;

    State_t return_value = UNKNOWN_FLASH_ID;

    /* disable interrupts */
    IEN = 0;
    _nop();

    * (globs->amd_addr_1) = AMD_UNLOCK_CMD1;
    * (globs->amd_addr_2) = AMD_UNLOCK_CMD2;
    * (globs->amd_addr_3) = INTELLIGENT_ID_CMD;


    /* Read Manufacturer's ID and Device Code */
    man_id = * (UINT_16 *)0x100000;
    dev_code = * (UINT_16 *)0x100002;

    if ((man_id == AMD_MAN_ID) && (dev_code == AMD_29F040_DEV_ID))
    {
        globs->device_type = AMD_29F040_DEV_ID;
        /* Reset device to read mode */
        * (globs->amd_addr_1) = AMD_READ_RESET;
        return AMD_29F040_ID;
    }
    else if ((man_id == SST_MAN_ID) && (dev_code == SST_39SF040_DEV_ID))
    {
        globs->device_type = SST_39SF040_DEV_ID;
        /* Reset device to read mode */
        * (globs->amd_addr_1) = AMD_READ_RESET;
        return SST_39SF040_ID;
    }


    m29w_ptr = (UINT_16 huge *)M29W800_CMD_WORD_ADDR1;
    *m29w_ptr = 0xAA;
    m29w_ptr = (UINT_16 huge *)M29W800_CMD_WORD_ADDR2;
    *m29w_ptr = 0x55;
    m29w_ptr = (UINT_16 huge *)M29W800_CMD_WORD_ADDR3;
    *m29w_ptr = 0x90;
    m29w_ptr = (UINT_16 huge *)M29W800_CMD_WORD_ADDR3;
    *m29w_ptr = 0x98;

    /* Read Manufacturer's ID and Device Code */
    man_id = * (UINT_16 *)0x100000;
    dev_code = * (UINT_16 *)0x100002;


    if (((man_id & 0xFF) == 0x20) && (dev_code == M29W800_DEV_ID))
    {
        globs->device_type = M29W800_DEV_ID;

        // Issue read / reset command
        m29w_ptr = (UINT_16 huge *)M29W800_CMD_WORD_ADDR1;
        *m29w_ptr = 0xAA;
        m29w_ptr = (UINT_16 huge *)M29W800_CMD_WORD_ADDR2;
        *m29w_ptr = 0x55;
        m29w_ptr = (UINT_16 huge *)M29W800_CMD_WORD_ADDR3;
        *m29w_ptr = 0xF0;

        return M29W800DT_DEV_ID;
    }

    intel_ptr = (UINT_16 huge *)0x100000;

    *intel_ptr = INTEL_CLEAR_REGISTER;
    *intel_ptr = INTEL_ID_CMD;
    /* Look for Intel ID */
    man_id = * (UINT_16 *)0x100000;

    *intel_ptr = INTEL_CLEAR_REGISTER;
    *intel_ptr = INTEL_ID_CMD;
    dev_code =  * (UINT_16 *)0x100002;

    if ((man_id == INTEL_MAN_ID) && (dev_code == INTEL_28F800T_DEV_ID))
    {
        globs->device_type = INTEL_28F800T_DEV_ID;
        return INTEL_28F800T_ID;
    }
    else if ((man_id == INTEL_MAN_ID) && (dev_code == INTEL_28F800B_DEV_ID))
    {
        globs->device_type = INTEL_28F800B_DEV_ID;
        return INTEL_28F800B_ID;
    }


    /* restore interrupts */
    //IEN = 1;
    _nop();

    return return_value;

}


/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Erase_AMD29040
*
*  ABSTRACT:
*
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        ERR_FLASH_CLEAR
*        ERR_FLASH_ERASE
*        ERR_FLASH_NONE
*        AMD_UNLOCK_CMD1
*        AMD_UNLOCK_CMD2
*        AMD_ERASE_CMD
*        AMD_ERASE_CHIP
*        AMD_READ_RESET
*
*     Procedure Parameters:
*        globs                  struct interface_data_t *
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*
*  FUNCTIONAL DESCRIPTION:
*
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
*  28 Jan 2002 D.Smail
*     Performed Sector erases
******************************************************************************/
UINT_16 Erase_AMD29040 (struct interface_data_t *globs)
{

    UINT_16 ret_val;      /* return value */
    UINT_16 status;       /* value read from FLASH */

    ret_val = ERR_FLASH_CLEAR;


	/* Send the ERASE command sequence */
    * (globs->amd_addr_1)          = AMD_UNLOCK_CMD1;
    * (globs->amd_addr_2)          = AMD_UNLOCK_CMD2;
    * (globs->amd_addr_3)          = AMD_ERASE_CMD;
    * (globs->amd_addr_4)          = AMD_UNLOCK_CMD1;
    * (globs->amd_addr_5)          = AMD_UNLOCK_CMD2;
    * (globs->amd_addr_6)          = AMD_ERASE_CHIP;


    while (1)
    {

        /* Wait for bit 7 on both chips to become high -> ERASE complete */
        status = * (globs->amd_addr_6);

        if ((globs->device_type == AMD_29F040_DEV_ID) || (globs->device_type == SST_39SF040_DEV_ID))
        {
            if ((status & 0x8080) == 0x8080)
            {
                break;
            }
            /* Check bit 5 on both chips to see if an error occurred */
            if ((status & 0x2020) == 0x2020)
            {
                /* reread status */
                status = * (globs->amd_addr_6);
                /* If either bit 7 on both chips not high, an error occurred */
                if ((status & 0x8080) != 0x8080)
                {
                    ret_val = ERR_FLASH_ERASE;
                }
                break;
            }
        }

        else if ((globs->device_type == ATMEL_49F8192A_DEV_ID)   ||
                    (globs->device_type == ATMEL_49F8192AT_DEV_ID))
        {
            if ((status & 0x0080) == 0x0080)
            {
                break;
            }
        }

	}


    if (ret_val != ERR_FLASH_ERASE)
    {
        /* Confirm FLASH erased (0xFFFF) */
        status = * (globs->amd_addr_6);
        if (status == 0xFFFF)
        {
            ret_val = ERR_FLASH_NONE;
        }
        else
        {
            ret_val = ERR_FLASH_ERASE;
        }
    }

    /* Reset device to read mode */
    * (globs->amd_addr_1) = AMD_READ_RESET;


    return (ret_val);

}

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Erase_INTEL28F800
*
*  ABSTRACT:
*
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        ERR_FLASH_CLEAR
*        ERR_FLASH_ERASE
*        ERR_FLASH_NONE
*        NUM_OF_SECTORS
*        INTEL_CLEAR_REGISTER
*        INTEL_ERASE_CMD
*        INTEL_ERASE_CONFIRM
*        INTEL_READ_STATUS_REGISTER
*
*
*     Procedure Parameters:
*        globs                  struct interface_data_t *
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*
*  FUNCTIONAL DESCRIPTION:
*
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
*  23 Oct 2001 D.Smail
*  Modified to handle both top and bottom boot
******************************************************************************/
UINT_16 Erase_INTEL28F800 (struct interface_data_t *globs)
{

    UINT_16 ret_val;      /* return value */
    UINT_16 status;       /* value read from FLASH */
    UINT_16 i;            /* index used in loop */
    UINT_16 j;

    UINT_16 num_sectors;

    ret_val = ERR_FLASH_CLEAR;

    if (globs->device_type == INTEL_28F800T_DEV_ID)
    {
        num_sectors = 4;
    }
    else
    {
        num_sectors = 7;
    }


    /* Initialize the sector address */
    globs->intel_sector_addr = (UINT_16 huge *)0x100000;
    for (i = 0; i < num_sectors; i++)
    {
        j = 0;

        /* Send the ERASE sector command */
        *globs->intel_sector_addr = INTEL_CLEAR_REGISTER;
        *globs->intel_sector_addr = INTEL_ERASE_CMD;
        *globs->intel_sector_addr = INTEL_ERASE_CONFIRM;
        *globs->intel_sector_addr = INTEL_READ_STATUS_REGISTER;

        status = * (globs->intel_sector_addr);

        while (1)
        {
            /* read the status register */
            status = * (globs->intel_sector_addr);

            /* Wait for bit 7 to become Active */
            if (status & 0x0080)
            {
                /* Check bit 5 to see if an error occurred */
                if (status & 0x0020)
                {
                    ret_val = ERR_FLASH_ERASE;
                }
                else
                {
                    ret_val = ERR_FLASH_NONE;
                }
                break;
            }
            j++;
        }

        /* Break out of while loop because an error occurred */
        if (ret_val == ERR_FLASH_ERASE)
        {
            break;
        }

        if (globs->device_type == INTEL_28F800T_DEV_ID)
        {
            globs->intel_sector_addr += 0x10000;
        }

        else
        {
            switch (i)
            {
                case 0:
                    /* advance to next sector (16k byte) */
                    globs->intel_sector_addr += 0x2000;
                    break;

                /* advance to next sector (8k byte) */
                case 1:
                case 2:
                    globs->intel_sector_addr += 0x1000;
                    break;

                /* advance to next sector (128k byte) */
                default:
                    globs->intel_sector_addr += 0x10000;
                    break;
            }
        }

    }

    return (ret_val);

}



/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Erase_M29W800
*
*  ABSTRACT:
*
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*
*     Procedure Parameters:
*        globs                  struct interface_data_t *
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*
*  FUNCTIONAL DESCRIPTION:
*     Erases new FLASH chip M29W800DT installed on HiDacs due to obsolescence
*
* .b
*
* History :
*  29 Sep 2013 D.Smail
*     Created
* Revised :
******************************************************************************/
UINT_16 Erase_M29W800 (struct interface_data_t *globs)
{

    UINT_16 ret_val;      /* return value */
    UINT_16 status;       /* value read from FLASH */

    UINT_16 huge *m29w800_ptr;


    ret_val = ERR_FLASH_CLEAR;

    /* FLASH Chip erase sequence */
    m29w800_ptr = (UINT_16 huge *)0x100AAA;
    *m29w800_ptr = 0xAA;

    m29w800_ptr = (UINT_16 huge *)0x100554;
    *m29w800_ptr = 0x55;

    m29w800_ptr = (UINT_16 huge *)0x100AAA;
    *m29w800_ptr = 0x80;

    m29w800_ptr = (UINT_16 huge *)0x100AAA;
    *m29w800_ptr = 0xAA;

    m29w800_ptr = (UINT_16 huge *)0x100554;
    *m29w800_ptr = 0x55;

    m29w800_ptr = (UINT_16 huge *)0x100AAA;
    *m29w800_ptr = 0x10;

    /* Set the pointer to read erase status */
    m29w800_ptr = (UINT_16 huge *)0x100000;

    while (1)
    {
        /* read the status register */
        status = * (m29w800_ptr);

        /* Wait for bit 7 to become Active */
        if (status & 0x0080)
        {
            ret_val = ERR_FLASH_NONE;
            break;
        }
        else
        {
            /* Check bit 5 to see if an error occurred */
            if (status & 0x0020)
            {
                /* error bit set; one last chance to see if erase occurred */
                status = * (m29w800_ptr);
                /* Erase occurred successfully if bit 7 set */
                if (status & 0x0080)
                {
                    ret_val = ERR_FLASH_NONE;
                }
                else
                {
                    /* Error occurred; inform calling function */
                    ret_val = ERR_FLASH_ERASE;
                }
                break;
            }
        }
    }


    return (ret_val);

}