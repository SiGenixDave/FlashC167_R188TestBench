/***************************************************************************
*.b
*  Copyright (c) 2000-2013 Bombardier Transportation
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : parsehex.c
*  Subsystem  : PC (MS-DOS)
*  Procedures : Parse_hex_file
*               Parse_hex_record
*               Get_extended_linear_address
*               Get_segment_offset
*
*
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
*  13 Feb 2001 D.Smail
*    Modified Parse_hex_file() to support C1666r10 ihex166.exe
**************************************************************************/

#include "include.h"



char *ParseStringGets (char * str, int num, char **input)
{
    char *next = *input;
    int  numread = 0;

    while (numread + 1 < num && *next)
    {
        int isnewline = (*next == '\n');
        *str++ = *next++;
        numread++;
        // newline terminates the line but is included
        if (isnewline)
            break;
    }

    if (numread == 0)
        return NULL;  // "eof"

    // must have hit the null terminator or end of line
    *str = '\0';  // null terminate this tring
    // set up input for next call
    *input = next;
    return str;
}

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Parse_hex_file
*
*  ABSTRACT:
*     Converts Intel Hex file to TXT file used for C167 FLASH programming
*
*  INPUTS:
*
*     Globals:
*       None
*
*     Constants:
*       DATA_RECORD
*       HEX_OK
*       END_RECORD
*       SEEK_END
*       HEX_BAD
*       ESA_RECORD
*       SSA_RECORD
*       ELA_RECORD
*       SLA_RECORD
*       END_RECORD
*       TRUE
*       FALSE
*
*     Procedure Parameters:
*       fp                  FILE *
*       out                 FILE *
*       write_header_info   unsigned char   TRUE if block address, size and /n
*                                           are to be written
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
*  01 Apr 2000 D.Smail
*     Created
* Revised :
*  13 Feb 2001 D. Smail
*     Modified because the C1666r10 ihex166.exe utility generates 2 consecutive
*     ELA records. Not sure why, but a fix was added to support this
*     idiosyncrasy.
*
*
******************************************************************************/
unsigned char Parse_hex_file (char *cp, FILE *fp, FILE *out, unsigned char write_header_info)
{

    unsigned int num_bytes_in_rec;  /* number of bytes in record; used to write
									bytes to text file and terminate line  */

    char hex_buf[MAX_SIZE_OF_ROW_IN_HEX_FILE];  /* string that stores current
												Intel Hex file line to be
												parsed */

    unsigned char data_buf[MAX_BYTES_IN_DATA_RECORD]; /* data (application
													  program ) bytes from
													  Intel Hex file; written
													  to TXT file */

    unsigned int address_segment; /* 16 bit address segment (MSW) */
    unsigned int address_offset;  /* 16 bit address offset  (LSW) */

    unsigned char first_offset_after_segment;

    unsigned long total_num_of_bytes;

    unsigned long num_of_bytes_in_block;

    long int jump_file_position;

    unsigned long num_of_records_in_block;

    unsigned char first_ela_record;

    unsigned int err_code;

    unsigned int record_type;

    unsigned int previous_record_type;

    unsigned int i;

    int rc;

    unsigned char failure_flag;

    unsigned char resourceFile = FALSE;

    total_num_of_bytes = 0;
    num_of_bytes_in_block = 0;
    num_of_records_in_block = 0;
    first_ela_record = TRUE;

    first_offset_after_segment = FALSE;
    failure_flag = FALSE;
    address_offset = 0;
    address_segment = 0;
    previous_record_type = DATA_RECORD;

    if (cp != NULL)
    {
        resourceFile = TRUE;
    }


    if (write_header_info == TRUE)
    {
        /* this will be overwritten after the entire hex file has been parsed */
        fprintf (out, "%08lX", total_num_of_bytes);
    }

    do
    {

        if (resourceFile == TRUE)
        {
            /* read the next record in the Intel Hex file */
            ParseStringGets (hex_buf, 100, &cp);

            if (cp == NULL)
            {
                break;
            }
        }
        else
        {
            /* read the next record in the Intel Hex file */
            fgets (hex_buf, 100, fp);
        }

        /* determine the record type */
        record_type = Parse_hex_record (hex_buf);

        switch (record_type)
        {
            case DATA_RECORD:
                /* Get address offset; used only after segment record detected to
                make a 32 bit address */
                address_offset = Get_segment_offset (hex_buf, &num_bytes_in_rec, data_buf, &err_code);
                if (err_code == HEX_OK)
                {
                    if (write_header_info == TRUE)
                    {
                        if (first_offset_after_segment == TRUE)
                        {
                            /* used as an indicator that a valid segment was found */
                            first_offset_after_segment = FALSE;
                            /* Store the 32 bit address in the text file as well as the
                            number of bytes detected in the Intel Hex block (number
                            of bytes before next segment detected) */
                            rc = fprintf (out, "\n%04X", address_segment);
                            total_num_of_bytes += 2;
                            rc = fprintf (out, "%04X", address_offset);
                            total_num_of_bytes += 2;
                            rc = fprintf (out, "%04X", num_bytes_in_rec);
                            total_num_of_bytes += 2;
                        }
                        rc = fprintf (out, "\n");
                        /* account for \n while jumping back to bookmark */
                        num_of_records_in_block++;

                        /* Write the data bytes to the file */
                        for (i = 0; i < num_bytes_in_rec; i++)
                        {
                            /* write to output file */
                            rc = fprintf (out, "%02X", data_buf[i]);
                            total_num_of_bytes++;
                            num_of_bytes_in_block++;
                        }
                    }
                    else
                    {
                        /* no header info required in txt file */
                        for (i = 0; i < num_bytes_in_rec; i++)
                        {
                            /* write to output file */
                            rc = fprintf (out, "%02X", data_buf[i]);
                        }
                    }
                }   /* end else no error in hex record */
                else
                {
                    rc = fprintf (out, " \n !!! Error (2) in hex file !!!");
                    rc = printf (" \n !!! Error (2) in hex file !!!");
                }
                previous_record_type = DATA_RECORD;
                break;

            case END_RECORD:
                if (write_header_info == TRUE)
                {
                    /* update block size of last block */
                    /* go back and overwrite the size of the previous block
                    before resetting for the next block */
                    jump_file_position = num_of_records_in_block +
                                         (num_of_bytes_in_block + 2);
                    jump_file_position *= -2;
                    rc = fseek (out, jump_file_position, 1);
                    if (rc == 0)
                    {
                        rc = fprintf (out, "%04X", num_of_bytes_in_block);
                        /* go forward to end of file for next block */
                        rc = fseek (out, 0, SEEK_END);
                    }
                    else
                    {
                        err_code = HEX_BAD;
                        return (1);
                    }

                    rc = fprintf (out, "\n");
                    /* print the total number of bytes at the start of the file */
                    /* seek to the beginning of the file */
                    rc = fseek (out, 0L, SEEK_SET);
                    total_num_of_bytes += 4;
                    fprintf (out, "%08lX", total_num_of_bytes);
                }
                previous_record_type = END_RECORD;
                break;

            case ESA_RECORD:
                err_code = HEX_BAD;
                rc = fprintf (out, " \n !!! Error (4) in hex file !!!");
                rc = printf (" \n !!! Error (4) in hex file !!!");
                previous_record_type = ESA_RECORD;
                break;

            case SSA_RECORD:
                err_code = HEX_BAD;
                rc = fprintf (out, " \n !!! Error (5) in hex file !!!");
                rc = printf (" \n !!! Error (5) in hex file !!!");
                previous_record_type = SSA_RECORD;
                break;

            case ELA_RECORD:
                /* DAS added if{} 02/13/2001:
                R Zentner created a condition where 2 ESA segments are back
                to back and identical with the C167 6.0 compiler. Don't understand
                why the ihex166.exe does this, but that's why this if statement
                is there. */

                if (previous_record_type != ELA_RECORD)
                {
                    address_segment = Get_extended_linear_address (hex_buf,
                                      &err_code);
                    if (err_code == HEX_OK)
                    {
                        first_offset_after_segment = TRUE;
                        if (write_header_info == TRUE)
                        {
                            if (first_ela_record == FALSE)
                            {
                                /* go back and overwrite the size of the previous block
                                before resetting for the next block */
                                jump_file_position = num_of_records_in_block +
                                                     (num_of_bytes_in_block + 2);
                                jump_file_position *= -2;
                                rc = fseek (out, jump_file_position, 1);
                                if (rc == 0)
                                {
                                    rc = fprintf (out, "%04X", num_of_bytes_in_block);
                                    /* go forward to end of file for next block */
                                    rc = fseek (out, 0, SEEK_END);
                                }
                                else
                                {
                                    err_code = HEX_BAD;
                                    return (1);
                                }
                            }
                        }
                        else
                        {
                            /* no header info required, do nothing */
                        }
                        num_of_bytes_in_block = 0;
                        first_ela_record = FALSE;
                        jump_file_position = 0;
                        num_of_records_in_block = 0;
                    }
                    else
                    {
                        rc = fprintf (out, " \n !!! Error (3) in hex file !!!");
                        rc = printf (" \n !!! Error (3) in hex file !!!");
                    }
                }
                previous_record_type = ELA_RECORD;
                break;
            case SLA_RECORD:
                err_code = HEX_BAD;
                rc = fprintf (out, " \n !!! Error (6) in hex file !!!");
                rc = printf (" \n !!! Error (6) in hex file !!!");
                previous_record_type = SLA_RECORD;
                break;
            default:
                rc = printf (" \n !!! Error (7): Not a hex file !!!");
                failure_flag = TRUE;
                break;
        }

    }
    while ((record_type != END_RECORD) && (failure_flag == FALSE));

    return (failure_flag);

}



/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Parse_hex_record
*
*  ABSTRACT:
*     Extracts the RECORD type from the current string
*
*  INPUTS:
*
*     Globals:
*       None
*
*     Constants:
*       COLON
*
*     Procedure Parameters:
*       ptr     unsigned char *       pointer to the current string in HEX file
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*       ret_val    unsigned int,   data record type; 0xff if ':' not present
*
*  FUNCTIONAL DESCRIPTION:
*     This function is responsible for returning to the calling function
*   the record type for the line in the scanned Intel Hex file. A check is
*   also performed to verify that this is a valid Intel Hex line
*   (verification first character is ':').
*
*
* .b
*
* History :
*  01 Apr 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
unsigned int Parse_hex_record (char *ptr)
{

    unsigned int ret_val;     /* return value from function */

    /* Init to invalid record type */
    ret_val = 0xff;

    /* Verify first character on the line is a ':' */
    if (*ptr == COLON)
    {
        /* Combine the 7th and 8th character (not including the ':') on the line
        into a binary value to form the record type. */
        ret_val = ASCII_nibbles_to_binary_byte (* (ptr + 7) , * (ptr + 8));
    }

    return (ret_val);

}



/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Get_extended_linear_address
*
*  ABSTRACT:
*     Gets the 16 bit segment address
*
*
*  INPUTS:
*
*     Globals:
*       None
*
*     Constants:
*       HEX_OK
*       HEX_BAD
*
*     Procedure Parameters:
*       ptr     unsigned char *         pointer to current string to be parsed
*       error   unsigned int *          HEX_OK, HEX_BAD
*
*  OUTPUTS:
*
*     Global Variables:
*       None
*
*     Returned Value:
*       unsigned int                    segment address
*
*  FUNCTIONAL DESCRIPTION:
*       This function extracts the address segment from the current string
*     and returns it to the calling function. Verification that the string
*     is a valid Intel hex string is also performed.
*
* .b
*
* History :
*  01 Apr 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
unsigned int Get_extended_linear_address (char *ptr,
        unsigned int *error)
{
    unsigned char lo_byte;  /* stores the low byte of the address segment */
    unsigned char hi_byte;  /* stores the high byte of the address segment */
    unsigned int segment;   /* segment address; returned value */

    unsigned char byte_1;   /* used in verification of a segment record line */
    unsigned char byte_2;   /* used in verification of a segment record line */
    unsigned char byte_3;   /* used in verification of a segment record line */

    if (*ptr++ != ':')
    {
        /* valid record not found */
        segment = 0;
        *error = HEX_BAD;
        return (segment);
    }

    /* get the next byte */
    byte_1 = ASCII_nibbles_to_binary_byte (*ptr , * (ptr + 1));
    ptr += 2;

    /* Verify a 02 */
    if (byte_1 == 0x02)
    {
        byte_2 = ASCII_nibbles_to_binary_byte (*ptr , * (ptr + 1));
        ptr += 2;

        byte_3 = ASCII_nibbles_to_binary_byte (*ptr , * (ptr + 1));
        ptr += 2;

        if ((byte_2 == 0x00) && (byte_3 == 0x00))
        {
            /* address field is 0x0000, get the segment */
            ptr += 2; /* skip the record type (0x04) */
            hi_byte = ASCII_nibbles_to_binary_byte (*ptr , * (ptr + 1));
            ptr += 2;
            lo_byte = ASCII_nibbles_to_binary_byte (*ptr , * (ptr + 1));
            ptr += 2;

            segment = (hi_byte << 8) | lo_byte;
            *error = HEX_OK;
        }
        else
        {
            /* valid record not found */
            segment = 0;
            *error = HEX_BAD;
        }
    }
    else
    {
        /* valid record not found */
        segment = 0;
        *error = HEX_BAD;
    }

    return (segment);

} 


/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: Get_segment_offset
*
*  ABSTRACT:
*     Return address offset and ASCII data bytes in a binary array
*
*
*  INPUTS:
*
*     Globals:
*       None
*
*     Constants:
*       COLON
*       DATA_RECORD
*       HEX_OK
*       HEX_BAD
*
*     Procedure Parameters:
*       ptr           char *
*       num_of_bytes  unsigned int *
*       byte_array    unsigned char *
*       error         unsigned int *
*
*  OUTPUTS:
*
*     Global Variables:
*       None
*
*     Returned Value:
*
*  FUNCTIONAL DESCRIPTION:
*     Function is used to generate the 16 bit offset address after a data
*   record is encountered in the Intel Hex file. It also places all data
*   bytes from that offset into "byte_array". Verification that the line
*   is a valid Intel Hex Data Record line is also performed.
*
*
* .b
*
* History :
*  01 Apr 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
unsigned int Get_segment_offset (char *ptr,
                                 unsigned int  *num_of_bytes,
                                 unsigned char *byte_array,
                                 unsigned int  *error)
{
    unsigned int byte_count;  /* stores the number of data bytes on the line */
    unsigned char lo_byte;    /* stores the low byte of the address offset */
    unsigned char hi_byte;    /* stores the high byte of the address offset */
    unsigned char data_byte;  /* temp storage of the data byte as the line is
							  scanned; placed in byte array */
    unsigned char record_byte;  /* stores the RECORD TYPE of the line */
    unsigned int ret_val; /* address offset */
    unsigned int i;       /* used for looping */

    /* Verify 1st character of the record is a ':' */
    if (*ptr++ == COLON)
    {
        /* determine the number of data bytes on the line */
        byte_count = ASCII_nibbles_to_binary_byte (*ptr , * (ptr + 1));
        ptr += 2;

        /* Create the 16 bit offset address */
        hi_byte = ASCII_nibbles_to_binary_byte (*ptr , * (ptr + 1));
        ptr += 2;
        lo_byte = ASCII_nibbles_to_binary_byte (*ptr , * (ptr + 1));
        ptr += 2;
        ret_val = (hi_byte << 8) | lo_byte;

        /* Verify that the line is a DATA_RECORD */
        record_byte = ASCII_nibbles_to_binary_byte (*ptr , * (ptr + 1));
        if (record_byte != DATA_RECORD)
        {
            *error = HEX_BAD;
            return (0);
        }
        /* good record */
        else
        {
            ptr += 2;
            *error = HEX_OK;
            *num_of_bytes = byte_count;
            if (byte_count > MAX_BYTES_IN_DATA_RECORD)
            {
                *error = HEX_BAD;
                return (0);
            }
            /* Place in byte array all data from the line */
            for (i = 0; i < byte_count; i++)
            {
                data_byte = ASCII_nibbles_to_binary_byte (*ptr , * (ptr + 1));
                * (byte_array + i) = data_byte;
                ptr += 2;
            }
        }   /* end else good record */
        return (ret_val);
    }
    /* ':' not found as first character on the line */
    else
    {
        *error = HEX_BAD;
        return (0);
    }
}

