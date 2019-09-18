/***************************************************************************
*.b
*  Copyright (c) 2000-2013 Bombardier Transportation
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : flashmon.c
*  Subsystem  : PC (MS-DOS)
*  Procedures : Flash_monitor
*               Wait_for_command_reponse
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
*  PROCEDURE NAME: Flash_monitor
*
*  ABSTRACT:
*     Responsible for transmitting the application code that is to be
*   programmed into FLASH to the Logic
*
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*       CMD_COMPLETE
*       CMD_FAILED
*       FLASH_COMMAND_TIMEOUT_SECONDS
*       FLASH_SERIAL_PORT_TIMEOUT_SECONDS
*       TRUE
*       GET_TOTAL_BYTES
*       SEND_BLOCK_ADDRESS_AND_SIZE
*       SEND_BLOCK_PROGRAM_DATA
*
*     Procedure Parameters:
*       com_port        ASYNC *
*       files           struct file_info_t
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*       int           0 if successful; unique error codes if unsuccessful
*
*  FUNCTIONAL DESCRIPTION:
*
*
* .b
*
* History :
*  01 Apr 2000 D.Smail
*     Created
* Revised :
*  29 Sep 2013 D.Smail - Added support for M29W800 FLASH chip
******************************************************************************/
int Flash_monitor (struct file_info_t files, char *crc_string)
{

    FILE *f_flashapp_167; /* stores the application text file */

    struct echo_t logic_val; /* determines if communication between PC
							 and logic successful */

    int i;  /* index used in looping */

    char ascii_hi; /* hi nibble read from text file (ASCII) */
    char ascii_lo; /* lo nibble read from text file (ASCII) */

    unsigned char outdata;  /* binary combination of ascii_hi & ascii_lo */

    char hex_buf[100];  /* stores string from CRC configuration file */

    unsigned long num_bytes_sent_total;  /* running total of number of bytes sent
										 to Logic; includes block address, block
										 size and data; compared to code_size
										 to determine when end of comm takes
										 place */

    unsigned int build_code_size_var; /* increments in GET_TOTAL_BYTES state
									  when reading text file to create the
									  value code_size */
    unsigned long code_size;  /* total of number of bytes to be sent to Logic;
							  generated from first line in text file */


    unsigned long percent_complete; /* stores percentage of download complete;
									scaled where:
									1    =   0.1%
									10   =   1.0%
									100  =  10.0%
									1000 = 100.0% */
    unsigned long old_percent_complete; /* stores percent_complete; used in
										comparisons to determine if screen
										updates required */

    unsigned int stream_position;

    unsigned int block_address_state;

    unsigned int block_size;

    unsigned int num_bytes_sent_block;

    unsigned long temp_32;

    unsigned char parse_complete;

    unsigned char file_read_in_progress;

    unsigned char crc_width;

    unsigned char crc_rx_fail;

    unsigned char unknown_flash_id;

    int command_response;

    struct flash_t flash_type;

    /* Initialize local variables */
    num_bytes_sent_total = 0;
    code_size = 0;
    percent_complete = 0;
    old_percent_complete = 0;
    num_bytes_sent_block = 0;
    unknown_flash_id = FALSE;

    /* Create the text file that contains the data to be transmitted to the Logic */
    printf ("\t> Creating application download file .......... %s \n", files.flashapp_hex_name);
    f_flashapp_167 = (FILE *)fopen (files.flashapp_hex_name, "w+");
    if (f_flashapp_167 == (FILE *) 0)
    {
        /* unable to open file */
        printf ("** Unable to open download file %s \n",
                files.flashapp_hex_name);
        return (1);
    }

    /* Convert from Intel hex format to application specific text format */
    parse_complete = Parse_hex_file (NULL, files.f_flashapp_hex, f_flashapp_167, 1);

    fclose (files.f_flashapp_hex);
    fclose (f_flashapp_167);

    /* Return if the parsing of the Intel Hex file failed */
    if (parse_complete)
    {
        return (2);
    }

    /*********************************************************************/
    /************ MAKE CONNECTION WITH FLASH MONITOR IN LOGIC ************/
    /*********************************************************************/
    printf ("\t> Connecting with Logic .......................");
    logic_val = Send_byte_wait_for_echo ('c', FLASH_SERIAL_PORT_TIMEOUT_SECONDS);

    if (logic_val.error_code == ECHO_TIMEOUT)
    {
        printf ("\n**** Lost Communication with target: did not receive 'c' ");
        return (3);
    }
    else if (logic_val.error_code == INVALID_ECHO)
    {
        printf ("\n**** Received invalid echo from target: did not receive 'c' ");
        return (3);
    }
    else
    {
        printf (" SUCCESSFUL\n");
    }


    /*********************************************************************/
    /******************* INQUIRE LOGIC FOR FLASH TYPE ********************/
    /*********************************************************************/

    printf ("\t> Inquiring Flash Type ........................ ");
    logic_val = Send_byte_wait_for_echo ('f', FLASH_SERIAL_PORT_TIMEOUT_SECONDS);

    if (logic_val.error_code == ECHO_TIMEOUT)
    {
        printf ("\n**** Lost Communication with target: did not receive 'f' ");
        return (4);
    }
    else if (logic_val.error_code == INVALID_ECHO)
    {
        printf ("\n**** Received invalid echo from target: did not receive 'f' ");
        return (4);
    }

    /* wait for command response */
    flash_type = Get_flash_type (FLASH_COMMAND_TIMEOUT_SECONDS);

    if (flash_type.cmd_response == CMD_COMPLETE)
    {
        switch (flash_type.id)
        {
            case AMD_29F040_ID:
                printf ("AMD 29F040\n");
                break;
            case INTEL_28F800T_ID:
                printf ("INTEL 28F800T\n");
                break;
            case INTEL_28F800B_ID:
                printf ("INTEL 28F800B\n");
                break;
            case ATMEL_49F8192A_ID:
                printf ("ATMEL 49F8192A\n");
                break;
            case ATMEL_49F8192AT_ID:
                printf ("ATMEL 49F8192AT\n");
                break;
            case M29W800DT_ID:
                printf ("M29W800DT\n");
                break;
            case SST_39SF040_ID:
                printf ("SST 39SF040\n");
                break;
            default:
                printf ("UNKNOWN\n");
                unknown_flash_id = TRUE;
                break;
        }
        if (unknown_flash_id == TRUE)
        {
            printf ("\n**** Logic returned an unknown FLASH ID ****");
            return (4);
        }


    }
    else
    {
        if (flash_type.cmd_response == CMD_FAILED)
        {
            printf ("\n**** Command failed **** \n");
        }
        else
        {
            printf ("\n**** Timed out waiting for target response ***** \n");
        }
        return (4);
    }



    /*********************************************************************/
    /******************** COMMAND LOGIC TO ERASE FLASH *******************/
    /*********************************************************************/
    printf ("\t> Erasing Flash Command .......................");
    logic_val = Send_byte_wait_for_echo ('e', FLASH_SERIAL_PORT_TIMEOUT_SECONDS);

    if (logic_val.error_code == ECHO_TIMEOUT)
    {
        printf ("\n**** Lost Communication with target: did not receive 'e' ");
        return (5);
    }
    else if (logic_val.error_code == INVALID_ECHO)
    {
        printf ("\n**** Received invalid echo from target: did not receive 'e' ");
        return (5);
    }
    else
    {
        printf (" ACKNOWLEDGED\n");
    }


    printf ("\t> Erasing flash ...............................");

    /* wait for command response */
    command_response = Wait_for_command_reponse ("*E", FLASH_COMMAND_TIMEOUT_SECONDS);

    if (command_response == CMD_COMPLETE)
    {
        printf (" COMPLETE \n");
    }
    else
    {
        if (command_response == CMD_FAILED)
        {
            printf ("\n**** Command failed: '$E' received \n");
        }
        else
        {
            printf ("\n**** Timed out waiting for target response: '*E' \n");
        }
        return (5);
    }



    /*********************************************************************/
    /****************** INFORM LOGIC DOWNLOAD TO BEGIN *******************/
    /*********************************************************************/
    logic_val = Send_byte_wait_for_echo ('t', FLASH_SERIAL_PORT_TIMEOUT_SECONDS);

    if (logic_val.error_code == ECHO_TIMEOUT)
    {
        printf ("\n**** Lost Communication with target: did not receive 't' ");
        return (6);
    }
    else if (logic_val.error_code == INVALID_ECHO)
    {
        printf ("\n**** Received invalid echo from target: did not receive 't' ");
        return (6);
    }


    f_flashapp_167 = (FILE *)fopen (files.flashapp_hex_name, "r");
    if (f_flashapp_167 == (FILE *) 0)
    {
        /* unable to open file */
        printf ("** Unable to open application 167 file \n");
        return (7);
    }


    stream_position = GET_TOTAL_BYTES;
    build_code_size_var = 0;
    file_read_in_progress = TRUE;

    /* Download the entire contents of the application text file */
    while (file_read_in_progress == TRUE)
    {
        /* when end of file is not reached */
        do
        {
            ascii_hi = fgetc (f_flashapp_167);
        }
        while (ascii_hi == '\n');

        do
        {
            ascii_lo = fgetc (f_flashapp_167);
        }
        while (ascii_lo == '\n');

        /* convert ASCII indata to binary outdata */
        outdata = ASCII_nibbles_to_binary_byte (ascii_hi , ascii_lo);

        /* Send binary byte to logic */
        a_putc (outdata);
        num_bytes_sent_total++;
        num_bytes_sent_block++;

        switch (stream_position)
        {
            /* The first 4 bytes in the text file and sent to the logic are the
            total number of bytes to be downloaded */
            case GET_TOTAL_BYTES:
                switch (build_code_size_var)
                {
                    case 0:
                        /* get MSB 4 */
                        code_size  = (unsigned long)outdata << 24;
                        build_code_size_var++;
                        break;
                    case 1:
                        /* get MSB 3 */
                        code_size |= (unsigned long)outdata << 16;
                        build_code_size_var++;
                        break;
                    case 2:
                        /* get MSB 2 */
                        code_size |= (unsigned long)outdata << 8;
                        build_code_size_var++;
                        break;
                    case 3:
                        /* get MSB 1 */
                        code_size |= (unsigned char)outdata;
                        build_code_size_var++;

                        /* WAIT FOR THE LOGIC RESPONSE AFTER SENDING THE TOTAL
                        NUMBER OF BYTES TO BE DOWNLOADED TO THE LOGIC */
                        command_response =
                            Wait_for_command_reponse ("*T",
                                                      FLASH_COMMAND_TIMEOUT_SECONDS);

                        if (command_response == CMD_COMPLETE)
                        {
                            printf ("\t> Bytes to be downloaded ...................... %lu\n", code_size);
                            printf ("\t> Download status ......................   0.0%% complete");
                            printf ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");

                            /* Send block transfer command "b" and verify B returned */
                            logic_val = Send_byte_wait_for_echo ('b' ,
                                                                 FLASH_SERIAL_PORT_TIMEOUT_SECONDS);

                            if (logic_val.error_code == ECHO_TIMEOUT)
                            {
                                printf ("\n**** Lost Communication with target: did not receive 'b' ");
                                return (8);
                            }
                            else if (logic_val.error_code == INVALID_ECHO)
                            {
                                printf ("\n**** Received invalid echo from target: did not receive 'b' ");
                                return (8);
                            }
                            block_address_state = 0;
                            block_size = 0;
                            stream_position = SEND_BLOCK_ADDRESS_AND_SIZE;
                        }
                        else
                        {
                            if (command_response == CMD_FAILED)
                            {
                                printf ("\n**** Command failed: '$T' received \n");
                            }
                            else
                            {
                                printf ("\n**** Timed out waiting for target response: '*T' \n");
                            }
                            return (9);
                        }

                        break;
                    default:
                        break;
                }
                break;

            case SEND_BLOCK_ADDRESS_AND_SIZE:
                switch (block_address_state)
                {
                    /* First 4 bytes are the logic address where the next block of data
                    is to be stored */
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        block_address_state++;
                        break;
                    /* Next 2 bytes are the size of the block to be transferred to the
                    logic (in bytes) */
                    case 4:
                        block_size = outdata << 8;
                        block_address_state++;
                        break;
                    case 5:
                        /* Build "block_size" variable */
                        block_size |= outdata;
                        block_address_state++;
                        stream_position = SEND_BLOCK_PROGRAM_DATA;
                        num_bytes_sent_block = 0;
                        break;
                    default:
                        break;
                }
                break;

            case SEND_BLOCK_PROGRAM_DATA:
                /* Update the screen with the percentage of the download complete.
                Only update when the percent complete has change to avoid extra
                screen update calls */
                percent_complete = (num_bytes_sent_total * 1000) / code_size;
                if (percent_complete != old_percent_complete)
                {
                    printf ("%3lu.%1lu%%\b\b\b\b\b\b", percent_complete / 10,
                            percent_complete % 10);
                    /* Update variable for checking on the next iteration */
                    old_percent_complete = percent_complete;
                }

                /* Determine if the number of bytes in the block have been sent */
                if (num_bytes_sent_block >= block_size)
                {
                    /* Logic will respond with "*B" if it is in agreement that the entire
                    block has been received */
                    command_response =
                        Wait_for_command_reponse ("*B",
                                                  FLASH_COMMAND_TIMEOUT_SECONDS);

                    if (command_response != CMD_COMPLETE)
                    {
                        if (command_response == CMD_FAILED)
                        {
                            printf ("\n**** Command failed: '$B' received \n");
                        }
                        else
                        {
                            printf ("\n**** Timed out waiting for target response: '*B' \n");
                        }
                        return (10);
                    }

                    /********************************************************/
                    /* INFORM LOGIC TO PROGRAM THE BLOCK JUST SENT IN FLASH */
                    /********************************************************/
                    logic_val =
                        Send_byte_wait_for_echo ('p' ,
                                                 FLASH_SERIAL_PORT_TIMEOUT_SECONDS);

                    if (logic_val.error_code == ECHO_TIMEOUT)
                    {
                        printf ("\n**** Lost Communication with target: did not receive 'p' ");
                        return (11);
                    }
                    else if (logic_val.error_code == INVALID_ECHO)
                    {
                        printf ("\n**** Received invalid echo from target: did not receive 'p' ");
                        return (11);
                    }

                    command_response =  Wait_for_command_reponse ("*P", FLASH_COMMAND_TIMEOUT_SECONDS);

                    if (command_response != CMD_COMPLETE)
                    {
                        if (command_response == CMD_FAILED)
                        {
                            printf ("\n**** Command failed: '$P' received \n");
                        }
                        else
                        {
                            printf ("\n**** Timed out waiting for target response: '*P' \n");
                        }
                        return (12);
                    }

                    /* Determine if all bytes have been sent */
                    if (num_bytes_sent_total >= code_size)
                    {
                        /* End the while() */
                        file_read_in_progress = FALSE;
                    }
                    else
                    {
                        /* send the next block */
                        logic_val =  Send_byte_wait_for_echo ('b', FLASH_SERIAL_PORT_TIMEOUT_SECONDS);
                        if (logic_val.error_code == ECHO_TIMEOUT)
                        {
                            printf ("\n**** Lost Communication with target: did not receive 'p' ");
                            return (13);
                        }
                        else if (logic_val.error_code == INVALID_ECHO)
                        {
                            printf ("\n**** Received invalid echo from target: did not receive 'p' ");
                            return (13);
                        }

                        block_address_state = 0;
                        block_size = 0;
                        stream_position = SEND_BLOCK_ADDRESS_AND_SIZE;
                    }
                }
                break;


            default:
                break;
        }

    } /* Loop scanning application text file */


    /* User supplied CRC configuration file (used with GPCRCG.EXE) on the
    command line. Tell Logic to perform CRC on the programmed FLASH */
    if (files.f_config_crc != NULL)
    {
        printf ("\n\t> Programmed FLASH CRC Confirmation ...........");

        /* Tell logic to perform CRC  */
        logic_val = Send_byte_wait_for_echo ('r' ,
                                             FLASH_SERIAL_PORT_TIMEOUT_SECONDS);

        if (logic_val.error_code == ECHO_TIMEOUT)
        {
            printf ("\n**** Lost Communication with target: did not receive 'r' ");
            return (14);
        }
        else if (logic_val.error_code == INVALID_ECHO)
        {
            printf ("\n**** Received invalid echo from target: did not receive 'r' ");
            return (14);
        }
        /* Flush the first 3 lines in crc config file */
        fgets (hex_buf, 100, files.f_config_crc);
        fgets (hex_buf, 100, files.f_config_crc);
        fgets (hex_buf, 100, files.f_config_crc);

        /* Get the CRC width ( 8 , 16 , 32 ) and send to the logic */
        fgets (hex_buf, 100, files.f_config_crc);
        sscanf (hex_buf, "%lu", &temp_32);
        sprintf (hex_buf, "%02lx", temp_32);
        crc_width = (unsigned char)temp_32;

        outdata = ASCII_nibbles_to_binary_byte (hex_buf[0] , hex_buf[1]);

        /* Send binary byte to logic */
        a_putc (outdata);

        /* Get and write the polynomial from the configuration file */
        fgets (hex_buf, 100, files.f_config_crc);
        sscanf (hex_buf, "%lx", &temp_32);
        sprintf (hex_buf, "%08lx", temp_32);

        for (i = 0; i < 8; i += 2)
        {
            outdata = ASCII_nibbles_to_binary_byte (hex_buf[i] , hex_buf[i + 1]);
            /* Send binary byte to logic */
            a_putc (outdata);
        }

        /* Flush another line */
        fgets (hex_buf, 100, files.f_config_crc);

        /* Get and write the ROM start address */
        fgets (hex_buf, 100, files.f_config_crc);
        sscanf (hex_buf, "%lx", &temp_32);
        sprintf (hex_buf, "%08lx", temp_32);
        for (i = 0; i < 8; i += 2)
        {
            outdata = ASCII_nibbles_to_binary_byte (hex_buf[i] , hex_buf[i + 1]);
            /* Send binary byte to logic */
            a_putc (outdata);
        }


        /* Get and write the ROM end address */
        fgets (hex_buf, 100, files.f_config_crc);
        sscanf (hex_buf, "%lx", &temp_32);
        sprintf (hex_buf, "%08lx", temp_32);
        for (i = 0; i < 8; i += 2)
        {
            outdata = ASCII_nibbles_to_binary_byte (hex_buf[i] , hex_buf[i + 1]);
            /* Send binary byte to logic */
            a_putc (outdata);
        }


        /* Get and write the address in FLASH where the CRC is stored. If the
        command line CRC was entered, use 0 because there is no CRC stored in
        just programmed FLASH. This tells the embedded to calculate the CRC,
        but always send a *R to inform a PASS condition */
        fgets (hex_buf, 100, files.f_config_crc);
        if (files.commandline_crc == FALSE)
        {
            sscanf (hex_buf, "%lx", &temp_32);
        }
        else
        {
            temp_32 = 0;
        }
        sprintf (hex_buf, "%08lx", temp_32);

        for (i = 0; i < 8; i += 2)
        {
            outdata = ASCII_nibbles_to_binary_byte (hex_buf[i] , hex_buf[i + 1]);
            /* Send binary byte to logic */
            a_putc (outdata);
        }

        command_response =
            Wait_for_command_reponse ("*R",
                                      FLASH_COMMAND_TIMEOUT_SECONDS);

        if (command_response == CMD_COMPLETE)
        {
            printf (" DONE\n");
        }
        else
        {
            if (command_response == CMD_FAILED)
            {
                printf ("\n\t**** CRC Failed .... Please Try Again ****\n");
            }
            else
            {
                printf ("\n** Timed out waiting for target response.\n");
            }
            fclose (f_flashapp_167);
            return (15);
        }


        /* Get the CRC from the Logic */
        crc_rx_fail = Get_crc_from_logic (crc_width, crc_string, files);
        /* CRC failed if return value is non-zero */
        if (crc_rx_fail > 0)
        {
            switch (crc_rx_fail)
            {
                case 1:
                    printf ("\n**** Lost Communication with target: did not receive 'g' ");
                    break;
                case 2:
                    printf ("\n**** Received invalid echo from target: did not receive 'g' ");
                    break;
                case 3:
                    printf ("\n**** Did not receive CRC data from Logic ");
                    break;
                case 4:
                    printf ("\n**** CRC data from Logic did not agree with command line ");
                    break;
                default:
                    break;
            }
            return (16);
        }
        else
        {
            printf ("\t> For your records, the CRC of the FLASH is --> 0x%s", crc_string);
        }


    }
    else
    {
        printf ("\n\t> ***** WARNING **** No CRC Confirmation of FLASH performed");
    }


    if (files.reset)
    {
        /* Tell logic that the communication is done */
        logic_val = Send_byte_wait_for_echo ('S' ,
                                             FLASH_SERIAL_PORT_TIMEOUT_SECONDS);

        if (logic_val.error_code == ECHO_TIMEOUT)
        {
            printf ("\n**** Lost Communication with target: did not receive 's' ");
            return (19);
        }
        else if (logic_val.error_code == INVALID_ECHO)
        {
            printf ("\n**** Received invalid echo from target: did not receive 's' ");
            return (19);
        }

        /*********************************************************************/
        /******************** END COMMUNICATION WITH LOGIC *******************/
        /*********************************************************************/
        command_response =
            Wait_for_command_reponse ("*S",
                                      FLASH_COMMAND_TIMEOUT_SECONDS);

        fclose (f_flashapp_167);

        if (command_response == CMD_COMPLETE)
        {
            printf ("\n\t> FLASH Programming Successful!");
            printf ("\n\t> PCB is being reset (command line request)... Please allow 5 seconds.");
            return (0);
        }
        else
        {
            if (command_response == CMD_FAILED)
            {
                printf ("\n**** Command failed: '$S' received \n");
            }
            else
            {
                printf ("\n**** Timed out waiting for target response: '*S' \n");
            }
            return (20);
        }
    }
    else
    {

        /* Tell logic that the communication is done */
        logic_val = Send_byte_wait_for_echo ('z' ,
                                             FLASH_SERIAL_PORT_TIMEOUT_SECONDS);


        if (logic_val.error_code == ECHO_TIMEOUT)
        {
            printf ("\n**** Lost Communication with target: did not receive 'z' ");
            return (17);
        }
        else if (logic_val.error_code == INVALID_ECHO)
        {
            printf ("\n**** Received invalid echo from target: did not receive 'z' ");
            return (17);
        }


        /*********************************************************************/
        /******************** END COMMUNICATION WITH LOGIC *******************/
        /*********************************************************************/
        command_response =
            Wait_for_command_reponse ("*Z",
                                      FLASH_COMMAND_TIMEOUT_SECONDS);

        fclose (f_flashapp_167);

        if (command_response == CMD_COMPLETE)
        {
            printf ("\n\t> FLASH Programming Successful! \n");
            return (0);
        }
        else
        {
            if (command_response == CMD_FAILED)
            {
                printf ("\n**** Command failed: '$Z' received \n");
            }
            else
            {
                printf ("\n**** Timed out waiting for target response: '*Z' \n");
            }
            return (18);
        }
    }
}


/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Wait_for_command_reponse
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
*       CMD_UNKNOWN
*       CMD_FAILED
*       CMD_COMPLETE
*       NULL
*
*     Procedure Parameters:
*       cmd           const char *
*       com_port      ASYNC *
*       timeout_secs  long
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
*  15 Jun 1999 DAS
*     Created
* Revised :
******************************************************************************/
int Wait_for_command_reponse (const char *cmd,
                              long timeout_secs)
{
    char indata;

    time_t start_t;

    time_t current_t;

    int ret_val = CMD_UNKNOWN;

    char response[5];         /* max length of response is 4 */

    /* assumption first character received is :
    * command complete response, or
    $ command failed response
    */

    start_t = time (NULL);
    do
    {
        indata = a_getc();
        current_t = time (NULL);
    }
    while ((indata != '$') &&
            (indata != '*') &&
            ((current_t - start_t) < timeout_secs));

    if ((current_t - start_t) >= timeout_secs)
    {
        return (ret_val);
    }
    else
    {
        response[0] = indata;
    }


    start_t = time (NULL);
    do
    {
        indata = a_getc();
        current_t = time (NULL);
    }
    while ((indata != cmd[1]) &&
            ((current_t - start_t) < timeout_secs));

    if ((current_t - start_t) >= timeout_secs)
    {
        return (ret_val);
    }
    else
    {
        response[1] = indata;
    }


    /* did the command pass or fail? */
    if ((response[0] == '*') && (response[1] == cmd[1]))
    {
        ret_val = CMD_COMPLETE;
    }
    else if ((response[0] == '$') && (response[1] == cmd[1]))
    {
        ret_val = CMD_FAILED;
    }
    else
    {
        ret_val = CMD_UNKNOWN;
    }

    return (ret_val);
}



/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: ASCII_nibbles_to_binary_byte
*
*  ABSTRACT:
*     Responsible for converting to ASCII bytes ( hi nibble & lo nibble )
*   to a binary byte
*
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
*        hi_nibble_ascii                  char
*        lo_nibble_ascii                  char
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*        binary value of combined ASCII hi nibble and ASCII lo nibble
*
*  FUNCTIONAL DESCRIPTION:
*     Function is responsible for converting 2 ASCII bytes to a binary
*  value. The ASCII nibbles are converted binary nibbles and then combined
*  to form the binary byte
*
*
* .b
*
* History :
*  15 Jun 2000 DAS
*     Created
* Revised :
******************************************************************************/
unsigned char ASCII_nibbles_to_binary_byte (char hi_nibble_ascii ,
        char lo_nibble_ascii)
{

    unsigned char hi_nibble_binary;  /* binary value of ASCII hi nibble */
    unsigned char lo_nibble_binary;  /* binary value of ASCII hi nibble */

    hi_nibble_binary = ASCII_nibble_to_binary_nibble (hi_nibble_ascii);
    lo_nibble_binary = ASCII_nibble_to_binary_nibble (lo_nibble_ascii);

    /* Create the binary value and return it */
    return ((hi_nibble_binary << 4) | lo_nibble_binary);
}


/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Get_crc_from_logic
*
*  ABSTRACT:
*    Responsible for getting the ASCII representation of the CRC from the
*  Logic
*
*
*  INPUTS:
*
*     Globals:
*        None
*
*     Constants:
*        FLASH_SERIAL_PORT_TIMEOUT_SECONDS
*        TRUE
*        NULL
*        FLASH_COMMAND_TIMEOUT_SECONDS
*        CMD_COMPLETE
*        CMD_FAILED
*
*     Procedure Parameters:
*        com_port                      ASYNC * ,
*        crc_width                     unsigned char ,
*        crc_string                    char * ,
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*        0 if successful
*        non-zero if unsuccessful
*
*  FUNCTIONAL DESCRIPTION:
*     This function asks the Logic to transmit the calculated CRC so the
*  user can view it and confirm the FLASH was programmed correctly. The
*  ASCII representation of the CRC is received from the Logic. For an 8 bit
*  CRC, 2 ASCII Bytes will be sent MSNibble (ASCII) and LSNibble (ASCII). 4
*  ASCII bytes for a 16 bit CRC and 8 ASCII bytes for a 32 bit CRC.
*     The "g" command is transmitted. The software then waits for the ASCII
*  CRC from the Logic. After the CRC is received, the software then waits for
*  command complete acknowledge from the Logic ("*G").
*     If any error occurs, the returned value will be non-zero
*
*
* .b
*
* History :
*  15 Jun 2000 DAS
*     Created
* Revised :
******************************************************************************/
unsigned char Get_crc_from_logic (unsigned char crc_width,
                                  char *crc_string,
                                  struct file_info_t files)
{

    time_t start_t;   /* Start time in seconds; used for timeouts */
    time_t current_t; /* Current time in seconds; used for timeouts */

    int indata;       /* Serial port received data */
    unsigned char i;  /* index for looping */
    struct echo_t logic_val; /* returned value from function call */

    unsigned long logic_crc;


    logic_val = Send_byte_wait_for_echo ('g' ,
                                         FLASH_SERIAL_PORT_TIMEOUT_SECONDS);

    /* Valid echo of g not received, exit */
    if (logic_val.error_code == ECHO_TIMEOUT)
    {
        return (1);
    }
    else if (logic_val.error_code == INVALID_ECHO)
    {
        return (2);
    }

    /* Initialize variables */
    i = 0;
    /* "crc_width" = 8, 16, or 32 prior to division */
    crc_width /= 4;

    /* Logic now transmits the ASCII representation of the CRC
    crc_width is now the number of ASCII bytes to receive (2 , 4, or 8) */
    while (i < crc_width)
    {
        start_t = time (NULL);
        /* Wait for the ASCII byte string of the CRC */
        do
        {
            indata = a_getc();
            current_t = time (NULL);
        }
        while ((indata == -1) &&
                ((current_t - start_t) < (time_t)FLASH_SERIAL_PORT_TIMEOUT_SECONDS));

        /* Timeout occurred */
        if ((current_t - start_t) >= (time_t)FLASH_SERIAL_PORT_TIMEOUT_SECONDS)
        {
            return (3);
        }
        /* Build the string */
        else
        {
            crc_string[i] = (char)indata;
        }
        i++;
    }

    /* Terminate the string */
    crc_string[i] = NULL;

    if (files.commandline_crc != FALSE)
    {
        sscanf (crc_string, "%lx", &logic_crc);
        if (logic_crc != files.commandline_crc)
        {
            printf ("\n**** Logic CRC is %s: Command line CRC is %lX", crc_string,
                    files.commandline_crc);
            return (4);
        }
    }


    return (0);

}


struct flash_t Get_flash_type (long timeout_secs)
{

    char indata;

    time_t start_t;

    time_t current_t;

    struct flash_t flash_type;

    char response[5];         /* max length of response is 4 */

    flash_type.cmd_response = CMD_UNKNOWN;


    /* assumption first character received is :
    * command complete response, or
    $ command failed response
    */

    start_t = time (NULL);
    do
    {
        indata = a_getc();
        current_t = time (NULL);
    }
    while ((indata != '$') &&
            (indata != '*') &&
            ((current_t - start_t) < timeout_secs));

    if ((current_t - start_t) >= timeout_secs)
    {
        return (flash_type);
    }
    else
    {
        response[0] = indata;
    }


    start_t = time (NULL);
    do
    {
        indata = a_getc();
        current_t = time (NULL);
    }
    while ((indata != '0') &&
            (indata != '1') &&
            (indata != '2') &&
            (indata != '3') &&
            (indata != '4') &&
            (indata != '5') &&
            (indata != '6') &&
            (indata != '7') &&
            ((current_t - start_t) < timeout_secs));

    if ((current_t - start_t) >= timeout_secs)
    {
        return (flash_type);
    }
    else
    {
        response[1] = indata;
    }


    /* did the command pass or fail? */
    if (response[0] == '*')
    {
        flash_type.cmd_response = CMD_COMPLETE;
        switch (response[1])
        {
            case '1':
                flash_type.id = AMD_29F040_ID;
                break;
            case '2':
                flash_type.id = INTEL_28F800T_ID;
                break;
            case '3':
                flash_type.id = INTEL_28F800B_ID;
                break;
            case '4':
                flash_type.id = ATMEL_49F8192A_ID;
                break;
            case '5':
                flash_type.id = ATMEL_49F8192AT_ID;
                break;
            case '6':
                flash_type.id = M29W800DT_ID;
                break;
            case '7':
                flash_type.id = SST_39SF040_ID;
                break;
            default:
                flash_type.id = UNKNOWN_FLASH_ID;
                break;
        }
    }
    else if (response[0] == '$')
    {
        flash_type.cmd_response = CMD_FAILED;
    }
    else
    {
        flash_type.cmd_response = CMD_UNKNOWN;
    }

    return (flash_type);


}

