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
    if (!cmp_inter_generate(path_src, &inter_ins, &inter_ins_count)) {
        free(inter_ins);
        return 1;
    }

    asm_instruction_t *asm_ins;
    uint32_t asm_ins_count;
    if (!cmp_asm_generate(inter_ins, inter_ins_count, &asm_ins, &asm_ins_count)) {
        free(inter_ins);
        free(asm_ins);
        return 1;
    }
    free(inter_ins);

    uint8_t *bin_data;
    uint32_t bin_data_size;
    if (!cmp_asm_assemble(asm_ins, asm_ins_count, &bin_data, &bin_data_size)) {
        free(asm_ins);
        free(bin_data);
        return 1;
    }
    free(asm_ins);

    inter_ins_t inter_prog[] = {
        { .type = INTER_INS_DECLARATION,        .ins = &(inter_ins_declaration_t){ .type = PARSER_TYPE_UINT16, .name = "x", .fixed = false } },
        { .type = INTER_INS_ASSIGNMENT,         .ins = &(inter_ins_assignment_t){ .to = "x", .from_constant = 0xfe08, .assignment_source = INTER_INS_ASSIGNMENT_SOURCE_CONSTANT } },
        { .type = INTER_INS_PUSH,               .ins = &(inter_ins_push_t){ .from = "x" } },
        { .type = INTER_INS_DECLARATION,        .ins = &(inter_ins_declaration_t){ .type = PARSER_TYPE_UINT16, .name = "y", .fixed = false } },
        { .type = INTER_INS_ASSIGNMENT,         .ins = &(inter_ins_assignment_t){ .to = "y", .from_constant = 0xffff, .assignment_source = INTER_INS_ASSIGNMENT_SOURCE_CONSTANT } },
        { .type = INTER_INS_PUSH,               .ins = &(inter_ins_push_t){ .from = "y" } },
        { .type = INTER_INS_DECLARATION,        .ins = &(inter_ins_declaration_t){ .type = PARSER_TYPE_UINT16, .name = "GPOA", .fixed = true, .fixed_address = 0x0200 } },
        { .type = INTER_INS_STACK_OPERATION,    .ins = &(inter_ins_stack_operation_t){ .op2 = true, .op = INTER_OPERATOR_BI_ADD, .to = "GPOA", .type_a = PARSER_TYPE_UINT16, .type_b = PARSER_TYPE_UINT16 } }
    };
    uint32_t inter_prog_count = sizeof(inter_prog) / sizeof(inter_ins_t);
    cmp_inter_debug_print(inter_prog, inter_prog_count);

    /*
    TODO:
    - optimize
     - simulate intermediate program, save known state, reset on label
    */

    FILE *f_bin = fopen("compiled.bin", "w");
    if (f_bin == NULL) {
        printf(ERROR "failed to open binary output file" ENDL);
        return 1;
    }
    fwrite(bin_data, 1, bin_data_size, f_bin);
    fclose(f_bin);

    free(bin_data);

    return 0;
}
