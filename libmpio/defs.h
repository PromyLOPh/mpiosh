/*
 *  libmpio - a library for accessing Digit@lways MPIO players
 *  Copyright (C) 2002, 2003 Markus Germeier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc.,g 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * uses code from mpio_stat.c by
 * Yuji Touya (salmoon@users.sourceforge.net)
 */

#ifndef _MPIO_DEFS_H_
#define _MPIO_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "usb.h"

typedef unsigned char  BYTE;
typedef char CHAR;
typedef unsigned short WORD;
typedef unsigned int   DWORD;

/* Memory selection */
typedef enum { MPIO_INTERNAL_MEM = 0x01,
	       MPIO_EXTERNAL_MEM = 0x10 } mpio_mem_t;

/* model type */
typedef enum { MPIO_MODEL_DME      = 0x00,
	       MPIO_MODEL_DMG      = 0x01,
	       MPIO_MODEL_DMG_PLUS = 0x02,
	       MPIO_MODEL_DMB      = 0x03,
	       MPIO_MODEL_DMB_PLUS = 0x04,
	       MPIO_MODEL_DMK      = 0x05,
	       MPIO_MODEL_FD100    = 0x06,
	       MPIO_MODEL_FL100    = 0x07,
	       MPIO_MODEL_FY100    = 0x08,
	       MPIO_MODEL_FY200    = 0x09,
	       MPIO_MODEL_VP_01    = 0x0a,
	       MPIO_MODEL_VP_02    = 0x0b,
               MPIO_MODEL_UNKNOWN  = 0x0c } mpio_model_t;

/* USB commands */
typedef enum { GET_VERSION      = 0x01,
	       GET_BLOCK        = 0x02, /* GET_MEGABLOCK */
               PUT_SECTOR       = 0x03,
               DEL_BLOCK        = 0x04, /* DEL_MEGABLOCK */
               GET_SECTOR       = 0x06,
               GET_SPARE_AREA   = 0x07,
               PUT_BLOCK        = 0x08,
	       MMC_GET_BLOCK    = 0x11,
	       MMC_GET_BIG_BLOCK= 0x12,
	       MMC_WRITE_SECTOR = 0x18,
	       MMC_WRITE_CLUSTER= 0x19,
	       PUT_MEGABLOCK    = 0x30,
               MODIFY_FIRMWARE  = 0xa0 } mpio_cmd_t; 

/* file types on internal memory */
typedef enum { FTYPE_CHAN  = 0x00,
               FTYPE_MUSIC = 0x01,
               FTYPE_CONF  = 'C',
	       FTYPE_FONT  = 'F',
               FTYPE_OTHER = 'H',
               FTYPE_MEMO  = 'M',
               FTYPE_WAV   = 'V',
               FTYPE_ENTRY = 'R',
               FTYPE_DIR   = 'D', 
               FTYPE_DIR_RECURSION = 'r', 
	       FTYPE_BROKEN = 'X', /* internal "dummy" type, used when
				      internal FAT is broken */
               FTYPE_PLAIN = '-'} mpio_filetype_t;

/* fixed filenames */
#define MPIO_CONFIG_FILE  "CONFIG.DAT"
#define MPIO_CHANNEL_FILE "FMCONFIG.DAT"
#define MPIO_MPIO_RECORD  "MPIO RECORD"
#define MPIO_FONT_FON     "FONT.FON"

/* type of callback functions */
typedef BYTE (*mpio_callback_t)(int, int) ; 
typedef BYTE (*mpio_callback_init_t)(mpio_mem_t, int, int) ;

/* zone lookup table */
#define MPIO_ZONE_MAX        8 /* 8* 16MB = 128MB */
#define MPIO_ZONE_PBLOCKS 1024 /* physical blocks per zone */
#define MPIO_ZONE_LBLOCKS 1000 /* logical blocks per zone */
typedef DWORD mpio_zonetable_t[MPIO_ZONE_MAX][MPIO_ZONE_PBLOCKS];

#define MPIO_BLOCK_FREE      0xffff
#define MPIO_BLOCK_DEFECT    0xffee
#define MPIO_BLOCK_CIS       0xaaaa 
#define MPIO_BLOCK_NOT_FOUND 0xcccccccc

/* filenames */
#define MPIO_FILENAME_LEN    129
typedef CHAR mpio_filename_t[MPIO_FILENAME_LEN];

#ifndef NULL
#define NULL             0
#endif

#define MPIO_DEVICE "/dev/usb/mpio"
#define MPIO_CHARSET "ISO-8859-15"

#define MPIO_ID3_FORMAT "%p - %t"

#define SECTOR_SIZE      0x200
#define SECTOR_ECC       0x040
#define SECTOR_TRANS     (SECTOR_SIZE + SECTOR_ECC)

#define BLOCK_SECTORS    0x20     /*  0x10  8MB Smartmedia :salmoon */
#define BLOCK_SIZE       (SECTOR_SIZE * BLOCK_SECTORS)
#define BLOCK_TRANS      (BLOCK_SIZE + (SECTOR_ECC * BLOCK_SECTORS))

#define MEGABLOCK_SECTORS 0x100
#define MEGABLOCK_SIZE    (SECTOR_SIZE * MEGABLOCK_SECTORS) 
#define MEGABLOCK_READ    (MEGABLOCK_SIZE + (SECTOR_ECC * MEGABLOCK_SECTORS))
#define MEGABLOCK_WRITE   (MEGABLOCK_SIZE + (0x10 * MEGABLOCK_SECTORS))
#define MEGABLOCK_TRANS_WRITE (BLOCK_SIZE + (0x10 * BLOCK_SECTORS))

#define DIR_NUM          0x10
#define DIR_SIZE         (SECTOR_SIZE*DIR_NUM)
#define DIR_ENTRY_SIZE   0x20

#define SMALL_MEM        0x02

#define CMD_SIZE         0x40

#define INFO_LINE        129

/* error codes */
typedef struct {
  int	id;
  char *msg;
} mpio_error_t;

#define MPIO_OK                          0
#define MPIO_ERR_FILE_NOT_FOUND		-1
#define MPIO_ERR_NOT_ENOUGH_SPACE	-2
#define MPIO_ERR_FILE_EXISTS		-3
#define MPIO_ERR_FAT_ERROR		-4
#define MPIO_ERR_READING_FILE		-5
#define MPIO_ERR_PERMISSION_DENIED	-6
#define MPIO_ERR_WRITING_FILE		-7
#define MPIO_ERR_DIR_TOO_LONG           -8
#define MPIO_ERR_DIR_NOT_FOUND          -9
#define MPIO_ERR_DIR_NOT_A_DIR         -10
#define MPIO_ERR_DIR_NAME_ERROR        -11
#define MPIO_ERR_DIR_NOT_EMPTY         -12
#define MPIO_ERR_DEVICE_NOT_READY      -13
#define MPIO_ERR_OUT_OF_MEMORY         -14
#define MPIO_ERR_INTERNAL              -15
#define MPIO_ERR_DIR_RECURSION         -16
#define MPIO_ERR_FILE_IS_A_DIR         -17
#define MPIO_ERR_USER_CANCEL           -18
#define MPIO_ERR_MEMORY_NOT_AVAIL      -19
/* internal errors, occur when UI has errors! */
#define MPIO_ERR_INT_STRING_INVALID	-101

#define MPIO_USB_TIMEOUT 1000 /* in msec => 1 sec */

/* get formatted information, about the MPIO player */

typedef struct {
  CHAR firmware_id[INFO_LINE];  
  CHAR firmware_version[INFO_LINE];
  CHAR firmware_date[INFO_LINE];

  CHAR model[INFO_LINE];

  CHAR mem_internal[INFO_LINE];
  CHAR mem_external[INFO_LINE];
  
} mpio_info_t;

/* view of the MPIO firmware */
typedef struct {
  /* everything we get from GET_VERSION */
  CHAR id[12];
  CHAR major[3];
  CHAR minor[3];
  CHAR year[5];
  CHAR month[3];
  CHAR day[3];  
} mpio_firmware_t;

/* */
typedef struct {
  DWORD  NumCylinder;
  DWORD  NumHead;
  DWORD  NumSector;
  DWORD  SumSector;
} mpio_disk_phy_t;

/* */

struct mpio_directory_tx {
  CHAR name[INFO_LINE];
  BYTE dir[MEGABLOCK_SIZE];
  
  BYTE *dentry;
    
  struct mpio_directory_tx *prev;
  struct mpio_directory_tx *next;
    
};
typedef struct mpio_directory_tx mpio_directory_t;


/* view of a SmartMedia(tm) card */
typedef struct {
  BYTE		id;
  BYTE		manufacturer;
  WORD		size;		/* MB */    
  BYTE          chips;          /* this is the hack for
                                   MPIO internal representation, because
				   there might be up to four chips involved */
  
  /*  only needed for external SmartMedia cards */
  mpio_disk_phy_t geo;
  
  BYTE cis[SECTOR_SIZE];       
  BYTE mbr[SECTOR_SIZE];	/* Master Boot Record */
  BYTE pbr[SECTOR_SIZE];	/* Partition Boot Record */

  int  pbr_offset;             /* sector offset for the PBR */
  int  fat_offset;             /* sector offset for the FAT */
  int  dir_offset;

  /* these are needed for internal and external cards */
  DWORD  max_cluster;            /* # of clusters actually available */
  DWORD  fat_size;               /* # sectors for FAT */  
  DWORD  fat_nums;               /* # of FATs */
  BYTE * fat;                    /* *real FAT (like in block allocation :-) */

  /* needed for directory support */
  mpio_directory_t *root; /* root directory */
  mpio_directory_t *cdir; /* current directory */

  /* how many physical blocks are available
   * for internal memory is this value equal to max_cluster
   */
  int max_blocks;
  BYTE * spare;

  /* lookup table for phys.<->log. block mapping */
  mpio_zonetable_t zonetable;

  /* version of chips used */
  BYTE version;

  /* special "features" */
  BYTE recursive_directory;

  /* OK sue me, we can also handle the MMCs with this */
  BYTE    mmc;                   /* Added for external MMC memory modules */
  int     mmc_sector_size;       /* Bytes/sector */
  BYTE    mmc_cluster_size;      /* Sectors/Cluster */
  int     mmc_addressing_offset; /* Offset used in addressing - varies for mmc chip size */

} mpio_smartmedia_t;

/* health status of a memory "card" */
typedef struct {
  WORD total;      /* total blocks on "card" */
  WORD spare;      /* (available spare blocks */
  WORD broken;     /* broken blocks */
} mpio_health_single_t;

typedef struct {
  BYTE num;        /* number of chips or zones */
  BYTE block_size; /* block size in KB */
  /* internal: max 4 chips
   * external: max 8 zones (128MB) -> max 8 */
  mpio_health_single_t data[8];
} mpio_health_t;  

/* view of the MPIO-* */
typedef struct {
  CHAR version[CMD_SIZE];
  
  int fd;
  struct usb_bus *usb_busses;
  struct usb_bus *usb_bus;
  struct usb_dev_handle *usb_handle;
  int usb_out_ep;
  int usb_in_ep;
  CHAR *charset;                   /* charset used for filename conversion */

  BYTE id3;                        /* enable/disable ID3 rewriting support */
  CHAR id3_format[INFO_LINE];
  CHAR id3_temp[INFO_LINE];
  
  mpio_firmware_t firmware;  

  mpio_model_t model;

  mpio_smartmedia_t internal;
  mpio_smartmedia_t external;
} mpio_t;

typedef struct {
  mpio_t *m;
  BYTE mem;                        /* internal/external memory */

  DWORD entry;                     /* number of FAT entry */

  /* internal */
  BYTE  i_index;                   /* file index of file to store */
  BYTE  i_fat[16];                 /* internal FAT entry */

  /* external */
  DWORD e_sector;                  /* logical startsector of cluster */

  /* mapping to HW (== block address) */
  DWORD hw_address;                /* 3 bytes block addressing + 1 byte chip */
  
} mpio_fatentry_t;


/* these are copied from:
 * http://www.linuxhq.com/kernel/v2.4/doc/filesystems/vfat.txt.html
 *
 */

typedef struct  { // Short 8.3 names 
  char name[8];          // file name 
  char ext[3];           // file extension 
  unsigned char attr;             // attribute byte 
  unsigned char lcase;		  // Case for base and extension
  unsigned char ctime_ms;         // Creation time, milliseconds
  unsigned char ctime[2];         // Creation time
  unsigned char cdate[2];         // Creation date
  unsigned char adate[2];         // Last access date
  unsigned char reserved[2];	  // reserved values (ignored) 
  unsigned char time[2];          // time stamp 
  unsigned char date[2];          // date stamp 
  unsigned char start[2];         // starting cluster number 
  unsigned char size[4];          // size of the file 
} mpio_dir_entry_t;

typedef struct  { // Up to 13 characters of a long name 
  unsigned char id;               // sequence number for slot 
  char name0_4[10];      // first 5 characters in name 
  unsigned char attr;             // attribute byte
  unsigned char reserved;         // always 0 
  unsigned char alias_checksum;   // checksum for 8.3 alias 
  char name5_10[12];     // 6 more characters in name
  unsigned char start[2];         // starting cluster number
  char name11_12[4];     // last 2 characters in name
} mpio_dir_slot_t;

#ifdef __cplusplus
}
#endif 

#endif /* _MPIO_DEFS_H_ */

