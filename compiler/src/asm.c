#ifndef INC_ASM_C
#define INC_ASM_C

#include "includes.h"

typedef uint16_t asm_int_t;

typedef enum asm_bus_w {
    ASM_BUS_W_LIT       = 0x0,
    ASM_BUS_W_A         = 0x1,
    ASM_BUS_W_B         = 0x2,
    ASM_BUS_W_C         = 0x3,
    ASM_BUS_W_RAM       = 0x4,
    ASM_BUS_W_RAM_P     = 0x5,
    ASM_BUS_W_PC        = 0x6,
    ASM_BUS_W_STAT      = 0x7,
    ASM_BUS_W_ADD       = 0xA,
    ASM_BUS_W_COM       = 0xB,
    ASM_BUS_W_NOR       = 0xC,
    ASM_BUS_W_INVALID   = 0x10,
} asm_bus_w_t;

typedef enum asm_bus_r {
    ASM_BUS_R_VOID      = 0x0,
    ASM_BUS_R_A         = 0x1,
    ASM_BUS_R_B         = 0x2,
    ASM_BUS_R_C         = 0x3,
    ASM_BUS_R_RAM       = 0x4,
    ASM_BUS_R_RAM_P     = 0x5,
    ASM_BUS_R_PC        = 0x6,
    ASM_BUS_R_STAT      = 0x7,
    ASM_BUS_R_A_B       = 0xA,
    ASM_BUS_R_B_RAM_P   = 0xB,
    ASM_BUS_R_C_PC      = 0xC,
    ASM_BUS_R_PC_C      = 0xD,
    ASM_BUS_R_PC_Z      = 0xE,
    ASM_BUS_R_PC_N      = 0xF,
    ASM_BUS_R_INVALID   = 0x10,
} asm_bus_r_t;

typedef struct asm_instruction {
    asm_int_t addr;
    asm_int_t literal;
    asm_bus_w_t bus_w;
    asm_bus_r_t bus_r;
    char *label;
} asm_instruction_t;

// returns false on error
bool cmp_asm_generate(inter_ins_t *inter_ins, uint32_t inter_ins_count, asm_instruction_t **asm_ins, uint32_t *asm_ins_count) {
    *asm_ins = malloc(1 * sizeof(asm_instruction_t));
    if (*asm_ins == NULL) {
        printf(ERROR "out of memory" ENDL);
        return false;
    }
    *asm_ins_count = 0;

    // TODO: iterate through intermediate code instrucitons, convert to hera assembly instructions

    for (uint32_t i = 0; i < inter_ins_count; i++) {
        // inter_ins[i];
    }

    return true;
}

bool cmp_asm_assemble(asm_instruction_t *asm_ins, uint32_t asm_ins_count, uint8_t **bin_data, uint32_t *bin_data_size) {
    *bin_data = NULL;

    *bin_data = 1;

    if (*bin_data == NULL) {
        return false;
    }

    return true;
}

#endif
