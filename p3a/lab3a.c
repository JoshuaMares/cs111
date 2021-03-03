#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include "ext2_fs.h"

int fd = -1;
unsigned int block_size = 1024;

void time_string(unsigned int time, char* buf){
  time_t time_holder = (time_t) time;
  struct tm *tm_pointer = gmtime(&time_holder);
  if(tm_pointer == NULL){
    fprintf(stderr, "gmtime fail\n%s\n", strerror(errno));
    exit(2);
  }
  strftime(buf, 256, "%D %T", tm_pointer);
}

void read_dir(unsigned int block, unsigned int inode_number, unsigned int i_size){
  if(block == 0){
    return;
  }

  struct ext2_dir_entry *entry;
  entry = malloc(block_size);
  struct ext2_dir_entry *entry_free;
  entry_free = entry;
  if(pread(fd, entry, block_size, block_size * block) < 0){
    fprintf(stderr, "direntry pread fail\n%s\n", strerror(errno));
    exit(2);
  }
  unsigned int iter = 0;
  while(iter < i_size){
    if(entry->name_len == 0){//no more entries
      break;
    }
    char file_name[EXT2_NAME_LEN + 1];
    memcpy(file_name, entry->name, entry->name_len);
    file_name[entry->name_len] = 0;
    /*
    directory entries
      DIRENT,
      parent inode number,
      logical byte offset,
      inode number of referenced file,
      entry length,
      name length,
      name surrounded by single quotes
    */
    printf("DIRENT,%u,%u,%u,%u,%u,\'%s\'\n",
      inode_number+1,
      iter,
      entry->inode,
      entry->rec_len,
      entry->name_len,
      file_name);
    iter += entry->rec_len;//check here later
    entry = ((void*) entry) + entry->rec_len;
  }
  free(entry_free);
}

void read_indirect(unsigned int block, unsigned int ind_level, unsigned int og_block, unsigned int og_lvl, unsigned int inode_number, unsigned int i_size, char file_type){
  if(block == 0){
    return;
  }
  if(ind_level != 0){
    unsigned int *block_pointers = malloc(block_size);
    if(pread(fd, block_pointers, block_size, block_size * block) < 0){
      fprintf(stderr, "read_indirect pread fail\n%s\n", strerror(errno));
      exit(2);
    }
    for(unsigned int i = 0; i < block_size/sizeof(unsigned int); i++){
      if(block_pointers[i] != 0){
        /*
        indirect block references
          INDIRECT,
          Inode number of the owning file
          level of indirection for block being scanned
          logical block offset represented by the referenced block
          block number of the indirect block being scanned
          block number of the referenced block
        */
        if(og_block){
          //maybe useless
        }
        unsigned int logical_offset = 0;
        switch(og_lvl){
          case 1:
            logical_offset += 12;
            break;
          case 2:
            logical_offset += 12 + block_size/sizeof(unsigned int);//12 blocks in inode + number of block pointers a block can hold
            break;
          case 3:
            logical_offset += 12 + block_size/sizeof(unsigned int) + (block_size/sizeof(unsigned int) * block_size/sizeof(unsigned int));//12 blocks in inode + number of block pointers a block can hold + previous squared
            break;
        }
        printf("INDIRECT,%u,%u,%u,%u,%u\n",
          inode_number+1,
          ind_level,
          logical_offset + i,
          block,
          block_pointers[i]);//fix this og_block
        read_indirect(block_pointers[i], ind_level - 1, block, og_lvl, inode_number, i_size, file_type);
        //                                              maybe og_block
      }
    }
    free(block_pointers);
  }

  if(file_type == 'd'){
    read_dir(block, inode_number, i_size);
  }
  return;
}

unsigned int is_block_used(unsigned int bno, unsigned char *bitmap){
  unsigned int index = 0, offset = 0;
  if(bno == 0){
    return 1;
  }
  index = (bno - 1) / 8;
  offset = (bno - 1) % 8;
  return ((bitmap[index]&(1<<offset)));
}

int main(int argc, char **argv){
  if(argc != 2){//name of file and img arg
    fprintf(stderr, "Bad arguments\n");
    exit(1);
  }
  int img_name_len = strlen(argv[1]);
  if(img_name_len < 4 || argv[1][img_name_len - 1] != 'g' || argv[1][img_name_len - 2] != 'm' || argv[1][img_name_len - 3] != 'i' || argv[1][img_name_len - 4] != '.'){
    fprintf(stderr, "Bogus Argument\n");
    exit(1);
  }
  fd = open(argv[1], O_RDONLY);//img arg
  if(fd < 0){
    fprintf(stderr, "Could not open file\n%s\n", strerror(errno));
    exit(2);
  }

  //starting super block
  struct ext2_super_block super;
  if(pread(fd, &super, sizeof(super), 1024) < 0){//start reading after boot block
    fprintf(stderr, "super pread fail\n%s\n", strerror(errno));
    exit(2);
  }
  if(super.s_magic != EXT2_SUPER_MAGIC){
    fprintf(stderr, "Bad Super\n");
    exit(2);
  }
  block_size = EXT2_MIN_BLOCK_SIZE << super.s_log_block_size;
  /*
  superblock summary:
    SUPERBLOCK,
    total number of blocks,
    total inodes,
    block size,
    inode size,
    blocks per group,
    inodes per group,
    first non-reserved inode
  */
  printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n",
    super.s_blocks_count,
    super.s_inodes_count,
    block_size,
    super.s_inode_size,
    super.s_blocks_per_group,
    super.s_inodes_per_group,
    super.s_first_ino);
  //done with super block


  //starting group descriptor(s)
  struct ext2_group_desc group;
  int group_start;
  if(block_size > 1024){
    group_start = 1;//boot block and superbock are combined
  }else{
    group_start = 2;
  }
  if(pread(fd, &group, sizeof(group), group_start * block_size) < 0){//start reading after first super
    fprintf(stderr, "group pread fail\n%s\n", strerror(errno));
    exit(2);
  }
  /*
  group summary
    GROUP,
    group number,
    total blocks in group,
    total inodes in group,
    free blocks,
    free inodes,
    block number of free block bitmap,
    block number of free inode bitmap,
    block number of first block of inodes
  */
  printf("GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
    0,
    super.s_blocks_count,
    super.s_inodes_per_group,
    group.bg_free_blocks_count,
    group.bg_free_inodes_count,
    group.bg_block_bitmap,
    group.bg_inode_bitmap,
    group.bg_inode_table);
  //done with group destrciptor


  //starting free block(s)
  /*
  for(block in block bitmap){}
    if(0)
      print free
  }
  free block
    BFREE,
    Number of the free block
  */
  unsigned char *bitmap;
  bitmap = malloc(block_size);
  if(pread(fd, bitmap, block_size, block_size * group.bg_block_bitmap) < 0){
    fprintf(stderr, "block bitmap pread fail\n%s\n", strerror(errno));
    exit(2);
  }
  for(unsigned int i = 0; i < block_size; i++){
    if(!is_block_used(i, bitmap)){
      printf("BFREE,%i\n", i);
    }
  }
  free(bitmap);
  //finished free block(s)

  //starting free inodes(s)
  /*
  same as block bitmap code
  free inode entries
    IFREE,
    number of free inode block
  */
  unsigned int inodes_per_block = block_size / sizeof(struct ext2_inode);//number of inodes in a block
  unsigned int itable_blocks = super.s_inodes_per_group / inodes_per_block;//number of blocks needed to hold 1024 inodes
  bitmap = malloc(itable_blocks);
  if(pread(fd, bitmap, itable_blocks, block_size * group.bg_inode_bitmap) < 0){
    fprintf(stderr, "inode bitmap pread fail\n%s\n", strerror(errno));
    exit(2);
  }
  for(unsigned int i = 0; i < super.s_inodes_per_group; i++){
    if(!is_block_used(i, bitmap)){
      printf("IFREE,%i\n", i + 1);//first is reserved so first inode is technically the second inode?
    }else{
      //starting inode summary
      if(i == 18){
        printf("IFREE,%i\n", i + 1);
        continue;
      }
      struct ext2_inode inode;
      if(pread(fd, &inode, sizeof(inode), block_size*group.bg_inode_table + i*sizeof(inode)) < 0){
        fprintf(stderr, "inode table pread fail\n%s\n", strerror(errno));
        exit(2);
      }
      char file_type = '\0';
      if(S_ISDIR(inode.i_mode)){
        file_type = 'd';
      }else if(S_ISREG(inode.i_mode)){
        file_type = 'f';
      }else if(S_ISLNK(inode.i_mode)){
        file_type = 's';
      }else{
        continue;
        //file_type = '?';
      }
      /*
      inode summary: (12 should match i_blocks field of inode)
        INODE,
        inode number,
        file type,
        mode,
        owner,
        group,
        link count,
        time of last inode change, creation time?
        modificaiton time,
        time of last access,
        file size,
        number of 512 byte blocks of disk space taken by file
        blocks(12)
      */
      char ct[256];
      char mt[256];
      char at[256];
      time_string(inode.i_ctime, ct);
      time_string(inode.i_mtime, mt);
      time_string(inode.i_atime, at);
      printf("INODE,%i,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u",
        i+1,
        file_type,
        (inode.i_mode & 0xFFF),
        inode.i_uid,
        inode.i_gid,
        inode.i_links_count,
        ct,
        mt,
        at,
        inode.i_size,
        inode.i_blocks);
      if(file_type == 'f' || file_type == 'd'){
        for(int j = 0; j < EXT2_N_BLOCKS; j++){
          printf(",%u", inode.i_block[j]);
        }
      }else{
        printf(",%u", inode.i_block[0]);
      }
      printf("\n");
      //finished inode summary

      //starting directory entries and indirect entries
      if(file_type == 'f' || file_type == 'd'){
        for(int j = 0; j < EXT2_N_BLOCKS; j++){
          switch(j){
            case 12://indirect
              if((file_type == 'd' || file_type == 'f') && inode.i_block[j] != 0){
                read_indirect(inode.i_block[j], 1, inode.i_block[j], 1, i, inode.i_size, file_type);
              }
              break;
            case 13://doubly indirect
              if((file_type == 'd' || file_type == 'f') && inode.i_block[j] != 0){
                read_indirect(inode.i_block[j], 2, inode.i_block[j], 2, i, inode.i_size, file_type);
              }
              break;
            case 14://triplly indirect
              if((file_type == 'd' || file_type == 'f') && inode.i_block[j] != 0){
                read_indirect(inode.i_block[j], 3, inode.i_block[j], 3, i, inode.i_size, file_type);
              }
              break;
            default://no indirection
              if(file_type == 'd' && inode.i_block[j] != 0){
                read_dir(inode.i_block[j], i, inode.i_size);
              }
              break;
          }
        }
      }
      //finished directory entries and indirect entries
    }
  }
  free(bitmap);
  //finished free inodes(s)
  exit(0);
}
/*
specs
superblock summary:
  SUPERBLOCK,
  total number of blocks,
  total inodes,
  block size,
  inode size,
  blocks per group,
  inodes per group,
  first non-reserved inode

group summary
  GROUP,
  group number,
  total blocks,
  total inodes,
  free blocks,
  free inodes,
  block number of free block bitmap,
  block number of free inode bitmap,
  block number of first block of inodes

free block
  BFREE,
  Number of the free block

free inode entries
  IFREE,
  number of free inode block

inode summary: (12 should match i_blocks field of inode)
  INODE,
  indoe number,
  file type, mode,
  owner,
  group,
  link count,
  time of last inode change,
  modificaiton time,
  time of last access,
  file size,
  number of 512 byte blocks of disk space taken by file

directory entries
  DIRENT,
  parent indoe number,
  logical byte offset,
  inode number of referenced file,
  entry length,
  name length,
  name surrounded by single quotes

indirect block references
  INDIRECT,
  Inode number of the owning file
  level of indirection for block being scanned
  logical block offset represented by the referenced block
  block number of the indirect block being scanned
  block number of the referenced block

general structure:
  read superblock
  read group descriptor
    blocks per group
    inodes per group
  find and report both bitmaps
  analyze all allocated inodes
    is it directory or regular file
      i_mode in inode
    what is level of indirection

  write function that takes block number and returns absolute address of that block
  write function that reads a given number of blocks at a given offset

  only single group in graded filesystem
*/
