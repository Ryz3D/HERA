#include "includes.h"
#include "tokenizer.c"
#include "parser.c"
#include "intermediate.c"

const char *cmp_debug_src = "./programs/c/test.c";

int main(int argc, char *argv[]) {
    const char *path_src = argv[1];
#if DEBUG
    path_src = cmp_debug_src;
#endif

    printf("parsing \"%s\"..." ENDL, path_src);
    FILE *f = fopen(path_src, "r");
    if (f == NULL) {
        printf("could not open source file \"%s\"" ENDL, path_src);
        return 1;
    }

    ast_element_t *ast = NULL;
    cmp_parser_run(f, &ast);
    fclose(f);
    if (ast == NULL) {
        return 1;
    }
    printf("ast %i" ENDL, ast->type);

    return 0;
}
