#include "sfs_api.h"
#include "disk_emu.h"
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void clear_buf(void* buf, int start, int end){
    for (int i = start; i < end; i++) {
        *(uint8_t*)(buf + i) = 0;
    }
}

int free_allocate(){
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (free_blk[i]==NOBIT) {
            free_blk[i] = BIT;
            write_blocks(1, FREE_BLOCKS_STORAGE, free_blk);
            return i;
        }
    }

    // No free blocks available.
    errno = ENOSPC;
    perror("No Free Blocks");
    return -(errno);
}

void updt_direct(){
    write_pre(&root_dir, (void*)&direct, sizeof(file_t) * NUM_INODES);
    root_dir.wptr = 0;
}




int write_pre(int fID, void* buf, int length){
    // Store number of bytes written.
    int written,isread, iswritten = 0;
    file_descriptor* fileID= &fdt[fID];

    // Pull inode.
    inode_t* inode = &table[fileID->inode];


    // Allocate room for a block size in memory.
    void* block = (void*)malloc(BLOCK_SIZE);

    int start=fileID->wptr;
    int start_index = start / BLOCK_SIZE;
    int curr_cnt = start_index;
    int dirCount=0;

    while (length) {

        // Get direct block.
        int blck_idx = -1;
        if (curr_cnt < 12) {
            int z=inode->direct_ptr[curr_cnt];
            if (z>0) {
                blck_idx= z;
            } else {
                blck_idx = free_allocate();
                inode->direct_ptr[curr_cnt] = blck_idx;
                write_blocks(1 + FREE_BLOCKS_STORAGE, SUPER->inode_len, table);

            }
        }else {
            perror("no more blocks");
            return written;
        }

        if (blck_idx<0){
            perror("Write");
            break;
        }

        clear_buf(block, 0, BLOCK_SIZE);
        isread = read_blocks(blck_idx, 1, block);

        // Modify block in memory.
        int m=fileID->wptr % BLOCK_SIZE;
        while(m<BLOCK_SIZE){

            if (length <= 0) { //done
                break;
            }
            // Copy current byte into buffer.
            memcpy(block + m, buf+dirCount, 1);

            fileID->wptr += 1;
            dirCount+=1;
            m++;

            // Update inode size.
            if (inode->size < fileID->wptr) { //write pointer causes inode size to increase.
                inode->size = fileID->wptr;
            }


            //+1 Blocks Written
            written += 1;
            // Decrement length.
            length -= 1;

        }

        iswritten= write_blocks(blck_idx, 1, block);

        curr_cnt += 1;
    }

    free(block);


    // Write inodes and free blocks.
    write_blocks(1 + FREE_BLOCKS_STORAGE, SUPER->inode_len, table);
    write_blocks(1, FREE_BLOCKS_STORAGE, free_blk);
    return written;
}
int read_pre (int fID, void* buf, int length){//meta read

    int red, isread = 0;
    file_descriptor* fileID= &fdt[fID];
    inode_t* inode = &table[fileID->inode];

    // Allocate room for a block size in memory.
    void* block = (void*)malloc(BLOCK_SIZE);

    int start=fileID->rptr;
    int curr_cnt = start / BLOCK_SIZE;
    int dirCount=0;

    while (length) {

        int blck_idx = -1;
        if (curr_cnt < 12) {
            int z=inode->direct_ptr[curr_cnt];
            if (z>0) {
                blck_idx= z;
            } else {
                blck_idx = free_allocate();
                inode->direct_ptr[curr_cnt] = blck_idx;
                write_blocks(1 + FREE_BLOCKS_STORAGE, SUPER->inode_len, table);

            }
        }else {
            perror("no more blocks");
            return red;
        }

        if (blck_idx<0){
            perror("Read");
            break;
        }

        clear_buf(block, 0, BLOCK_SIZE);
        isread = read_blocks(blck_idx, 1, block);

        int m= fileID->rptr % BLOCK_SIZE;
        while(m<BLOCK_SIZE){

            if (length <= 0) { //done
                break;
            }
            // Copy current byte into buffer.
            memcpy(block + m, buf+dirCount, 1);

            fileID->rptr += 1;
            dirCount+=1;
            m++;

            if (inode->size <= fileID->rptr) { //write pointer causes inode size to increase.
                break;
            }


            red += 1;
            length -= 1;

        }

        isread= write_blocks(blck_idx, 1, block);

        curr_cnt += 1;
    }

    free(block);

    return red;
}

void mksfs(int fresh){
    // Set up root directory file descriptor.
    root_dir.inode = SUPER->root;
    root_dir.rptr = 0;

    if (fresh==1) {//Create disk using disk emulator

        init_fresh_disk(DISK_NAME, BLOCK_SIZE, NUM_BLOCKS);

        SUPER = (superblock_t*)calloc(sizeof(superblock_t),1); //Initialize super block

        SUPER->magic = MAGIC_AD;
        SUPER->block_size = BLOCK_SIZE;
        SUPER->fsys_size = NUM_BLOCKS * BLOCK_SIZE;
        SUPER->inode_len = INODE_TABLE;
        SUPER->root = 0;
        write_blocks(0, 1, &SUPER); //writing block to disk
        //bit map..?

        // Initialize mask.
        for (int i = 0; i < NUM_BLOCKS; i++) {
            if (i>=(1 + FREE_BLOCKS_STORAGE + SUPER->inode_len)){
                free_blk[i]=NOBIT;
            }else{
                free_blk[i]=BIT;
            }
        }

        // Write to the inode table.
        //-----------------
        table[0].link_cnt = 2;
        write_blocks(1 + FREE_BLOCKS_STORAGE, SUPER->inode_len, table);

        // Write root directory.
        file_t root_file;
        root_file.inode = SUPER->root;
        strncpy(root_file.filename, ".", MAXFILENAME);
        direct[SUPER->root] = root_file;
        updt_direct();

        // Write free blocks.
        write_blocks(1, FREE_BLOCKS_STORAGE, free_blk);
    } else {

        init_disk(DISK_NAME, BLOCK_SIZE, NUM_BLOCKS);
        read_blocks(0, 1, SUPER);

        // Store free mask, backwards of first part
        read_blocks(1, FREE_BLOCKS_STORAGE, free_blk);

        //  Store inode table backwards of first part.
        read_blocks(1 + FREE_BLOCKS_STORAGE, SUPER->inode_len, table);

        // Read root directory.
        read_pre(&root_dir, (void*)&direct, sizeof(file_t) * NUM_INODES);
    }
}
//-------------------
int sfs_getnextfilename(char* fname){

    for (int i = 0; i < NUM_INODES; i++) {

        if (!shown[i] && direct[i].inode) {//may be undefined check
            strcpy(fname, direct[i].filename);
            return 1;
            shown[i] = 1;
        }
    }
    for (int i = 0; i < NUM_INODES; i++) {
        shown[i] = 0;
    }
    return 0;
}

int sfs_getfilesize(const char* path){

    int loc=-1;
    for (int i = 0; i < NUM_INODES; i++) {
        if (strncmp(direct[i].filename, path, MAXFILENAME) == 0) {
            loc=i;
        }
    }
    if (loc == -1) {
        perror("Error in File Size");
        exit (-1);
    }

    return table[loc].size;
}


int sfs_fopen(char* name){

    int loc=-1;
    for (int i = 0; i < NUM_INODES; i++) {
        if (strncmp(direct[i].filename, name, MAXFILENAME) == 0) {
            loc=i;
        }
    }
    if (loc == -1) {

        int inode=-1;
        for (int i = 1; i < NUM_INODES; i++) {
            if (table[i].link_cnt == 0) {
                table[i].link_cnt = 1;
                inode= i;
                break;
            }
        }

        if (inode < 0) {
            // No more inodes.
            perror("no more inodes");//Need this?
            return inode;
        }

        // Create new file.
        file_t f;
        strncpy(f.filename, name, MAXFILENAME);
        f.inode = inode;
        //inode_t n = table[inode];
        //n.link_cnt += 1;
        //any need for this??

        // Add file to directory.
        direct[inode] = f;
        updt_direct();
    }

    // Get file descriptor number.
    int fd = -1;
    for (int i = 0; i < NUM_FILE_DESCRIPTORS; i++) {
        if (fdt[i].inode == loc) {
            fd = i;
            break;
        }
    }

    if (fd == -1) {
        // Get unused file descriptor number.
        int fileno = -1;
        file_descriptor filed;
        for (int i = 0; i < NUM_FILE_DESCRIPTORS; i++) {
            filed = fdt[i];
            if (filed.inode == 0 && filed.wptr == 0) {
                fileno = i;
                break;
            }
        }
        fd=fileno;

        if (fd < 0) {
            // No more file descriptors.
            return fd;
        }

        // Set file descriptor.
        fdt[fd].inode = loc;
        fdt[fd].rptr=0;
        fdt[fd].wptr = table[loc].size;
    }

    // Write to disk.
    write_blocks(1, FREE_BLOCKS_STORAGE, free_blk);
    write_blocks(1 + FREE_BLOCKS_STORAGE, SUPER->inode_len, table);

    // Return file descriptor.
    return fd;
}


int sfs_fclose(int fileID){

    if (fileID >= NUM_FILE_DESCRIPTORS) {
        perror("Out of Bound");
        exit(-1);
    }

    if (fdt[fileID].inode>0) {
        fdt[fileID].inode = 0;
        fdt[fileID].rptr = 0;
        fdt[fileID].wptr=0;
        return 0;
    }


    perror("Bad file");
    exit(-1);
}


//simple own implem, Read/Write
int sfs_fread(int fileID, char* buf, int length){
    

    if (fdt[fileID].inode != 0) {
        return read_pre(&fdt[fileID], (void*)buf, length);
    }


    perror("Bad File");
    exit(-1);
}

int sfs_fwrite(int fileID, const char* buf, int length){
    //OOB
   

    if (fdt[fileID].inode != 0) {
        return write_pre(&fdt[fileID], (void*)buf, length);
    }

    // Bad file number.
    errno = EBADF;
    perror("Write");
    return 0;
}
//(Y)
int sfs_seekMETA(int fileID, int loc, int RW)
{
    if (fileID >= NUM_FILE_DESCRIPTORS) {
        errno = ENFILE;
        perror("Seek");
        return errno;
    }

    if (fdt[fileID].inode>0) {
        //removed
        // Set pointer.
        if (RW==0){
            fdt[fileID].wptr = loc;
        }
        else{
            fdt[fileID].rptr = loc;

        }

        return 0;
    }

    errno = EBADF;
    perror("Seek");
    return errno;
}
int sfs_fwseek(int fileID, int loc){

    int ret= sfs_seekMETA( fileID,  loc,  0); //0 for Write
    return ret;
}
int sfs_frseek(int fileID, int loc){

    int ret= sfs_seekMETA( fileID,  loc,  1);    //1 for READ
    return ret;
}
int sfs_remove(char *file){
    return 0;
}



//TODO: Read, Write, Remove, separate methods for Read and Write File since we need them.
