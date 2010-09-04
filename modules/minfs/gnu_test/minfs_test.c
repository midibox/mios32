#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../minfs.h"

#define data_len 255
 
MINFS_fs_t fs;
MINFS_file_t f;
char data[data_len];
int32_t status;



// ------- local prototypes -------
static uint32_t fs_init(void);
static uint32_t file_open(uint16_t file_i);
static uint32_t file_write(void);
static uint32_t file_read(void);


// ------- main -------
int main(void){
  strcpy(data, "MINFS is a minimal filesystem.\nIt features numbered files, PEC, Block caching interface,\nsupport for random data r/w form or to the storage unit (in this case without PEC).");
  
  fs_init();

  file_open(1);
  
  file_write();

  strcpy(data, "Block size and the number of blocks are configurable at format time\nand will be stored in the fs-header at the beginning of the first block.\nMaximal number of files is limited by the number of blocks.");

  file_open(2);

  file_write();

  file_open(1);
  
  file_read();

  file_open(2);

  file_read();


  exit(0);
}

// ------- helper functions -------
static uint32_t fs_init(void){
  // format f-system
  fs.info.block_size = 6;
  fs.info.num_blocks = 64;
  fs.info.flags = 0;
  fs.info.os_flags = 0;
  fs.fs_id = 0;
  if( status = MINFS_Format(&fs, NULL) ){
    printf("Error on FS-format: %d\n", status);
    exit(0);
  }
  // open filesystem
  if( status = MINFS_FSOpen(&fs, NULL) ){
    printf("Error on FS-open: %d\n", status);
    exit(0);
  }
  printf("FS formated and opened!\n");
  return 0;
}

static uint32_t file_open(uint16_t file_i){
  // open file...
  if( status = MINFS_FileOpen(&fs, file_i, &f, NULL) ){
    printf("Error on open file: %d\n", status);
    exit(0);
  }
  printf("file openened: %d!\n", file_i);
  return 0;
}

static uint32_t file_write(void){
  if( (status  = MINFS_FileWrite(&f, &(data[0]), data_len, NULL)) && status != MINFS_STATUS_EOF ){
    printf("Error on file write: %d\n", status);
    exit(0);
  }
  printf("Write to file :%d!\n", f.file_id);
  return 0;
}

static uint32_t file_read(void){
  if( status = MINFS_FileSeek(&f, 0, NULL) ){
    printf("Error on file-seek: %d\n", status);
  }
  memset(data, 0, data_len); // empty the string buffer
  uint32_t len = data_len;
  if( (status = MINFS_FileRead(&f, &(data[0]), &len, NULL)) && status != MINFS_STATUS_EOF ){
    printf("Error on file-read: %d\n", status);
    exit(0);
  }
  printf("Read from file %d!\n", f.file_id);
  printf("\n%s\n\n", data);
  return 0;
}
