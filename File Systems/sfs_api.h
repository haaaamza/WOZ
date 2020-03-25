#ifndef __SFS_API_H
#define __SFS_API_H

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DISK_NAME "sfs.disk"
#define BLOCK_SIZE 1024
#define NUM_BLOCKS 600000
#define MAGIC_AD 0xACBD0005
#define BIT         0x00
#define NOBIT      0x01

#ifndef NUM_INODES
#define NUM_INODES 10000
#endif

#ifndef NUM_FILE_DESCRIPTORS
#define NUM_FILE_DESCRIPTORS 1000
#endif

#ifndef MAXFILENAME
#define MAXFILENAME 20
#endif

#ifndef MAXFILEXT
#define MAXFILEXT 3
#endif

#define INODE_SIZE sizeof(inode_t)
#define INODE_TABLE ( INODE_SIZE* 10000 / 1024 + 1)
#define NUM_FILES_PER_BLOCK (BLOCK_SZ / (MAXFILENAME + sizeof(uint64_t)))
#define FREE_BLOCKS_STORAGE ((NUM_BLOCKS / 1024) + 1)
#define MAX_BLOCKS_PER_INODE (NUM_DIRECT_BLOCKS + BLOCK_SZ / sizeof(uint64_t))
#define NUM_INDIRECT_POINTERS (BLOCK_SZ / sizeof(uint8_t))

// Force byte packing on structs stored on disk.
#pragma pack(push, blocks, 1)
struct magicBlocks{
    int magic;
    int block_size;
    int fsys_size;
    int inode_len;
    uint64_t root; //type I_NODE

} ;
typedef struct magicBlocks superblock_t;

typedef struct {
    uint64_t mode;
    uint64_t link_cnt;
    uint64_t uid;
    uint64_t gid;
    uint64_t size;
    uint64_t direct_ptr[12];
    uint64_t indirect_ptr;

    // Pad to block size length.
    uint8_t __RESERVED[BLOCK_SIZE - ((6 + 12) * sizeof(uint64_t))];
} inode_t;

typedef struct {
    char filename[MAXFILENAME];
    uint64_t inode;
} file_t;

typedef struct {
    uint64_t inode;
    uint64_t rptr;
    uint64_t wptr;
} file_descriptor;

extern int errno;

superblock_t *SUPER;
inode_t table[NUM_INODES];
file_descriptor root_dir;
file_descriptor fdt[NUM_FILE_DESCRIPTORS];
file_t direct[NUM_INODES];
uint8_t free_blk[NUM_BLOCKS];
uint8_t shown[NUM_INODES];
#pragma pack(pop, blocks)


void mksfs(int fresh);
int sfs_getnextfilename(char* fname);
int sfs_getfilesize(const char* path);
int sfs_fopen(char* name);
int sfs_fclose(int fileID);
int sfs_fread(int fileID, char* buf, int length);
int sfs_fwrite(int fileID, const char* buf, int length);
int sfs_frseek(int fileID, int loc); //fwseek, frseek
int sfs_fwseek(int fileID, int loc); //fwseek, frseek
int sfs_remove(char* file);
int write_pre(int fID, void* buf, int length);
void updt_direct();
void clear_buf(void* buf, int start, int end);
int sfs_seekMETA(int fileID, int loc, int RW);

#include "disk_emu.h"


#endif
