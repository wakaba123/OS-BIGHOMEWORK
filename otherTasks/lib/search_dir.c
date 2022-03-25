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
#include "proto.h"

PUBLIC char* search_dir(char* path) {
    MESSAGE msg;
    msg.type = SEARCH;
    // msg.pBUF[0] = 'y';
    // msg.pBUF[1] = 'j';
    // msg.pBUF[2] = 'q';
    memcpy(msg.pBUF, path, strlen(path));
    // printl("msg.pBug address is %d\n", msg.pBUF);
    // printl("BUF : %s\n", msg.pBUF);
    send_recv(BOTH, TASK_FS, &msg);
    // return msg.BUF;
    return msg.pBUF;
}