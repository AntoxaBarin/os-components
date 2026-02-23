/* FUSE File System */

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>

struct ffs_inode {

};

struct ffs_state {
  struct ffs_inode* root;
};


int ffs_open(const char* path, struct fuse_file_info* fi) { 
  return 0; 
}

struct fuse_operations ffs_oper = {
    .open = ffs_open,
};

int main(int argc, char* argv[]) {
  printf("Hello world\n");
  return 0;
}