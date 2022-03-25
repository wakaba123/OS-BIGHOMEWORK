#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"
#include "hd.h"
#include "fs.h"

PUBLIC int do_search_dir() {
    struct inode* dir_inode;
    char filename[MAX_PATH];
    char* dir = fs_msg.pBUF;
    int pointer = 0;

    printl("here : %s\n", dir);
    memset(filename, 0, MAX_FILENAME_LEN);
    if (strip_path(filename, dir, &dir_inode) != 0) {
        return 0;
    }

    // printl("dir:%s\n", dir);
    // printl("buf:%s\n", fs_msg.pBUF);
    // printl("dir_node:%d\n", dir_inode);

    int dir_blk0_nr = dir_inode->i_start_sect;
    int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    int nr_dir_entries =
        dir_inode->i_size / DIR_ENTRY_SIZE; /**
                                             * including unused slots
                                             * (the file has been deleted
                                             * but the slot is still there)
                                             */
    struct dir_entry* pde;
    int i, j;
    for (i = 0; i < nr_dir_blks; i++) {
        RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
        pde = (struct dir_entry*)fsbuf;
        for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++) {
            // printl("%s  ", pde->name);
            dir[pointer] = ' ';
            pointer += 1;
            memcpy(dir + pointer, pde->name, strlen(pde->name));
            pointer += strlen(pde->name);
        }
    }
    // printl("after for : %s\n", dir);
    return (void*)0;
}