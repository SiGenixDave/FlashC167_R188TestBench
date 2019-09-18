/***************************************************************************
*.b
*  Copyright (c) 2000 DaimlerChrysler Rail Systems (North America) Inc
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : MAIN.C
*  Subsystem  : Third stage boot loader
*  Procedures : main
*               Get_command
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
#include "flash.h"

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: main
*
*  ABSTRACT:
*     Start of code responsible for downloading and programming application
*   code in FLASH
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        WAIT_FOR_COMMAND
*        GET_BYTE_TOTAL
*        DOWNLOAD_BLOCK_CODE
*        FLASH_ERASE_SUCCESS
*        FLASH_ERASE_ERROR
*        FLASH_PROGRAM_SUCCESS
*        FLASH_PROGRAM_ERROR
*        DOWNLOAD_CRC_PARAMETERS
*        CRC_PASS_STATE
*        CRC_FAIL_STATE
*        DOWNLOAD_COMPLETE
*        DOWNLOAD_ERROR
*        UNKNOWN_COMMAND
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
*        None
*
*  FUNCTIONAL DESCRIPTION:
*     This function is responsible sending a success response to the PC
*  (2 byte messages with a '*' leading the alpha character) or failure
*  response (2 byte messages with a '$' leading the alpha character),
*  allowing the PC to ascertain whether the requested action passed or
*  failed. The state variable is modified after a valid command comes from
*  the PC and action either runs to completion (block download), passes
*  (CRC calculation OK), or fails (CRC calculation not OK).
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
UINT_8 byteCount;

void main (void)
{
    long i;

    struct interface_data_t globs;  /* All shared function variables */

    State_t cmd_response;  /* Determines response sent to the PC;
						   based on the received command and action
						   success or fail */

    IEN = 0; /* Disable interrupts & keep them disabled */

    /* NOPs left over from Jahnavi; not sure why */
    _nop();
    _nop();

    /* This needs to be done because if successive attempts at programming FLASH
    are made, the setting for reset may get "stuck" in memory. */
    globs.reset = 0;


    /* Initialize all command addresses */
    globs.amd_addr_1 = (UINT_16 huge *)AMD_CMD_WORD_ADDR1;
    globs.amd_addr_2 = (UINT_16 huge *)AMD_CMD_WORD_ADDR2;
    globs.amd_addr_3 = (UINT_16 huge *)AMD_CMD_WORD_ADDR3;
    globs.amd_addr_4 = (UINT_16 huge *)AMD_CMD_WORD_ADDR4;
    globs.amd_addr_5 = (UINT_16 huge *)AMD_CMD_WORD_ADDR5;
    globs.amd_addr_6 = (UINT_16 huge *)AMD_CMD_WORD_ADDR6;


    while (1)
    {
        /* Wait for a command from the PC */
        cmd_response = Get_command (&globs);

        switch (cmd_response)
        {

            /* Inform PC ready to accept block data */
            case GET_BYTE_TOTAL:
                io_putbyte ('*');
                io_putbyte ('T');
                break;
            /* Download the block */
            case DOWNLOAD_BLOCK_CODE:
                io_putbyte ('*');
                io_putbyte ('B');
                break;

            /********************************************/
            /* FLASH identification                     */
            /********************************************/
            case UNKNOWN_FLASH_ID:
                io_putbyte ('*');
                io_putbyte ('0');
                break;
            case AMD_29F040_ID:
                io_putbyte ('*');
                io_putbyte ('1');
                break;
            case INTEL_28F800T_ID:
                io_putbyte ('*');
                io_putbyte ('2');
                break;
            case INTEL_28F800B_ID:
                io_putbyte ('*');
                io_putbyte ('3');
                break;
            case ATMEL_49F8192A_ID:
                io_putbyte ('*');
                io_putbyte ('4');
                break;
            case ATMEL_49F8192AT_ID:
                io_putbyte ('*');
                io_putbyte ('5');
                break;
            case M29W800DT_DEV_ID:
                io_putbyte ('*');
                io_putbyte ('6');
                break;
            case SST_39SF040_ID:
                io_putbyte ('*');
                io_putbyte ('7');
                break;

            /********************************************/
            /* FLASH programming and erase operations   */
            /********************************************/
            /* FLASH erasing successful */
            case FLASH_ERASE_SUCCESS:
                io_putbyte ('*');
                io_putbyte ('E');
                break;
            /* FLASH erasing unsuccessful */
            case FLASH_ERASE_ERROR:
                io_putbyte ('$');
                io_putbyte ('E');
                break;
            /* Programmed the FLASH block successfully */
            case FLASH_PROGRAM_SUCCESS:
                io_putbyte ('*');
                io_putbyte ('P');
                break;
            /* Did not program the FLASH block successfully */
            case FLASH_PROGRAM_ERROR:
                io_putbyte ('$');
                io_putbyte ('P');
                break;
            /*****************************************/

            /************************************/
            /* CRC operations                   */
            /************************************/
            /* CRC calculation successful */
            case CRC_PASS_STATE:
                io_putbyte ('*');
                io_putbyte ('R');
                break;
            /* CRC calculation unsuccessful */
            case CRC_FAIL_STATE:
                io_putbyte ('$');
                io_putbyte ('R');
                break;
            /************************************/

            /************************************/
            /* Reset PCB                        */
            /************************************/
            /* PCB Reset successful */
            case  RESET_COMPLETE:
                io_putbyte ('*');
                io_putbyte ('S');
                break;
            /* PCB Reset unsuccessful */
            case RESET_ERROR:
                io_putbyte ('$');
                io_putbyte ('S');
                break;
            /************************************/


            /* Inform PC that the correct number of bytes were received; utilized just
            in case user doesn't request a CRC calculation */
            case DOWNLOAD_COMPLETE:
                io_putbyte ('*');
                io_putbyte ('Z');
                break;
            /* Inform PC that an incorrect number of bytes were received; utilized
            just in case user doesn't request a CRC calculation */
            case DOWNLOAD_ERROR:
                io_putbyte ('$');
                io_putbyte ('Z');
                break;

            /* Invalid command received from the PC */
            case UNKNOWN_COMMAND:
                io_putbyte ('*');
                io_putbyte ('U');
                break;

            default:
                break;

        }

        /* PCB reset requested */
        if (globs.reset == 0xDEADBEEF)
        {
            for (i = 0; i < 10000000; i++);
            _int166 (0);
        }

    }
}


/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Get_command
*
*  ABSTRACT:
*     Echo command, interpret it, and change state
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        WAIT_FOR_COMMAND
*        GET_BYTE_TOTAL
*        DOWNLOAD_BLOCK_CODE
*        DOWNLOAD_CRC_PARAMETERS
*        DOWNLOAD_COMPLETE
*        DOWNLOAD_ERROR
*        UNKNOWN_COMMAND
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
*        state        State_t         state after requested operation performed
*
*  FUNCTIONAL DESCRIPTION:
*     This function is responsible for getting a command from the PC. It then
*  echoes the command to the PC. Verification of the command takes place. If
*  the command is valid, the appropriate operation is performed. Depending
*  on the operation and the result, the state is modified (usually a Pass/Fail)
*
* .b
*
* History :
*  01 May 2000 D.Smail
*     Created
* Revised :
******************************************************************************/

State_t Get_command (struct interface_data_t *globs)
{

    State_t state;  /* returned value */
    UINT_8 command; /* ASCII command received from the PC */

    /* In case command doesn't modify the state, set the default return state */
    state = WAIT_FOR_COMMAND;

    /* Get command from PC and after its received, echo it */
    command = io_getbyte();
    io_putbyte (command);

    /* NOP from Jahnavi; unknown reason */
    _nop();

    switch (command)
    {
        /* ESTABLISH CONNECTION WITH PC AND FLASH MONITOR */
        case 'c':
        case 'C':
            /* Do nothing; echo was sufficient */
            break;

        case 'f':
        case 'F':
            /* Determine the type of Flash present */
            state = Autoselect_flash (globs);
            break;


        /* ERASE FLASH */
        case 'e':
        case 'E':
            state = Erase_flash (globs);
            break;

        /* GET TOTAL NUMBER OF BYTES FROM PC */
        case 't':
        case 'T':
            Get_total_bytes (globs);
            state = GET_BYTE_TOTAL;
            break;

        /* RECEIVE BLOCK OF DATA */
        case 'b':
        case 'B':
            Download_block_data (globs);
            state = DOWNLOAD_BLOCK_CODE;
            break;

        /* PROGRAM FLASH */
        case 'p':
        case 'P':
            state = Program_flash (globs);
            break;

        /* PERFORM CRC */
        case 'r':
        case 'R':
            state = Calc_crc (globs);
            break;

        /* Reset C167 PCB */
        case 's':
        case 'S':
            state = ResetPCB (globs);
            break;


        /* Send the ASCII equivalent of the CRC back to the PC */
        case 'g':
        case 'G':
            Get_crc_from_flash (globs);
            break;

        /* PC TERMINATING COMMUNICATION BECAUSE ALL BYTES SENT */
        case 'z':
        case 'Z':
            /* Verify that total bytes has decremented to 0 */
            if (globs->total_bytes == 0)
            {
                state = DOWNLOAD_COMPLETE;
            }
            else
            {
                state = DOWNLOAD_ERROR;
            }
            break;

        default:
            state = UNKNOWN_COMMAND;
            break;
    }

    return state;

}




