#include <stdio.h>
#include <string.h>
#include "../minfs.h"

 
 
MINFS_fs_t fs;
MINFS_file_t f;
char data[255];

int main(void){
  int32_t status;
  // format file-system
  fs.info.block_size = 6;
  fs.info.num_blocks = 128;
  fs.info.flags = 0;
  fs.info.os_flags = 0;
  fs.fs_id = 0;
  if( status = MINFS_Format(&fs, NULL) ){
  	printf("Error on Format: %d\n", status);
  	return 0;
  }
  printf("FS Formated!\n");
  // open filesystem
  if( status = MINFS_FSOpen(&fs, NULL) ){
  	printf("Error on Open Filesystem: %d\n", status);
  	return 0;
  }
  printf("FS Opened!\n");
  // open file...
  if( status = MINFS_FileOpen(&fs, 1, &f, NULL) ){
  	printf("Error on Open File: %d\n", status);
  	return 0;
  }
  printf("File Opened!\n");
  // ...and write
  uint32_t data_len = 255;
  strcpy(data, "This is a long test string. This is the second sentence of the Test-String. This is the third.\
    This is the fourth. And the fifth! This is the last!"
  );
  MINFS_FileWrite(&f, data, data_len, NULL);
  printf("Write to file success!\n");
  // seek to 0 and read
  MINFS_FileSeek(&f, 0, NULL);
  MINFS_FileRead(&f, data, &data_len, NULL);
  printf("Read from file success!\n");
  printf("\n%s\n", data);
  return 0;
}

