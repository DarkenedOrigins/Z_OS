//for filesystem header file
#ifndef _FS_H
#define _FS_H
#include "types.h"
#include "lib.h"
#include "paging.h"


#define fs_page_size            (4096)
#define BOOT_BLOCK_RESERVED     (52)
#define BOOT_BLOCK_ENTRIES      (63)
#define	BOOT_BLOCK_FIRST_HALF	(64)
#define DENTRY_RESERVED         (24)
#define INODE_DATA_LEN          (1023)
#define FILENAME_LEN            (32)
#define FOUR_KBYTES             (4096)
#define DENTRY_TYPE_RTC         (0)
#define DENTRY_TYPE_DIRECTORY   (1)
#define DENTRY_TYPE_FILE        (2)
#define MAX_ASCII_LEN_32        (11)
#define BASE_10                 (10)
#define FOUR_MBYTES             (FOUR_KBYTES * 1024)
#define PROGRAM_PAGE            (FOUR_MBYTES*32)
#define PROGRAM_OFFSET          (0x48000)


enum file_type{
    f_type_rtc,
    f_type_dir,
    f_type_reg_file
};

typedef struct {
    uint32_t length;
    uint32_t data_block_num[INODE_DATA_LEN];
} inode_t;

typedef struct {
    int8_t filename[FILENAME_LEN];
    uint32_t filetype;
    uint32_t inode_num;
    int8_t reserved[DENTRY_RESERVED];
} dentry_t;


typedef struct  {
    uint32_t dir_count;
    uint32_t inode_count;
    uint32_t data_count;
    int8_t reserved[BOOT_BLOCK_RESERVED];
    dentry_t direentries[BOOT_BLOCK_ENTRIES];
} bootblock_t;

typedef struct{
    uint8_t data[FOUR_KBYTES];
} data_block_t;

typedef struct {
    int32_t (*open)(const uint8_t *);
    int32_t (*close)(int32_t);
    int32_t (*write)(int32_t, int8_t *, uint32_t);
    int32_t (*read)(int32_t, int8_t *, uint32_t);
} file_ops_t;

typedef struct file {
    file_ops_t file_ops;
    uint32_t inode;
    uint32_t f_pos;
    uint32_t flags;
} file_t;


void fs_init(uint32_t start_address);

inode_t* get_inode_ptr(uint32_t index);
uint32_t get_num_inodes();
uint32_t get_num_dentries();
uint32_t get_num_data_blocks();
// Returns directory entry information from the given name
int32_t read_dentry_by_name(const uint8_t * fname, dentry_t * dentry);

// Returns directory entry information from the given index
int32_t read_dentry_by_index(uint32_t i, dentry_t * dentry);

// Reads bytes starting from 'offset' in the file with the inode 'inode'.
int32_t read_data(uint32_t inode, uint32_t offset, int8_t * buf, uint32_t length);

int32_t file_open(const uint8_t * name);
int32_t file_close(int32_t fd);
int32_t file_write(int32_t fd, int8_t * data, uint32_t len);
int32_t file_read(int32_t fd, int8_t * buf, uint32_t count);

int32_t rtc_open(const uint8_t * name);
int32_t rtc_close(int32_t fd);

int32_t dir_open(const uint8_t * name);
int32_t dir_close(int32_t fd);
int32_t dir_write(int32_t fd, int8_t * data, uint32_t len);
int32_t dir_read(int32_t fd, int8_t * buf, uint32_t count);


int32_t sb16_open(const uint8_t * name);
int32_t sb16_close(int32_t fd);
int32_t sb16_write(int32_t fd, int8_t * data, uint32_t len);
int32_t sb16_read(int32_t fd, int8_t * buf, uint32_t count);

#endif
