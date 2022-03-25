
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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
#include "myelf.h";
#define MAX_CHILDREN 3

/*****************************************************************************
 *                               kernel_main
 *****************************************************************************/
/**
 * jmp from kernel.asm::_start.
 *
 *****************************************************************************/
PUBLIC int kernel_main() {
    disp_str("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    int i, j, eflags, prio;
    u8 rpl;
    u8 priv; /* privilege */

    struct task* t;
    struct proc* p = proc_table;

    char* stk = task_stack + STACK_SIZE_TOTAL;

    for (i = 0; i < NR_TASKS + NR_PROCS; i++, p++, t++) {
        if (i >= NR_TASKS + NR_NATIVE_PROCS) {
            p->p_flags = FREE_SLOT;
            continue;
        }

        if (i < NR_TASKS) { /* TASK */
            t = task_table + i;
            priv = PRIVILEGE_TASK;
            rpl = RPL_TASK;
            eflags = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
            prio = 15;
        } else { /* USER PROC */
            t = user_proc_table + (i - NR_TASKS);
            priv = PRIVILEGE_USER;
            rpl = RPL_USER;
            eflags = 0x202; /* IF=1, bit 2 is always 1 */
            prio = 5;
        }

        strcpy(p->name, t->name); /* name of the process */
        p->p_parent = NO_TASK;

        if (strcmp(t->name, "INIT") != 0) {
            p->ldts[INDEX_LDT_C] = gdt[SELECTOR_KERNEL_CS >> 3];
            p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

            /* change the DPLs */
            p->ldts[INDEX_LDT_C].attr1 = DA_C | priv << 5;
            p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
        } else { /* INIT process */
            unsigned int k_base;
            unsigned int k_limit;
            int ret = get_kernel_map(&k_base, &k_limit);
            assert(ret == 0);
            init_desc(&p->ldts[INDEX_LDT_C], 0, /* bytes before the entry point
                                                 * are useless (wasted) for the
                                                 * INIT process, doesn't matter
                                                 */
                      (k_base + k_limit) >> LIMIT_4K_SHIFT,
                      DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

            init_desc(&p->ldts[INDEX_LDT_RW], 0, /* bytes before the entry point
                                                  * are useless (wasted) for the
                                                  * INIT process, doesn't matter
                                                  */
                      (k_base + k_limit) >> LIMIT_4K_SHIFT,
                      DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
        }

        p->regs.cs = INDEX_LDT_C << 3 | SA_TIL | rpl;
        p->regs.ds = p->regs.es = p->regs.fs = p->regs.ss =
            INDEX_LDT_RW << 3 | SA_TIL | rpl;
        p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
        p->regs.eip = (u32)t->initial_eip;
        p->regs.esp = (u32)stk;
        p->regs.eflags = eflags;

        p->ticks = p->priority = prio;

        p->p_flags = 0;
        p->p_msg = 0;
        p->p_recvfrom = NO_TASK;
        p->p_sendto = NO_TASK;
        p->has_int_msg = 0;
        p->q_sending = 0;
        p->next_sending = 0;

        for (j = 0; j < NR_FILES; j++)
            p->filp[j] = 0;

        stk -= t->stacksize;
    }

    k_reenter = 0;
    ticks = 0;

    p_proc_ready = proc_table;

    init_clock();
    init_keyboard();

    restart();

    while (1) {
    }
}

/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks() {
    MESSAGE msg;
    reset_msg(&msg);
    msg.type = GET_TICKS;
    send_recv(BOTH, TASK_SYS, &msg);
    return msg.RETVAL;
}

/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header { /* byte offset */
    char name[100];       /*   0 */
    char mode[8];         /* 100 */
    char uid[8];          /* 108 */
    char gid[8];          /* 116 */
    char size[12];        /* 124 */
    char mtime[12];       /* 136 */
    char chksum[8];       /* 148 */
    char typeflag;        /* 156 */
    char linkname[100];   /* 157 */
    char magic[6];        /* 257 */
    char version[2];      /* 263 */
    char uname[32];       /* 265 */
    char gname[32];       /* 297 */
    char devmajor[8];     /* 329 */
    char devminor[8];     /* 337 */
    char prefix[155];     /* 345 */
                          /* 500 */
};

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar f/e and store them.
 *
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char* filename) {
    printf("[extract `%s'\n", filename);
    int fd = open(filename, O_RDWR);

    int check_file;
    if (STATIC_CHECK) {
        check_file = open("check_file", O_RDWR);
        if (check_file == -1) {
            printf("creating check_file\n");
            check_file = open("check_file", O_CREAT);
        }
    }

    char temp_filename[32] = {0};
    assert(fd != -1);

    char buf[SECTOR_SIZE * 16];
    int chunk = sizeof(buf);
    int i = 0;
    int bytes = 0;

    while (1) {
        bytes = read(fd, buf, SECTOR_SIZE);
        assert(bytes == SECTOR_SIZE); /* size of a TAR file
                                       * must be multiple of 512
                                       */
        if (buf[0] == 0) {
            if (i == 0)
                printf("    need not unpack the file.\n");
            break;
        }
        i++;

        struct posix_tar_header* phdr = (struct posix_tar_header*)buf;

        /* calculate the file size */
        char* p = phdr->size;
        int f_len = 0;
        while (*p)
            f_len = (f_len * 8) + (*p++ - '0'); /* octal */

        int bytes_left = f_len;
        int fdout = open(phdr->name, O_CREAT | O_RDWR | O_TRUNC);
        if (fdout == -1) {
            printf("    failed to extract file: %s\n", phdr->name);
            printf(" aborted]\n");
            close(fd);
            return;
        }
        printf("    %s\n", phdr->name);
        strcpy(temp_filename, phdr->name);

        while (bytes_left) {
            int iobytes = min(chunk, bytes_left);
            read(fd, buf, ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
            bytes = write(fdout, buf, iobytes);
            assert(bytes == iobytes);
            bytes_left -= iobytes;
        }
        close(fdout);
        // here calculate its checksum
        if (STATIC_CHECK) {
            int fd_check = open(temp_filename, O_RDWR);
            if (fd_check == -1) {
                printf("open file for check failed\n");  // 打开当前文件
            }
            Check check;
            check.checksum = 0;  //初始化check
            int bytes_get = 1;
            char byte128[128];
            strcpy(check.name, temp_filename);
            while (bytes_get) {  // 循环读字节, 直到结束
                bytes_get = read(fd_check, &byte128, sizeof(byte128));
                int i = 0;
                for (i = 0; i < bytes_get; i++)
                    check.checksum ^= byte128[i];  // 按字节异或
            }
            write(check_file, &check, sizeof(check));  // 将结果写入
            close(fd_check);
        }
        // 关闭当前文件
    }

    if (i) {
        lseek(fd, 0, SEEK_SET);
        buf[0] = 0;
        bytes = write(fd, buf, 1);
        assert(bytes == 1);
    }
    if (STATIC_CHECK) {
        close(check_file);
    }

    close(fd);

    printf(" done, %d files extracted]\n", i);
}

int check_valid(int sub_argc, char* sub_argv[]) {
    int check_fd = open("check_file",
                        O_RDWR);  // 打开存储了校验码的文件
    if (check_fd == -1) {
        return 0;
    }
    int byte_get = 1;     // 存储读取到的
    char check_sum = 0;   // 存储checksum
    char temp_byte[128];  // temp_checksum
    Check check;
    int flag = 0;
    while (byte_get) {  // 得到文件的checksum
        byte_get = read(check_fd, &check, sizeof(check));
        if (strcmp(check.name, sub_argv[0]) == 0) {
            check_sum = check.checksum;
            flag = 1;
            break;
        }
    }
    close(check_fd);

    if (flag == 0) {  // 没有找到文件的checksum
        printf("sorry ,%s is not registered in system\n", sub_argv[0]);
        return 0;
    } else {
        int this_file = open(sub_argv[0], O_RDWR);
        if (this_file == -1) {
            printf("open %s wrong\n", sub_argv[0]);
        }

        byte_get = 1;
        while (byte_get) {
            byte_get = read(this_file, &temp_byte, sizeof(temp_byte));
            if (byte_get) {
                int i;
                for (i = 0; i < byte_get; i++) {
                    check_sum ^= temp_byte[i];
                }
            }
        }

        close(this_file);
        if (!check_sum) {
            printf("check right!\n");
            return 1;
        } else {
            printf("sorry, %s has been modified\n", sub_argv[0]);
            return 0;
        }
    }
}

/*****************************************************************************
 *                                shabby_shell
 *****************************************************************************/
/**
 * A very very simple shell.
 *
 * @param tty_name  TTY file name.
 *****************************************************************************/
void shabby_shell(const char* tty_name) {
    int fd_stdin = open(tty_name, O_RDWR);
    assert(fd_stdin == 0);
    int fd_stdout = open(tty_name, O_RDWR);
    assert(fd_stdout == 1);

    char rdbuf[128];

    while (1) {
        write(1, "$ ", 2);
        int r = read(0, rdbuf, 70);
        rdbuf[r] = 0;

        int argc = 0;
        char* argv[PROC_ORIGIN_STACK];
        char* p = rdbuf;
        char* s;
        int word = 0;
        char ch;
        do {
            ch = *p;
            if (*p != ' ' && *p != 0 && !word) {
                s = p;
                word = 1;
            }
            if ((*p == ' ' || *p == 0) && word) {
                word = 0;
                argv[argc++] = s;
                *p = 0;
            }
            p++;
        } while (ch);
        argv[argc] = 0;
        // printf("here is argv[0]:%d", argv[0]);
        // 此时已经将argc和argv准备好
        // __asm__ __volatile__("xchg %bx, %bx");
        int i;
        int count = 1;  // count 用于计算总的指令个
        for (i = 0; i < argc; i++) {
            // printf("%d: %s\n", i, argv[i]);
            if (argv[i][0] == '&') {
                count += 1;
            }
        }

        char* sub_argv[PROC_ORIGIN_STACK];
        int sub_argc = 0;

        for (i = 0; i <= argc; i++) {
            if (argv[i][0] != '&' && i != argc) {
                sub_argv[sub_argc] = argv[i];
                sub_argc += 1;
            } else {
                sub_argv[sub_argc] = 0;
                // printf("%s,%d\n", sub_argv[0], sub_argc - 1);
                int fd = open(sub_argv[0], O_RDWR);
                if (fd == -1) {
                    if (rdbuf[0]) {
                        write(1, "{", 1);
                        write(1, rdbuf, r);
                        write(1, "}\n", 2);
                    }
                    count--;
                } else {
                    close(fd);
                    int pid = fork();
                    if (pid != 0) { /* parent */
                        int s = 0;
                    } else { /* child */
                        if ((!STATIC_CHECK) ||
                            check_valid(sub_argc, sub_argv) == 1) {
                            execv(sub_argv[0], sub_argv);
                        } else {
                            printf("%s not valid\n", sub_argv[0]);
                        }
                    }
                }
                sub_argc = 0;
            }
        }

        int temp;  // 回收子进程, 提高稳定性
        while (count--) {
            wait(&temp);
        }
    }

    close(1);
    close(0);
}

/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 *
 *****************************************************************************/
void Init() {
    int fd_stdin = open("/dev_tty0", O_RDWR);
    assert(fd_stdin == 0);
    int fd_stdout = open("/dev_tty0", O_RDWR);
    assert(fd_stdout == 1);

    printf("Init() is running ...\n");

    /* extract `cmd.tar' */
    untar("/cmd.tar");

    char* tty_list[] = {"/dev_tty1", "/dev_tty2"};

    int i;
    for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
        int pid = fork();
        if (pid != 0) { /* parent process */
            printf("[parent is running, child pid:%d]\n", pid);
        } else { /* child process */
            printf("[child is running, pid:%d]\n", getpid());
            close(fd_stdin);
            close(fd_stdout);

            shabby_shell(tty_list[i]);
            assert(0);
        }
    }

    while (1) {
        int s;
        int child = wait(&s);
        printf("child (%d) exited with status: %d.\n", child, s);
    }

    assert(0);
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA() {
    // __asm__ __volatile__("xchg %bx,%bx");

    for (;;)
        ;
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB() {
    for (;;)
        ;
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC() {
    for (;;)
        ;
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char* fmt, ...) {
    int i;
    char buf[256];

    /* 4 is the size of fmt in the stack */
    va_list arg = (va_list)((char*)&fmt + 4);

    i = vsprintf(buf, fmt, arg);

    printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

    /* should never arrive here */
    __asm__ __volatile__("ud2");
}
