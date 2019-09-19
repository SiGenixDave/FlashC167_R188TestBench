/***************************************************************************
*.b
*  Copyright (c) 2000-2013 Bombardier Transportation
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : monitor.c
*  Subsystem  : PC (MS-DOS)
*  Procedures : main
*               Decode_ascii_hex_out
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

typedef void(__stdcall *InitComFnPtr) (unsigned int comPort, unsigned int baudRate);

static InitComFnPtr InitComPtr;

// Allow DLL to call .NET functions
__declspec (dllexport) void __stdcall SetInitComCallback(InitComFnPtr func)
{
	InitComPtr = func;
}

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: main
*
*  ABSTRACT:
*     Communicate with the C167 while in bootstrap mode for the purposes
*   of programming on-board FLASH
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
*        argc         int,        number of command line arguments
*        argv[]       char *,     array of pointers to command line arguments
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*         0 , 1       char,       DOS error code
*
*  FUNCTIONAL DESCRIPTION:
*     This function is invoked when the user executes the "flash.exe" from an
*   MS-DOS command line. Several checks are performed to verify the user
*   supplied the correct number of command-line arguments. The requested
*   comport is opened.  It then verifies that files necessary to perform
*   the download task can be opened. The functions responsible for communicating
*   downloading the bootstrap code and then downloading and programming the
*   application code are then called.
*
* .b
*
* History :
*  01 Apr 2000 D.Smail
*     Created
* Revised :
*  17 Jul 2001 D.Smail
*    Modified copyright name from Adtranz to Bombardier
******************************************************************************/
__declspec (dllexport) int FlashMain(int argc, char *argv[])
{
	struct file_info_t files;

	time_t start_t;		/* start time of download (elapsed seconds from 1/1/1970) */
	time_t end_t;		/* end time of download (elapsed seconds from 1/1/1970) */
	time_t elapsed_t;	/* difference between end_t and start_t */

	unsigned elapsed_min; /* number of minutes to download data */
	unsigned elapsed_sec; /* number of seconds to download data */

	int rc;					/* result from the boot load and application download */

	int baud_rate;					/* baud rate of data transfer */

	int ignore_crc_file = FALSE;	/* determines if to check for CRC configuration file */
	int ignore_crc_input = FALSE;	/* determines if to check for CRC on the
									   command line */

	char *file_ext;

	char crc_string[10];

	struct user_info_t user_info;
	FILE *f_network_log;
	FILE *f_local_log;
	char *info_a;
	char *info_b;
	char *info_c;

	char comPort;

	/* Get the time (program start); used later only for user info */
	start_t = time(NULL);

	printf("\n ******************************************************************************");
	printf("\n                 Bombardier Transportation USA Inc. (c) 2014-2019              ");
	printf("\n                    C167 Flash Programmer Version (Release 2.0)                ");
	printf("\n                               R188 BTE Specific                               ");
	printf("\n ******************************************************************************");
	printf("\n");

	/* Verify valid number of command line arguments */
	if (argc > 6 || argc < 2)
	{
		printf("\tUsage is: FlashC167 <comport> <IntelHexFilename> <CRC config file> <baud_code> <reset> \n");
		return (1);
	}

	files.reset = 0;
	/* Determine if reset is to be issued at the end of the programming sequence */
	if (argc > 2)
	{
		int count = 2;
		while (count < argc)
		{
			if (!strcmp(argv[count], "reset"))
			{
				ignore_crc_input = TRUE;
				files.reset = 1;
				break;
			}
			else
			{
				count++;
			}
		}
	}

	/* Set the default baud rate at 38400 to communicate to the boot loader */
	baud_rate = 38400;

	/* Determine if the baud rate needs to be changed */
	if (argc > 2)
	{
		int count = 2;
		while (count < argc)
		{
			if (!strcmp(argv[count], "19200"))
			{
				baud_rate = 19200;
				break;
			}
			else if (!strcmp(argv[count], "9600"))
			{
				baud_rate = 9600;
				break;
			}
			else if (!strcmp(argv[count], "4800"))
			{
				baud_rate = 4800;
				break;
			}
			else if (!strcmp(argv[count], "2400"))
			{
				baud_rate = 2400;
				break;
			}
			else
			{
				count++;
			}
		}
	}

	/* Get the comm port which is always the first argument after "flashC167" */
	comPort = (*argv[0]) - '0';

	/* Initialize the Comport */
	if ((comPort >= 1) && (comPort <= 9))
	{
		InitComPtr(comPort, baud_rate);
	}
	else
	{
		printf("\tInvalid Com Port: Valid options are comports 1 through 9\n");
		return (2);
	}

	printf(" ** Serial port %d set to baud rate %d **\n", comPort, baud_rate);

	files.commandline_crc = FALSE;
	files.f_config_crc = NULL;

	/* Set up file pointer for CRC configuration file */
	if (argc >= 3)
	{
		int count = 2;
		while (count < argc)
		{
			files.f_config_crc = (FILE *)fopen(argv[count], "r");
			if (files.f_config_crc != (FILE *)NULL)
			{
				break;
			}
			count++;
		}
	}

	if (files.f_config_crc == NULL)
	{
		printf(" ** CRC configuration file not detected... FLASH program will not be verified **\n\n");
	}

	/* Verify file exists */
	files.f_flashapp_hex = (FILE *)fopen(argv[1], "r");
	if (files.f_flashapp_hex == (FILE *)0)
	{
		/* unable to open file */
		printf("** Unable to open hex file %s \n", argv[1]);
		return (3);
	}

	/* convert string name to all lower case for ".hex" check below*/
	strlwr(argv[1]);
	/* Verify file extension is ".hex" */
	strcpy(files.flashapp_hex_name, argv[1]);
	file_ext = strstr(files.flashapp_hex_name, ".hex");
	if (file_ext == NULL)
	{
		printf("** Unable to open boot download file %s \n",
			files.flashapp_hex_name);

		return (4);
	}
	else
	{
		strcpy(file_ext, ".167");
	}

	/* Load the boot loader files in RAM */
	rc = Boot_strap_loader_monitor();
	if (rc)
	{
		printf("\n**** Report Error code %d ", (rc + 100));
		printf("\n**** Program aborted.\n");
		return (rc + 100);
	}

	/* Load the application file in RAM; then program to FLASH */
	rc = Flash_monitor(files, crc_string);
	if (rc)
	{
		printf("\n**** Report Error code %d ", (rc + 200));
		printf("\n**** Program aborted.\n");
		return (rc + 200);
	}

	/* Get the time (program end); used later only for user info */
	end_t = time(NULL);

	elapsed_t = end_t - start_t;

	/* Display the amount of time to download the application in minutes and
	seconds */
	elapsed_min = 0;
	if (elapsed_t > 59)
	{
		elapsed_min = (unsigned)(elapsed_t / 60);
	}
	elapsed_sec = (unsigned)(elapsed_t % 60);
	printf("\n\t> Total Download Time... %2u min   %2u sec\n",
		elapsed_min, elapsed_sec);

	/* Create log files in the working directory as well as the network */
	if (files.f_config_crc != NULL)
	{
		/* Flush another line */
		fgets(user_info.project, 100, files.f_config_crc);
		/* Get user info (if there) */
		info_a = fgets(user_info.project, 100, files.f_config_crc);
		info_b = fgets(user_info.pcb, 100, files.f_config_crc);
		info_c = fgets(user_info.name, 100, files.f_config_crc);

		/* Verify user info appended in configuration file */
		if ((info_a != NULL) && (info_b != NULL) && (info_c != NULL))
		{
			/* Strip "/n" from the string */
			sscanf(user_info.project, "%s", user_info.project);
			sscanf(user_info.pcb, "%s", user_info.pcb);
			sscanf(user_info.name, "%s", user_info.name);
			/* get the current date and time */

			/* Update the log file */
			if ((f_network_log = fopen("p:\\users\\smail\\flashlog\\flash.log", "a")) != NULL)
			{
				fprintf(f_network_log, "%10s %10s %10s %10s ", user_info.project,
					user_info.pcb,
					user_info.name,
					crc_string);
				fprintf(f_network_log, "%2u %2u\n", elapsed_min, elapsed_sec);
				fclose(f_network_log);
			}
			if ((f_local_log = fopen("flash.log", "a")) != NULL)
			{
				fprintf(f_local_log, "%10s %10s %10s %10s ", user_info.project,
					user_info.pcb,
					user_info.name,
					crc_string);
				fprintf(f_local_log, "%2u %2u\n", elapsed_min, elapsed_sec);
				fclose(f_local_log);
			}
		}
	}

	return (0);
}

/*****************************************************************************
*
* .b
*
*  PROCEDURE NAME: ASCII_nibble_to_binary_nibble
*
*  ABSTRACT:
*     Converts an ASCII character (0-9,A-F) to a binary nibble
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
*        ascii_in       char,       ASCII character to be converted to binary
*
*  OUTPUTS:
*
*     Global Variables:
*        None
*
*     Returned Value:
*        ret_val       char,       binary value of passed value ascii_in
*
*  FUNCTIONAL DESCRIPTION:
*     This function converts an ASCII character (0-9, A-F) to a hexadecimal
*   nibble in binary. The ASCII character passed to this function is returned
*   as a hexadecimal nibble.
*
* .b
*
* History :
*  01 Apr 2000 D.Smail
*     Created
* Revised :
******************************************************************************/
unsigned char ASCII_nibble_to_binary_nibble(char ascii_in)
{
	unsigned char ret_val;  /* value returned from function (binary) */

	if ((ascii_in >= '0') & (ascii_in <= '9'))
	{
		ret_val = ascii_in - '0';
	}
	else
	{
		if ((ascii_in >= 'A') && (ascii_in <= 'F'))
		{
			ret_val = ascii_in - 'A' + 10;
		}
		else
		{
			if ((ascii_in >= 'a') && (ascii_in <= 'f'))
			{
				ret_val = ascii_in - 'a' + 10;
			}
			else
			{
				ret_val = ascii_in;
			}
		}
	}

	return (ret_val);
}