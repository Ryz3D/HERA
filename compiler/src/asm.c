#ifndef INC_ASM_C
#define INC_ASM_C

#include "includes.h"

typedef struct asm_ins {
    char *label;
    char *instruction;
    char *comment;
} asm_ins_t;

typedef struct asm_state {
    asm_ins_t *ins;
    uint32_t ins_count;
} asm_state_t;

// returns false on error
bool cmp_asm_add_ins(asm_state_t *state, asm_ins_t *ins) {
    state->ins_count++;
    state->ins = realloc(state->ins, state->ins_count * sizeof(asm_ins_t));
    if (state->ins == NULL) {
        printf(ERROR "out of memory" ENDL);
        return false;
    }
    memcpy(&state->ins[state->ins_count - 1], ins, sizeof(asm_ins_t));
    return true;
}

// returns false on error
bool cmp_asm_generate(inter_ins_t *inter_ins, uint32_t inter_ins_count, asm_ins_t **asm_ins, uint32_t *asm_ins_count) {
    asm_state_t state = {
        .ins = NULL,
        .ins_count = 0,
    };

    for (uint32_t i = 0; i < inter_ins_count; i++) {
        // TODO: convert inter_ins[i] to hera assembly (text)
        asm_ins_t asm_ins_temp = {
            .label = NULL,
            .instruction = NULL,
            .comment = NULL,
        };
        switch (inter_ins[i].type) {
            case INTER_INS_DECLARATION:
                asm_ins_temp.instruction = "DECL";
                break;
            case INTER_INS_ASSIGNMENT:
                asm_ins_temp.instruction = "ASSIGN";
                break;
            case INTER_INS_PUSH:
                asm_ins_temp.instruction = "PUSH";
                break;
            case INTER_INS_POP:
                asm_ins_temp.instruction = "POP";
                break;
            case INTER_INS_JUMP:
                asm_ins_temp.instruction = "JUMP";
                break;
            case INTER_INS_RETURN:
                asm_ins_temp.instruction = "RETURN";
                break;
            case INTER_INS_OPERATION:
                asm_ins_temp.instruction = "OP";
                break;
            case INTER_INS_STACK_OPERATION:
                asm_ins_temp.instruction = "STACKOP";
                break;
            default:
                asm_ins_temp.instruction = "?";
                break;
        }
        if (!cmp_asm_add_ins(&state, &asm_ins_temp)) {
            return false;
        }
    }

    *asm_ins = state.ins;
    *asm_ins_count = state.ins_count;

    return true;
}

bool cmp_asm_write(const char *f_path, asm_ins_t *asm_ins, uint32_t asm_ins_count) {
    if (f_path == NULL) {
        printf("assembly file path null" ENDL);
        return false;
    }

	FILE *f = fopen(f_path, "w");
	if (f == NULL) {
        printf(ERROR "could not open assembly file for write \"%s\"" ENDL, f_path);
        return false;
	}

    for (uint32_t i = 0; i < asm_ins_count; i++) {
        if (asm_ins[i].label != NULL) {
            fwrite(asm_ins[i].label, 1, strlen(asm_ins[i].label), f);
            fwrite(":" FILE_ENDL, 1, strlen(":" FILE_ENDL), f);
        }
        fwrite(ASM_INDENT, 1, strlen(ASM_INDENT), f);
        if (asm_ins[i].instruction != NULL) {
            fwrite(asm_ins[i].instruction, 1, strlen(asm_ins[i].instruction), f);
            fwrite(";", 1, strlen(";"), f);
        }
        if (asm_ins[i].comment != NULL) {
            fwrite(" # ", 1, strlen(" # "), f);
            fwrite(asm_ins[i].comment, 1, strlen(asm_ins[i].comment), f);
        }
        fwrite(FILE_ENDL, 1, strlen(FILE_ENDL), f);
    }
    fflush(f);
    fclose(f);

    return true;
}

#endif
