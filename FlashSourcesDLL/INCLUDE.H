/***************************************************************************
*.b
*  Copyright (c) 2000-2013 Bombardier Transportation
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : include.h
*  Subsystem  : PC (MS-DOS)
*  Procedures : N/A
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
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <time.h>

/* DEFINES */
#define  COLON                            0x3a
#define  DATA_RECORD                      0x00
#define  END_RECORD                       0x01
#define  ESA_RECORD                       0x02
#define  SSA_RECORD                       0x03
#define  ELA_RECORD                       0x04
#define  SLA_RECORD                       0x05
#define  HEX_OK                           0
#define  HEX_BAD                          1
#define  MAX_BYTES_IN_DATA_RECORD         40
#define  MAX_SIZE_OF_ROW_IN_HEX_FILE      100
#define  EPROM_SIZE                       0x100000

#define  BAUD_115200                      1
#define  BAUD__57600                      2
#define  BAUD__38400                      3
#define  BAUD__19200                      6
#define  BAUD___9600                      12
#define  BAUD___4800                      24
#define  BAUD___2400                      48

#define  ECHO_TIMEOUT                     1
#define  INVALID_ECHO                     2

/* C167 boot strap CPU codes encountered so far */
#define  CPU_CODE_1                       0xa5
#define  CPU_CODE_2                       0xc5
#define  CPU_CODE_3                       0xd5

#define  CMD_COMPLETE                     0x00
#define  CMD_FAILED                       0x01
#define  CMD_UNKNOWN                      0x02

#define  FALSE                            0
#define  TRUE                             1

#define BOOT_SERIAL_PORT_TIMEOUT_SECONDS  3
#define FLASH_SERIAL_PORT_TIMEOUT_SECONDS 3
#define FLASH_COMMAND_TIMEOUT_SECONDS     20

#define     GET_TOTAL_BYTES               0
#define     SEND_BLOCK_ADDRESS_AND_SIZE   1
#define     SEND_BLOCK_PROGRAM_DATA       2

#define     UNKNOWN_FLASH_ID              0
#define     AMD_29F040_ID                 1
#define     INTEL_28F800T_ID              2
#define     INTEL_28F800B_ID              3
#define     ATMEL_49F8192A_ID             4
#define     ATMEL_49F8192AT_ID            5
#define     M29W800DT_ID				  6
#define     SST_39SF040_ID				  7

/* STRUCTURES */
struct file_info_t
{
	FILE *f_flashapp_hex;
	FILE *f_config_crc;
	char flashapp_hex_name[300];
	unsigned long commandline_crc;
	char reset;
};

struct user_info_t
{
	char project[100];
	char pcb[100];
	char name[100];
};

struct echo_t
{
	char error_code;
	char echo;
};

struct flash_t
{
	int cmd_response;
	int id;
};

/* PROTOTYPES */
char *GetStage1(void);
char *GetStage2(void);
char *GetStage3(void);

unsigned char Parse_hex_file(char *cp, FILE *fp, FILE *out, unsigned char write_header_info);

struct echo_t Send_byte_wait_for_echo(unsigned int send_byte,
	time_t timeout);

unsigned char ASCII_nibble_to_binary_nibble(char ascii_in);

int Wait_for_command_reponse(const char *cmd,
	long timeout);

unsigned int Parse_hex_record(char *ptr);

unsigned int Get_extended_linear_address(char *ptr,
	unsigned int *error);

unsigned int Get_segment_offset(char *ptr,
	unsigned int *no_of_bytes,
	unsigned char *byte_array,
	unsigned int *error);

int Flash_monitor(struct file_info_t files,
	char *crc_string);

int Boot_strap_loader_monitor(void);

unsigned char ASCII_nibbles_to_binary_byte(char hi_nibble_ascii,
	char lo_nibble_ascii);

unsigned char Get_crc_from_logic(unsigned char crc_width,
	char *crc_string,
	struct file_info_t files);

struct flash_t Get_flash_type(long timeout_secs);

void a_putc(char tx);
int a_getc(void);