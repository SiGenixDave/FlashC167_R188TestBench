/***************************************************************************
*.b
*  Copyright (c) 2000 DaimlerChrysler Rail Systems (North America) Inc
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : INCLUDE.H
*  Subsystem  : Third stage boot loader
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

#define     START_OF_DOWNLOAD_SRAM      0x210000

#define     NUM_FLASH_SECTORS           4


#define     MSB_OF_MSW_FLASH_ADDRESS    0
#define     LSB_OF_MSW_FLASH_ADDRESS    1
#define     MSB_OF_LSW_FLASH_ADDRESS    2
#define     LSB_OF_LSW_FLASH_ADDRESS    3
#define     MSB_OF_BLOCK_SIZE           4
#define     LSB_OF_BLOCK_SIZE           5

#define     FLASH_IS_UNKNOWN            0
#define     FLASH_IS_AMD_29F040         1
#define     FLASH_IS_INTEL_28F800T      2

typedef enum
{
    WAIT_FOR_COMMAND,
    GET_BYTE_TOTAL,
    DOWNLOAD_BLOCK_CODE,
    UNKNOWN_FLASH_ID,
    AMD_29F040_ID,
    INTEL_28F800T_ID,
    INTEL_28F800B_ID,
    ATMEL_49F8192A_ID,
    ATMEL_49F8192AT_ID,
    M29W800DT_DEV_ID,
    SST_39SF040_ID,
    FLASH_ERASE_SUCCESS,
    FLASH_ERASE_ERROR,
    FLASH_PROGRAM_SUCCESS,
    FLASH_PROGRAM_ERROR,
    CRC_PASS_STATE,
    CRC_FAIL_STATE,
    DOWNLOAD_COMPLETE,
    DOWNLOAD_ERROR,
    RESET_COMPLETE,
    RESET_ERROR,
    UNKNOWN_COMMAND
} State_t;


/* Structures */
typedef struct
{
    UINT_8   width;   /* received from PC ( 8, 16, or 32 ) */
    UINT_32  address; /* received from PC; flash address where CRC is stored */
} CRC_t;

struct interface_data_t
{
    UINT_16 huge *amd_addr_1; /* address for a FLASH command (write,erase) */
    UINT_16 huge *amd_addr_2; /* address for a FLASH command (write,erase) */
    UINT_16 huge *amd_addr_3; /* address for a FLASH command (write,erase) */
    UINT_16 huge *amd_addr_4; /* address for a FLASH command (write,erase) */
    UINT_16 huge *amd_addr_5; /* address for a FLASH command (write,erase) */
    UINT_16 huge *amd_addr_6; /* address for a FLASH command (write,erase) */

    UINT_16 huge *intel_sector_addr;

    UINT_32 total_bytes;        /* received from PC; total number of bytes to expect

                                 from downloading application code */

    UINT_32 reset;

    CRC_t   crc;
    UINT_16 device_type;        /* AMD 29040 or Intel 28F800B */
};


/* Prototypes */
/* serial.c */
UINT_8  io_getbyte (void);
void    io_putbyte (UINT_8);

/* crc.c */
UINT_8  Calc_crc (struct interface_data_t *);
void    Get_crc_from_flash (struct interface_data_t *);
UINT_8 ResetPCB (struct interface_data_t *globs);
UINT_8  Binary_byte_to_ASCII (UINT_8);


/* blkdata.c */
void    Download_block_data (struct interface_data_t *);
void    Get_total_bytes (struct interface_data_t *);

/* main.c */
void    main (void);
State_t Get_command (struct interface_data_t *);

/* flash.c */
void    Init_flash_pointers (struct interface_data_t *);
State_t Erase_flash (struct interface_data_t *);
State_t Program_flash (struct interface_data_t *);
UINT_16 Erase_flash_chip (struct interface_data_t *);
State_t Autoselect_flash (struct interface_data_t *);
UINT_16 Erase_INTEL28F800 (struct interface_data_t *globs);
UINT_16 Erase_AMD29040 (struct interface_data_t *globs);
UINT_16 Erase_M29W800 (struct interface_data_t *globs);


