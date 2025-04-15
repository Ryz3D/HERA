#include "includes.h"
#include "tokenizer.c"
#include "preprocessor.c"
#include "parser.c"
#include "intermediate.c"
#include "asm.c"

const char *cmp_debug_src = "./programs/c/test.c";

int main(int argc, char *argv[]) {
    const char *path_src = NULL;
#if DEBUG
    path_src = cmp_debug_src;
#else
    if (argc > 1) {
        path_src = argv[1];
    } else {
        printf("ERROR: specify source file as argument" ENDL);
        return 1;
    }
#endif

    inter_ins_t *inter_ins;
    uint32_t inter_ins_count;
    if (!cmp_inter_generate(path_src, &inter_ins, &inter_ins_count)) {
        return 1;
    }

    asm_instruction_t *asm_ins;
    uint32_t asm_ins_count;
    if (!cmp_asm_generate(inter_ins, inter_ins_count, &asm_ins, &asm_ins_count)) {
        return 1;
    }

    uint8_t *bin_data;
    uint32_t bin_data_size;
    if (!cmp_asm_assemble(asm_ins, asm_ins_count, &bin_data, &bin_data_size)) {
        return 1;
    }

    FILE *f_bin = fopen("compiled.bin", "w");
    if (f_bin == NULL) {
        printf("ERROR: failed to open binary output file" ENDL);
        return 1;
    }
    fwrite(bin_data, 1, bin_data_size, f_bin);
    fclose(f_bin);

    return 0;
}
