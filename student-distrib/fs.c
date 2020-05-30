#include "fs.h"
#include "syscall.h"
#include "sb16.h"
/*
You will need to support operations on the file system image provided to you, including opening and reading from
files,  opening and reading the directory (there’s only one—the structure is flat),  and copying program images into
contiguous physical memory from the randomly ordered 4 kB “disk” blocks that constitute their images in the file
system.  The source code for our
ls
program will show you how reading directories is expected to work.  Also see
Appendix A for an overview of the file system as well as Appendix B for how each function should work
*/

/*
8    Appendix A: The File System

8.1    File System Utilities

The figure below shows the structure and contents of the file system.  The file system memory is divided into 4 kB
blocks.  The first block is called the boot block, and holds both file system statistics and the directory entries.  Both
the statistics and each directory entry occupy 64B, so the file system can hold up to 63 files. The first directory entry
always refers to the directory itself, and is named “.”, so it can really hold only 62 files

Each directory entry gives a name (up to 32 characters, zero-padded, but
not necessarily including a terminal EOS
or 0-byte
), a file type, and an index node number for the file.  File types are 0 for a file giving user-level access to
the real-time clock (RTC), 1 for the directory, and 2 for a regular file. The index node number is only meaningful for
regular files and should be ignored for the RTC and directory types.
Each regular file is described by an index node that specifies the file’s size in bytes and the data blocks that make up
the file. Each block contains 4 kB; only those blocks necessary to contain the specified size need be valid, so be careful
not to read and make use of block numbers that lie beyond those necessary to contain the file data.

    Returns directory entry information from the given name
        int32_t read_dentry_by_name(const uint8_t * fname, dentry_t * dentry);

    Returns directory entry information from the given index
        int32_t read_dentry_by_index(uint32_t index, dentry_t * dentry);

    Reads bytes starting from 'offset' in the file with the inode 'inode'.
        int32_t read_data(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length);

The three routines provided by the file system module return -1 on failure, indicating a non-existent file or invalid
index in the case of the first two calls, or an invalid inode number in the case of the last routine. Note that the directory
entries are indexed starting with 0. Also note that the read_data call can only check that the given inode is within the
valid range. It does not check that the inode actually corresponds to a file (not all inodes are used). However, if a bad
data block number is found within the file bounds of the given inode, the function should also return -1.

When successful, the first two calls fill in the dentry_t block passed as their second argument with the file name, file
type, and inode number for the file, then return 0. The last routine works much like the read system call, reading up to length bytes
starting from position offset in the file with inode number inode
and returning the number of bytes read and placed in the buffer.
A return value of 0 thus indicates that the end of the file has been reached.
*/

static uint32_t bb_address;
static inode_t * inodes;
static dentry_t * dentries;
static data_block_t* data_blocks;

/*
description: initializes the file system
input: address in memory where the file system is stored
output: none
sfx: writes to glb vars that manage file system
*/
void fs_init(uint32_t start_address){

	// the boot block is at the address provided
    bb_address=start_address;
	//d entries are 64B into the boot block hench its addr + BOOT_BLOCK_FIRST_HALF
    dentries=(dentry_t*)(bb_address+BOOT_BLOCK_FIRST_HALF);
	// inodes are 4 kB in on block 1 "boot block is index 0"
    inodes=(inode_t*)(bb_address+(FOUR_KBYTES));
	//+1 cause 1st page for bblock
    data_blocks=(data_block_t*)(bb_address+(FOUR_KBYTES*(get_num_inodes()+1)));
return;
}

/*
description: gets the number of inodes in file system
input: none
output: number of inodes in fs
sfx: reads the boot block
*/
uint32_t get_num_inodes(){
	// get boot block address
    bootblock_t* bb=(bootblock_t*)bb_address;
	// ret the inode count
    return bb->inode_count;
}

/*
description: gets the number of dentries in file system
input: none
output: number of dentries in fs
sfx: reads the boot block
*/
uint32_t get_num_dentries(){
	// get boot block address
    bootblock_t* bb=(bootblock_t*)bb_address;
	// ret the dentries count
    return bb->dir_count;
}

/*
description: gets the number of data_blocks in file system
input: none
output: number of data_blocks in fs
sfx: reads the boot block
*/
uint32_t get_num_data_blocks(){
	// get boot block address
    bootblock_t* bb=(bootblock_t*)bb_address;
	// ret the data_blocks count
    return bb->data_count;
}

/*
description: gives a pointer to the inode at the given index
input: index of the inode needed
output: ptr to inode
sfx: none
*/
inode_t* get_inode_ptr(uint32_t index){
	/*
		index does not need to be multiplied by 4kB
		because "inodes" has ptr type inode_t which is
		of size 4kB so when the ptr is incrmented it
		goes to the next inode structure instead of
		going to the next byte inside inode
	*/
    return (inode_t*)(inodes+index);
}

/*
description: Returns directory entry information from the given name
input: filename and dentry to put info into
output:
	0 on match
	-1 on fail
sfx: goes through the file system
*/
int32_t read_dentry_by_name(const uint8_t * fname, dentry_t * dentry){
    if (fname == NULL || dentry == NULL || fname[0] == '\0') return -1;
    uint32_t i; // iterator
    uint32_t dentry_name_len;
    uint32_t fname_len;

    fname_len = strlen((int8_t*)fname);
	// loop goes through the d entries looking for a match to the name given
    for (i = 0; i < BOOT_BLOCK_ENTRIES; i++) {
		/* check if a filename string in they dentries is null terminated
		 	if it is call strlen() on it and put its ret value into len var
			else put the max string value which is FILENAME_LEN or 32
		 */
        dentry_name_len = (dentries[i].filename[FILENAME_LEN-1] == '\0') ? \
            strlen(dentries[i].filename) : FILENAME_LEN;
		//if the str length is not the same as the fname given continue
		// this speeds up the search since we only compare potential hits
        if (dentry_name_len != fname_len) continue;
		// compare fname and cur name strncmp returns 0 on similar
        if (!strncmp(dentries[i].filename, (int8_t*)fname, fname_len)) {
			// copy file info into dentry
            strncpy(dentry->filename, dentries[i].filename, FILENAME_LEN);
            dentry->filetype = dentries[i].filetype;
            dentry->inode_num = dentries[i].inode_num;
            strncpy(dentry->reserved, dentries[i].reserved, DENTRY_RESERVED);
            return 0;
        }
    }
	// file name was not found
    return -1;
}

/*
description: Returns directory entry information from the given index
input:
	dentry index
	dentry to store info into
output:
	0: on successful
	-1: invalid index or dentry ptr
sfx: reads file system
*/
int32_t read_dentry_by_index(uint32_t i, dentry_t * dentry){
	//check to see if i is valid index and dentry is not null
    if(i < get_num_dentries() && dentry != NULL){
		// copy info into dentry
        strncpy(dentry->filename, dentries[i].filename, FILENAME_LEN);
        dentry->filetype = dentries[i].filetype;
        dentry->inode_num = dentries[i].inode_num;
        strncpy(dentry->reserved, dentries[i].reserved, DENTRY_RESERVED);
        return 0;
    }
	// invalid index/null ptr was given
    return -1;
}

/*
description: reads data from an inode puts it in a given buffer and returns bytes read
input:
	inode: 	index to the inode
	offset: position inside data block to start reading from
	buf: 	ptr to buffer to put read data into
	length: how many bytes to read
output:
	0: successful
	-1: invalid parameters
sfx: read from file system
*/
int32_t read_data(uint32_t inode, uint32_t offset, int8_t * buf, uint32_t length){

    if (buf == NULL || inode >= get_num_inodes()) return -1;
	// get inode ptr to inode we want to read from
    inode_t* inode_ptr = &inodes[inode];
	// current index of data block number inside the inode
    uint32_t curr_block_idx = offset / FOUR_KBYTES;
	//index to the data block
    uint32_t curr_block = inode_ptr->data_block_num[curr_block_idx];
	//index inside data block
    uint32_t curr_pos = offset % FOUR_KBYTES;
	//index for buffer
    uint32_t buf_idx = 0;
	//file length
    uint32_t file_length=inode_ptr->length;
	// error check for offset
    if(offset>file_length)
        return -1;

    // Read into our buffer one byte at a time (Should change to memcpy later for speed)
    while(length>0 && offset < file_length){
        // Find our block (relative to inode) and position
        curr_block_idx = offset / FOUR_KBYTES;
        curr_pos = offset % FOUR_KBYTES;

        // Get the Actual block
        curr_block = inode_ptr->data_block_num[curr_block_idx];
        if (curr_block > get_num_data_blocks()) return -1;

        // Read one byte
        buf[buf_idx] = data_blocks[curr_block].data[curr_pos];
        
		//inc indexes
        buf_idx++;
        offset++;
        length--;
        // printf("%d",buf_idx);

    }
	// null terminate the buffer
    // buf[length -1] = '\0';
	//return bytes read
    return buf_idx;
}


// init temp structures

/*
description: Open file handler
input:
	char* name : pointer to name of file to open
output:
	0: on successful open
	-1: invalid name or type
sfx: opens and reads file from fs and sets flags
*/
int32_t file_open(const uint8_t * name) {
    if (name == NULL) return -1;    //performing invalid name and file type checks

    dentry_t entry;
    if (read_dentry_by_name((uint8_t*)name, &entry) == -1) return -1;

    if (entry.filetype != DENTRY_TYPE_FILE) return -1; // Probably won't need this check after open() dispatch (open finds filetype first)

    pcb_t * pcb = get_current_pcb();
    int32_t ret;
    uint32_t fd;
    ret = get_available_fd(pcb, &fd);

    if (ret != 0 || pcb->files[fd].flags != 0) return -1;

    file_t newfile; //sets all flags for files after taking an empty file and setting attributes to global open file
    newfile.file_ops.open = file_open;
    newfile.file_ops.close = file_close;
    newfile.file_ops.read = file_read;
    newfile.file_ops.write = file_write;
    newfile.inode = entry.inode_num;
    newfile.f_pos = 0;
    newfile.flags = 1;

    pcb->files[fd] = newfile;

    // printf("%s %d",name,fd);
    return fd;
}

// undo file_open
/*
description: Close file handler
input:
	a file descriptor ptr
output:
	0: on successful close
	-1: on invalid close if file was never opened
sfx: sets flags
*/
int32_t file_close(int32_t fd) {

    if (fd >= NUM_FILES) return -1;
    pcb_t * pcb = get_current_pcb();
    if (pcb->files[fd].flags == 0) return -1;
    pcb->files[fd].flags = 0;
    return 0;
}

// do nothing unless we implement r/w
/*
description: Write file handler
input:
        fd  : fd pointer
        data: pointer to block to write to
        len : length to write until
output:

	-1: invalid name or file is closed, since this is read only not implemented yet.
sfx: opens and reads file from fs and sets flags
*/
int32_t file_write(int32_t fd, int8_t * data, uint32_t len) {
    return -1;
}

// read count bytes of data from file into buf using read_data
/*
description: Read file handler
input:
	    int32_t fd      :   file descriptor ptr
        int8_t * buf    :   buffer to write the stuff thats been read of the file system
        uint32_t count  :   numbers of bytes to read
output:
        ret : number of bytes read
        -1 on failed read
sfx: opens and reads file from fs
*/
int32_t file_read(int32_t fd, int8_t * buf, uint32_t count) {

    if (buf == NULL || fd >= NUM_FILES || fd < 0) return -1;             // null checks and flag checks
    int32_t ret;

    pcb_t * pcb = get_current_pcb();

    if (pcb->files[fd].flags == 0) return -1;


    ret = read_data(pcb->files[fd].inode, pcb->files[fd].f_pos, buf, count);  //reads from fs

    if (ret == -1) return -1;
    pcb->files[fd].f_pos += ret; //sets last read byte, in handler so that future reads would happen after the last read

    // buf[count-1] = '\0'; //sets last byte to NULL
    // printf("%d %d %d /n",fd,count,ret);
    return ret;
}

/* dir_open
 * description: Opens a directory given the name
 * input:
 * 	name - name of the file to open
 * output:
 *	0 - successful
 * -1 - failure
 * side effects: Sets the open_dir variable
*/
int32_t dir_open(const uint8_t * name) {
    // Make sure we were given a valid name and there is not a directory open already
    if (name == NULL) return -1;


    dentry_t entry;

    // Ensure we have a valid length name
    if (strlen((int8_t*)name) > FILENAME_LEN) return -1;

    // Find the directory's dentry by the given name
    if (read_dentry_by_name((uint8_t*)name, &entry) == -1) return -1;

    // Make sure the found dentry represents a directory
    if (entry.filetype != DENTRY_TYPE_DIRECTORY) return -1;

    pcb_t * pcb = get_current_pcb();
    int32_t ret;
    uint32_t fd;
    ret = get_available_fd(pcb, &fd);

    if (ret != 0 || pcb->files[fd].flags != 0) return -1;

    // Copy the found dentry's contents into the open_dir variable
    file_t newfile;
    newfile.file_ops.open = dir_open;
    newfile.file_ops.close = dir_close;
    newfile.file_ops.read = dir_read;
    newfile.file_ops.write = dir_write;
    newfile.inode = entry.inode_num;
    newfile.f_pos = 0;
    newfile.flags = 1;
    pcb->files[fd] = newfile;

    return fd;
}

/* dir_close
 * description: Closes the currently open directory
 * input:
 * 	fd - File descriptor of the directory to close
 * output:
 *	0 - successful
 * -1 - failure
 * side effects: Sets open_dir's flag to 0
*/
int32_t dir_close(int32_t fd) {
    // Make sure there is an open directory
    if (fd >= NUM_FILES || fd < 0) return -1;
    pcb_t * pcb = get_current_pcb();
    if (pcb->files[fd].flags == 0) return -1;
    pcb->files[fd].flags = 0;
    return 0;
}

/* dir_write
 * description: Write to a directory
 * input:
 * 	fd - File descriptor of the directory to write to
 *  data - buffer to write into the directory
 *  len - length of data
 * output:
 *	0 - successful
 * -1 - failure
 * side effects: May edit the filesystem
*/
int32_t dir_write(int32_t fd, int8_t * data, uint32_t len) {
    return -1;
}

/* dir_read
 * description: Read from a directory
 * input:
 * 	fd - File descriptor of the directory to read from
 *  data - buffer to write into the directory
 *  len - length of data we can write to
 * output:
 *	0 - successful
 * -1 - failure
 * side effects: Write to given buffer
*/
int32_t dir_read(int32_t fd, int8_t * buf, uint32_t count) {

    // Make sure we are given a legitimate buffer and there is an open directory.
    if (buf == NULL) return -1;


    pcb_t * pcb = get_current_pcb();

    if (pcb->files[fd].flags == 0) return -1;

    if (pcb->files[fd].f_pos >= BOOT_BLOCK_ENTRIES) return 0;

    // Holds the current directory entry
    dentry_t entry;

    // Go until we find an existing file in the directory
    while (read_dentry_by_index(pcb->files[fd].f_pos, &entry) == -1) {
        pcb->files[fd].f_pos++;

        // If we've hit the end of the directory, say we written 0 bytes
        if (pcb->files[fd].f_pos >= BOOT_BLOCK_ENTRIES) return 0;
    }

    // File names are NULL padded, but may take up all FILENAME_LEN bytes
    uint32_t dentry_name_len = (entry.filename[FILENAME_LEN-1] == '\0') ? \
            strlen(entry.filename) : FILENAME_LEN;

    // Make sure we have enough room to copy the filename
    if (count < dentry_name_len) {
        pcb->files[fd].f_pos++;
        return -1;
    } else {
        // If we have enough room, copy the filename
        strncpy(buf, entry.filename, dentry_name_len);
        pcb->files[fd].f_pos++;
    }

    return dentry_name_len;
}


/* rtc_open
 * description: Open a file as an rtc type
 * input:
 * 	name - Should always be "rtc"
 * output:
 *	The FD of the new file, or -1 on failure
 * side effects: Initializes a new file as RTC
*/
int32_t rtc_open(const uint8_t * name) {
    //if (name == NULL) return -1;    //performing invalid name and file type checks

    dentry_t entry;
    if (read_dentry_by_name((uint8_t*)name, &entry) == -1) return -1;

    if (entry.filetype != DENTRY_TYPE_RTC) return -1; // Probably won't need this check after open() dispatch (open finds filetype first)

    pcb_t * pcb = get_current_pcb();
    int32_t ret;
    uint32_t fd;
    ret = get_available_fd(pcb, &fd);

    if (ret != 0 || pcb->files[fd].flags != 0) return -1;

    file_t newfile; //sets all flags for files after taking an empty file and setting attributes to global open file
    newfile.file_ops.open = rtc_open;
    newfile.file_ops.close = rtc_close;
    newfile.file_ops.read = read_RTC;
    newfile.file_ops.write = set_RTC;
    newfile.inode = entry.inode_num;
    newfile.f_pos = 0;
    newfile.flags = 1;

    pcb->files[fd] = newfile;

    return fd;
}

/* rtc_close
 * description: Close an RTC file
 * input:
 * 	fd - File descriptor of the file to close
 * output:
 *	The FD of the new file, or -1 on failure
 * side effects: Initializes a new file as RTC
*/
int32_t rtc_close(int32_t fd) {

    // Make sure we have a valid file descriptor
    if (fd >= NUM_FILES || fd < 0) return -1;

    // Get the PCB
    pcb_t * pcb = get_current_pcb();

    // Make sure this file isn't already closed
    if (pcb->files[fd].flags == 0) return -1;

    // Close the file, return success
    pcb->files[fd].flags = 0;
    return 0;
}


int32_t sb16_open(const uint8_t * name){
    if (name == NULL) return -1;    //performing invalid name and file type checks

    dentry_t entry;
    if (read_dentry_by_name((uint8_t*)name, &entry) == -1) return -1;

    if (entry.filetype != DENTRY_TYPE_FILE) return -1; // Probably won't need this check after open() dispatch (open finds filetype first)

    pcb_t * pcb = get_current_pcb();
    int32_t ret;
    uint32_t fd;
    ret = get_available_fd(pcb, &fd);

    if (ret != 0 || pcb->files[fd].flags != 0) return -1;

    file_t newfile; //sets all flags for files after taking an empty file and setting attributes to global open file
    newfile.file_ops.open = sb16_open;
    newfile.file_ops.close = sb16_close;
    newfile.file_ops.read = sb16_read;
    newfile.file_ops.write = sb16_write;
    newfile.inode = entry.inode_num;
    newfile.f_pos = 0;
    newfile.flags = 1;

    pcb->files[fd] = newfile;

    // printf("%s %d",name,fd);
    return fd;
}

int32_t sb16_close(int32_t fd){

    dsp_write(SB16_END_AI_16);
    if (fd >= NUM_FILES || fd < 0) return -1;
    pcb_t * pcb = get_current_pcb();
    if (pcb->files[fd].flags == 0) return -1;
    pcb->files[fd].flags = 0;
    return 0;
}

int32_t sb16_write(int32_t fd, int8_t * data, uint32_t len){
    
    DMA_buffer = (uint8_t*)data;

    full_buffer = len;
    init_mixer();

        // Install our general handler and enable the PIC line
    setIRQhandler(sb_pic_line + IRQ_OFFSET, &sb16_handler);
    enable_irq(sb_pic_line);
    init_DMA();
    sb16_play();
    return 0;
}

int32_t sb16_read(int32_t fd, int8_t * buf, uint32_t count){
    while (!got_dsp_int);
    got_dsp_int = 0;
    return 0;
}
