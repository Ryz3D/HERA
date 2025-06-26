#include "includes.h"

int main(int argc, char *argv[]) {
    const char *path_src = NULL;
#if DEBUG
    path_src = DEBUG_SRC;
#else
    if (argc > 1) {
        path_src = argv[1];
    } else {
        printf(ERROR "specify source file as argument" ENDL);
        return 1;
    }
#endif

    inter_ins_t *inter_ins;
    uint32_t inter_ins_count;
    if (!cmp_parser_run(path_src, &inter_ins, &inter_ins_count)) {
        free(inter_ins);
        return 1;
    }

    asm_ins_t *asm_ins;
    uint32_t asm_ins_count;
    if (!cmp_asm_generate(inter_ins, inter_ins_count, &asm_ins, &asm_ins_count)) {
        free(inter_ins);
        free(asm_ins);
        return 1;
    }

    cmp_inter_debug_print(inter_ins, inter_ins_count);

    free(inter_ins);

    /*
    TODO:
    - optimize
     - simulate intermediate program, save known state, reset on label
     - remove unused variables
    */

    if (!cmp_asm_write("./test.ha", asm_ins, asm_ins_count)) {
        free(asm_ins);
        return 1;
    }
    free(asm_ins);

    return 0;
}
