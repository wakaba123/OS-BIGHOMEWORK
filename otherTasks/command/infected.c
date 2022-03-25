#include "stdio.h"
#include "myelf.h"
int main(int argc, char* argv[]) {
    // int fd = open("check_file", O_RDWR);
    // Check check;
    // int byte_get = 1;
    // while (byte_get) {
    //     byte_get = read(fd, &check, sizeof(check));
    //     if (byte_get) {
    //         printf("%s,%d\n", check.name, check.checksum);
    //     }
    // }

    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    exit(0);
    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    __asm__ __volatile__("xchg %bx,%bx");
    printf("aaaa");
    return 0;
}