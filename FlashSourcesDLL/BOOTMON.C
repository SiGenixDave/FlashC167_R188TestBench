/***************************************************************************
*.b
*  Copyright (c) 2000-2013 Bombardier Transportation
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : bootmon.c
*  Subsystem  : PC (MS-DOS)
*  Procedures : Boot_strap_loader_monitor
*               Send_byte_wait_for_echo
*
*  Abstract   :
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

#include "include.h"

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Boot_strap_loader_monitor
*
*  ABSTRACT:
*     Converts the Boot loader files from Intel HEX format and transmits
*   the contents to the Logic
*
*  INPUTS:
*
*     Globals:
*         None
*
*     Constants:
*         EOF
*
*     Procedure Parameters:
*       com_port      ASYNC *       comm port selected by the user
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*
*  FUNCTIONAL DESCRIPTION:
*     This function is responsible for opening all 3 stage boot loader Intel
*   HEX files, converting them to 167 files, and then transmitting the binary
*   equivalent contents of the 167 files to the logic.
*     The first step is to convert all 3 Intel HEX files to text files. Text
*   files are ASCII equivalents of the bytes in the Intel HEX files.
*   The function then sends the NULL character to the Logic so the Logic
*   can perform an auto-detect. After the LOGIC echoes back the C167 chip code
*   the  function transmits the 1st stage boot loader. After completing the
*   first stage boot loader, the second stage loader is sent. Finally the third
*   stage loader is sent.
*
* .b
*
* History :
*  01 Apr 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
char *f_stage1_hex;  /* stage 1 Intel Hex download file */

char *f_stage2_hex;  /* stage 2 Intel Hex download file */

char *f_stage3_hex;  /* stage 3 Intel Hex download file */

int Boot_strap_loader_monitor (void)
{

    char stage1_167_name[] = "stage1.167";
    FILE *f_stage1_167;  /* pointers to stage 1 download file */

    char stage2_167_name[] = "stage2.167";
    FILE *f_stage2_167;  /* pointers to stage 2 download file */

    char stage3_167_name[] = "stage3.167";
    FILE *f_stage3_167;  /* pointers to stage 3 download file */

    time_t start_t;   /* start time of a process; used to measure timeouts */
    time_t current_t; /* current time of a process; used to measure timeouts */

    char indata_hi; /* first ASCII nibble read from a file */
    char indata_lo; /* second ASCII nibble read from a file */

    unsigned char outdata; /* combination of first and second binary nibbles */

    struct echo_t logic_val; /* non-zero if logic doesn't respond or if
							 incorrect echo */

    unsigned long ctr = 0;
    unsigned char rc_data;
    unsigned char parse_complete;

    f_stage1_167 = (FILE *)fopen (stage1_167_name, "w+");
    if (f_stage1_167 == (FILE *) 0)
    {
        /* unable to open file */
        printf ("** Unable to open file %s \n", stage1_167_name);
        return (1);
    }

    f_stage2_167 = (FILE *)fopen (stage2_167_name, "w+");
    if (f_stage2_167 == (FILE *) 0)
    {
        /* unable to open file */
        printf ("** Unable to open file %s \n", stage2_167_name);
        fclose (f_stage1_167);
        return (2);
    }

    f_stage3_167 = (FILE *) fopen (stage3_167_name, "w+");
    if (f_stage3_167 == (FILE *) 0)
    {
        /* unable to open file */
        printf ("** Unable to open file %s \n", stage3_167_name);
        fclose (f_stage1_167);
        fclose (f_stage2_167);
        return (3);
    }


    /* Convert STAGE1.HEX to STAGE1.167 */
    f_stage1_hex = GetStage1();
    parse_complete = Parse_hex_file (f_stage1_hex, NULL, f_stage1_167, 0);
    if (parse_complete)
    {
        printf ("** Error parsing Intel Hex File Stage1.hex resource file **\n");
        return (4);
    }


    /* Convert STAGE2.HEX to STAGE2.167 */
    f_stage2_hex = GetStage2();
    parse_complete = Parse_hex_file (f_stage2_hex, NULL, f_stage2_167, 0);
    if (parse_complete)
    {
        printf ("** Error parsing Intel Hex File Stage2.hex resource file **\n");
        return (5);
    }

    /* Convert STAGE3.HEX to STAGE3.167 */
    f_stage3_hex = GetStage3();
    parse_complete = Parse_hex_file (f_stage3_hex, NULL, f_stage3_167, 0);
    if (parse_complete)
    {
        printf ("** Error parsing Intel Hex File Stage3.hex resource file **\n");
        return (6);
    }

    /* Close the 167 files from write mode */
    fclose (f_stage1_167);
    fclose (f_stage2_167);
    fclose (f_stage3_167);

    /* Reopen 167 files for reading only */
    f_stage1_167 = (FILE *)fopen (stage1_167_name, "r");
    if (f_stage1_167 == (FILE *) 0)
    {
        /* unable to open file */
        printf ("** Unable to open file %s \n", stage1_167_name);
        return (7);
    }

    f_stage2_167 = (FILE *)fopen (stage2_167_name, "r");
    if (f_stage2_167 == (FILE *) 0)
    {
        /* unable to open file */
        printf ("** Unable to open file %s \n", stage2_167_name);
        fclose (f_stage1_167);
        return (8);
    }

    f_stage3_167 = (FILE *)fopen (stage3_167_name, "r");
    if (f_stage3_167 == (FILE *) 0)
    {
        /* unable to open file */
        printf ("** Unable to open file %s \n", stage3_167_name);
        fclose (f_stage1_167);
        fclose (f_stage2_167);
        return (9);
    }

    printf ("\t> Connecting to the C167 boot strap loader .... \n");

    /* send NULL byte to C167 for auto baud detection */
    a_putc (0);

    /* wait for acknowledge (0xA5 or 0xC5 from C167)*/
    start_t = time (NULL);

    do
    {
        rc_data = a_getc();
        current_t = time (NULL);
    }
    while (((rc_data == 0xff) || (rc_data == 0x00)) &&
            ((current_t - start_t) < BOOT_SERIAL_PORT_TIMEOUT_SECONDS));

    if ((current_t - start_t) >= BOOT_SERIAL_PORT_TIMEOUT_SECONDS)
    {
        printf ("**** C167 Boot Strap Loader did not respond: Ensure connection & BSL mode");
        return (10);
    }
    else if (! (rc_data == 0xC5 || rc_data == 0xA5 || rc_data == 0xD5))
    {
        printf ("**** C167 Boot Strap Loader did not respond with valid chip ID: (%02X)", rc_data);
        return (10);
    }

    printf ("\t> Connected to the C167 boot strap loader \n");
    printf ("\t> CPU Code: %XH\n", rc_data);
    printf ("\t> Sending stage1 boot loader ..................");

    while (!feof (f_stage1_167))
    {
        /* Parse stage1 167 file until the end is reached */
        indata_hi = fgetc (f_stage1_167);
        indata_lo = fgetc (f_stage1_167);

        if ((indata_hi != EOF) || (indata_lo != EOF))
        {
            /* convert ASCII indata to binary outdata */
            outdata = ASCII_nibbles_to_binary_byte (indata_hi , indata_lo);

            a_putc (outdata);
            /* delay */
            for (ctr = 0; ctr < 0xfff0; ctr++);

        }
    }

    printf (" DONE\n");

    printf ("\t> Sending stage2 boot loader ..................");

    while (!feof (f_stage2_167))
    {
        /* when end of file is not reached */
        indata_hi = fgetc (f_stage2_167);
        indata_lo = fgetc (f_stage2_167);

        if ((indata_hi != EOF) || (indata_lo != EOF))
        {
            /* convert ASCII indata to binary outdata */
            outdata = ASCII_nibbles_to_binary_byte (indata_hi , indata_lo);

            a_putc (outdata);

            /* delay */
            for (ctr = 0; ctr < 0xfff0; ctr++);
        }
    }
    printf (" DONE\n");

    printf ("\t> Sending stage3 boot loader ..................");

    /* Wait */

    /* ctr used as a transmit byte count */
    ctr = 0;
    while (!feof (f_stage3_167))
    {
        /* when end of file is not reached */
        indata_hi = fgetc (f_stage3_167);
        indata_lo = fgetc (f_stage3_167);

        if ((indata_hi != EOF) || (indata_lo != EOF))
        {
            /* convert ASCII indata to binary outdata */
            outdata = ASCII_nibbles_to_binary_byte (indata_hi , indata_lo);
            logic_val = Send_byte_wait_for_echo ((unsigned int)outdata , BOOT_SERIAL_PORT_TIMEOUT_SECONDS);
            ctr++;
            if (logic_val.error_code == ECHO_TIMEOUT)
            {
                printf ("\n**** Lost Communication with target");
                printf ("\n**** Logic Address: %lx", (0x200000UL) + ctr - 1);
                fclose (f_stage1_167);
                fclose (f_stage2_167);
                fclose (f_stage3_167);
                return (11);
            }
            else if (logic_val.error_code == INVALID_ECHO)
            {
                printf ("\n**** Received invalid echo from target");
                printf ("\n**** Logic Address: %lX  Echoed Data: %02X", (0x200000UL + ctr - 1), (logic_val.echo & 0x00ff));
                fclose (f_stage1_167);
                fclose (f_stage2_167);
                fclose (f_stage3_167);
                return (11);
            }
        } /* while not end of while */
    }

    printf (" DONE\n");

    /* close files */
    fclose (f_stage1_167);
    fclose (f_stage2_167);
    fclose (f_stage3_167);


    return (0);
}


/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Send_byte_wait_for_echo
*
*  ABSTRACT:
*     Transmit a byte to the logic and wait for the echo
*
*  INPUTS:
*
*     Globals:
*       None
*
*     Constants:
*       NULL
*       TRUE
*       FALSE
*
*     Procedure Parameters:
*       send_byte         char
*       com_port          ASYNC *
*       timeout           time_t
*
*  OUTPUTS:
*
*     Global Variables:
*       None
*
*     Returned Value:
*       TRUE if echo received in given time, otherwise FALSE
*
*  FUNCTIONAL DESCRIPTION:
*     Responsible for transmitting a byte to the logic and waiting for the
*   echoed response. If the logic echoes the response within the given
*   time, the function returns TRUE. If the logic does not echo or responds
*   with another byte(s) other than the transmitted byte, the timer will
*   expire and the function returns FALSE.
*
*
* .b
*
* History :
*  01 Apr 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
struct echo_t Send_byte_wait_for_echo (unsigned int send_byte ,
                                       time_t timeout)
{

    time_t start_t;     /* start time (seconds); used for timeout calculation. */
    time_t current_t;   /* current time (seconds); used for timeout calculation. */

    struct echo_t info;  /* Stores info regarding the success/failure of this
						 function. Returned to calling function */

    int rc_data;  /* value received from selected comm port */

    static unsigned int count = 0;

    /* Send the command to the logic */
    a_putc (send_byte);

    /* Get the start time */
    start_t = time (NULL);

    /* Wait for the echo from the Logic or until the timeout expires */
    do
    {
        current_t = time (NULL);
        if ((current_t - start_t) >= timeout)
        {
            break;
        }
        rc_data = a_getc();
    }
    while (rc_data == -1);

    /* incorrect echo, inform the calling function */
    if ((rc_data != send_byte) && (rc_data != -1))
    {
        info.echo = rc_data;
        info.error_code = INVALID_ECHO;
    }
    /* timer expired, inform the calling function */
    else if ((current_t - start_t) >= timeout)
    {
        info.error_code = ECHO_TIMEOUT;
    }
    else
    {
        info.error_code = 0;
    }

    return info;

}


