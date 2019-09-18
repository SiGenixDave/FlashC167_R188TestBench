/***************************************************************************
*.b
*  Copyright (c) 2000 DaimlerChrysler Rail Systems (North America) Inc
****************************************************************************
*  Project    : C167 FLASH Programming
*  File Name  : FLASH.H
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
*  16 Jul 2001 D.Smail
*     Changed to account for the Bottom boot Intel FLASH
**************************************************************************/
#define ERR_FLASH_NONE                0
#define ERR_FLASH_PROG                0x1
#define ERR_FLASH_ERASE               0x2
#define ERR_FLASH_CLEAR               0x4
#define ERR_FLASH_ID                  0x8

#define AMD_CMD_WORD_ADDR1            0x10aaaa  /* 0x5555*2 in external range */
#define AMD_CMD_WORD_ADDR2            0x105554  /* 0x2aaa*2 in external range */
#define AMD_CMD_WORD_ADDR3            0x10aaaa  /* 0x5555*2 in external range */
#define AMD_CMD_WORD_ADDR4            0x10aaaa  /* 0x5555*2 in external range */
#define AMD_CMD_WORD_ADDR5            0x105554  /* 0x2aaa*2 in external range */
#define AMD_CMD_WORD_ADDR6            0x10aaaa  /* 0x5555*2 in external range */

#define AMD_UNLOCK_CMD1               0xaaaa
#define AMD_UNLOCK_CMD2               0x5555

#define AMD_READ_RESET                0xf0f0
#define AMD_PROGRAM_CMD               0xa0a0
#define AMD_ERASE_CMD                 0x8080
#define AMD_ERASE_CHIP                0x1010
#define AMD_ERASE_SECTOR              0x3030
#define AMD_MAN_ID                    0x0101
#define AMD_29F040_DEV_ID             0xa4a4
#define SST_MAN_ID					  0xbfbf
#define SST_39SF040_DEV_ID			  0xb7b7

#define INTELLIGENT_ID_CMD            0x9090


#define M29W800_CMD_WORD_ADDR1        0x100aaa  /* 0x0555*2 in external range */
#define M29W800_CMD_WORD_ADDR2        0x100554  /* 0x02aa*2 in external range */
#define M29W800_CMD_WORD_ADDR3        0x100aaa  /* 0x0555*2 in external range */
#define M29W800_CMD_WORD_ADDR4        0x1000aa  /* 0x0055*2 in external range */


#define M29W800_DEV_ID				  0x22D7


#define INTEL_CLEAR_REGISTER          0x0050
#define INTEL_ERASE_CMD               0x0020
#define INTEL_ERASE_CONFIRM           0x00D0
#define INTEL_READ_STATUS_REGISTER    0x0070
#define INTEL_READ_ARRAY              0x00ff
#define INTEL_PROGRAM_CMD             0x0040
#define INTEL_MAN_ID                  0x0089
#define INTEL_28F800T_DEV_ID          0x889C
#define INTEL_28F800B_DEV_ID          0x889D
#define INTEL_ID_CMD                  0x0090

#define ATMEL_MAN_ID                  0x001F
#define ATMEL_49F8192A_DEV_ID         0x00A0
#define ATMEL_49F8192AT_DEV_ID        0x00A3


#define NUM_OF_SECTORS                8


#define U_BIT_7                       0x8000
#define U_BIT_5                       0x2000
#define U_BIT_4                       0x1000
#define U_BIT_3                       0x0800
#define U_BIT_54                      0x3000
#define L_BIT_7                       0x0080
#define L_BIT_5                       0x0020
#define L_BIT_4                       0x0010
#define L_BIT_3                       0x0008
#define L_BIT_54                      0x0030

#define FLASH_EPROM_START             0x100000
#define FLASH_EPROM_SIZE              0x3ffff         /* 16-bit words */
#define TOTAL_FLASH_EPROM_BYTES       0x100000        /* for 2 devices */
#define FLASH_SECTOR_SIZE             0x10000         /* 64K + 1 */



