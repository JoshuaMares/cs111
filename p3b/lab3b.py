#!/usr/bin/python
from __future__ import print_function
import sys
import csv

corrupt = 0

if len(sys.argv) != 2:
    print('Bad Arguments', file=sys.stderr)
    exit(1)

if sys.argv[1][-4:] != ".csv":
    print('Bogus Arguments', file=sys.stderr)
    exit(1)

csv_file = open(sys.argv[1], 'r')#gets file object
csv_reader = csv.reader(csv_file)

max_block = 0
num_reserved = 0
max_inode = 0
bfree_list = []
ifree_list = []
inode_list = []
dirent_list = {}
indirect_list = []
for line in csv_reader:
    #add each type to appropriate list
    #print(line)
    print(line[0])
    n = line[0]
    if n == 'SUPERBLOCK':
        max_block = int(line[1])
        num_reserved = 4 + int(line[6])/(int(line[3])/int(line[4])) #4+number of blocksoccupied by inode table
        max_inode = int(line[2])
    elif n == 'BFREE':
        bfree_list.append(int(line[1]))
    elif n == 'IFREE':
        ifree_list.append(int(line[1]))
    elif n == 'INODE':
        inode_list.append(line)
    elif n == 'DIRENT':
        if int(line[1]) in dirent_list:
            dirent_list[int(line[1])].append(line)
        else:
            dirent_list[int(line[1])] = list()
    elif n == 'INDIRECT':
        indirect_list.append(line)

print(inode_list)
my_block_bitmap = []
for inode in inode_list:
    #if inode not used
    #   continue
    print('entered inode loop')
    if int(inode[11]) > 0:
        for block in inode[-15:]:
            if int(block) < 0 or int(block) > max_block:
                #print('INVALID ' + indirection + 'block ' + block + 'at offset' + offset)
                print('invalid')
                corrupt = 2
            if int(block) <= num_reserved:
                print('reserved')
                corrupt = 2
            if int(block) in ifree_list:
                print('allocated')
                corrupt = 2
            if int(block) in my_block_bitmap:
                print('duplicated')
                corrupt = 2
            my_block_bitmap.append(int(block))

for block in my_block_bitmap:
    if int(block) not in bfree_list:
        print('unreferenced')
        corrupt = 2

for inode in inode_list:
    if int(inode[3]) != 0 and int(inode[1]) in ifree_list:
        print('allocated')
        corrupt = 2
    elif int(inode[3]) == 0 and int(inode[1]) not in ifree_list:
        printf('unallocated')
        corrupt = 2

iref_array = []
ipar_array = []
for inode in inode_list:
    if inode[2] != 'd':
        continue
    par_inode = int(inode[1])
    for de in dirent_list[int(inode[1])]:
        child_ino = int(de[1]) #sus
        child_name = de[6]
        if child_ino < 0 or child_ino > max_inode:
            print('invalid')
            corrupt = 2
        if child_ino in ifree_list:
            print('unallocated')
            corrupt = 2
        if child_name == '.' and child_ino != par_ino:
            print('current mismatch')
            corrupt = 2
        if child_name != '.' and child_name != '..':
            iref_array[child_ino] += 1
        ipar_array[child_ino] = par_ino

for inode in inode_list:
    if inode[2] != 'd':
        continue
    par_inode = int(inode[1])
    if iref_array[par_ino] != int(inode[6]):
        print('incorrect link count')
        corrupt = 2
    for de in dirent_list[int(inode[1])]:
        child_ino = int(de[3])
        child_name = de[6]
        if child_name == '..' and child_ino != ipar_array[par_ino]:
            print('mismatch')
            corrupt = 2

exit(corrupt)
