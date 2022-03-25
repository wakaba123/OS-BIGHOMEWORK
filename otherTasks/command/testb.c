#include "stdio.h"

int main() {
    int n = 200;
    while (n--) {
        int i = 100;
        while (i--)
            printf("0");
    }
    return 0;
}