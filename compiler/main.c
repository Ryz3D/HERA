#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "tokenizer.c"
#include "parser.c"

#define DEBUG 1
#define ENDL "\r\n"

const char *cmp_debug_src = "./programs/c/test.c";

int main(int argc, char *argv[]) {
    const char *path_src = argv[1];
#if DEBUG
    path_src = cmp_debug_src;
#endif

    printf("reading %s..." ENDL, path_src);

    return 0;
}
