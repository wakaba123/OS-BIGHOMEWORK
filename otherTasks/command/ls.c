#include "stdio.h"
#include "string.h"
#include "fs.h"
#include "const.h"

int main(int args, char* argv[]) {
    char* result;
    result = search_dir("/");
    printf("%s\n", result);
    return 0;
}