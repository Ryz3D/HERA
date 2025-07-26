#ifndef INC_ASM_C
#define INC_ASM_C

#include "includes.h"

typedef struct asm_ins {
    char *label;
    char *instruction;
    char *comment;
} asm_ins_t;

typedef struct asm_var {
    char *name;
    uint16_t addr;
    uint32_t size;
} asm_var_t;

typedef struct asm_state {
    asm_ins_t **ins;
    uint32_t *ins_count;
    asm_var_t *vars;
    uint32_t vars_count;
} asm_state_t;

// returns false on error
bool cmp_asm_add_ins(asm_state_t *state, asm_ins_t *ins) {
    (*state->ins_count)++;
    *state->ins = realloc(*state->ins, (*state->ins_count) * sizeof(asm_ins_t));
    if (*state->ins == NULL) {
        printf(ERROR "out of memory" ENDL);
        return false;
    }
    memcpy(&(*state->ins)[(*state->ins_count) - 1], ins, sizeof(asm_ins_t));
    return true;
}

asm_var_t *cmp_asm_find_var(asm_state_t *state, char *name) {
    for (uint32_t i = 0; i < state->vars_count; i++) {
        if (!strcmp(state->vars[i].name, name)) {
            return &state->vars[i];
        }
    }
    return NULL;
}

// returns false on error
bool cmp_asm_generate(inter_ins_t *inter_ins, uint32_t inter_ins_count, asm_ins_t **asm_ins, uint32_t *asm_ins_count) {
    *asm_ins = NULL;
    *asm_ins_count = 0;

    asm_state_t state = {
        .ins = asm_ins,
        .ins_count = asm_ins_count,
        .vars = NULL,
        .vars_count = 0,
    };

    asm_ins_t asm_main = {
        .label = "main",
        .instruction = NULL,
        .comment = NULL,
    };
    if (!cmp_asm_add_ins(&state, &asm_main)) {
        return false;
    }
    asm_ins_t asm_stack_init = {
        .label =  NULL,
        .instruction = "*STACK_INIT",
        .comment = NULL,
    };
    if (!cmp_asm_add_ins(&state, &asm_stack_init)) {
        return false;
    }

    for (uint32_t i = 0; i < inter_ins_count; i++) {
        switch (inter_ins[i].type) {
            case INTER_INS_DECLARATION: {
                inter_ins_declaration_t *ins_t = (inter_ins_declaration_t *)inter_ins[i].ins;
                const char *type_str = cmp_parser_pure_type_to_str(ins_t->type, NULL);
                uint32_t comment_len = strlen(ins_t->name) + strlen(type_str) + 20;
                asm_ins_t asm_decl_comment = {
                    .label = NULL,
                    .instruction = NULL,
                    .comment = malloc(comment_len),
                };
                if (asm_decl_comment.comment == NULL) {
                    printf(ERROR "out of memory" ENDL);
                    return false;
                }
                uint16_t addr = ins_t->fixed_address;
                if (!ins_t->fixed) {
                    // TODO: better (actual) static allocator
                    addr = 0x4000 + state.vars_count;
                }
                state.vars_count++;
                state.vars = realloc(state.vars, state.vars_count * sizeof(asm_var_t));
                if (state.vars == NULL) {
                    printf(ERROR "out of memory" ENDL);
                    return false;
                }
                state.vars[state.vars_count - 1].name = malloc(strlen(ins_t->name) + 1);
                if (state.vars[state.vars_count - 1].name == NULL) {
                    printf(ERROR "out of memory" ENDL);
                    return false;
                }
                memcpy(state.vars[state.vars_count - 1].name, ins_t->name, strlen(ins_t->name) + 1);
                state.vars[state.vars_count - 1].addr = addr;
                state.vars[state.vars_count - 1].size = 1;
                snprintf(asm_decl_comment.comment,
                    comment_len,
                    "%s %s @ 0x%04X",
                    type_str,
                    ins_t->name,
                    addr);
                if (!cmp_asm_add_ins(&state, &asm_decl_comment)) {
                    return false;
                }
                break;
            }
            case INTER_INS_ASSIGNMENT: {
                inter_ins_assignment_t *ins_t = (inter_ins_assignment_t *)inter_ins[i].ins;
                asm_var_t *var = cmp_asm_find_var(&state, ins_t->to);
                if (var == NULL) {
                    printf(ERROR "unknown intermediate variable \"%s\"" ENDL, ins_t->to);
                    return false;
                }
                switch (ins_t->assignment_source) {
                    case INTER_INS_ASSIGNMENT_SOURCE_CONSTANT: {
                        asm_ins_t asm_a_init = {
                            .label = NULL,
                            .instruction = malloc(20),
                            .comment = NULL,
                        };
                        if (asm_a_init.instruction == NULL) {
                            printf(ERROR "out of memory" ENDL);
                            return false;
                        }
                        snprintf(asm_a_init.instruction,
                            20,
                            "0x%04X -> A",
                            ins_t->from_constant);
                        if (!cmp_asm_add_ins(&state, &asm_a_init)) {
                            return false;
                        }
                        break;
                    }
                    case INTER_INS_ASSIGNMENT_SOURCE_VARIABLE: {
                        asm_var_t *var2 = cmp_asm_find_var(&state, ins_t->from_variable);
                        if (var2 == NULL) {
                            printf(ERROR "unknown intermediate variable \"%s\"" ENDL, ins_t->from_variable);
                            return false;
                        }
                        asm_ins_t asm_rp_init = {
                            .label = NULL,
                            .instruction = malloc(20),
                            .comment = NULL,
                        };
                        if (asm_rp_init.instruction == NULL) {
                            printf(ERROR "out of memory" ENDL);
                            return false;
                        }
                        snprintf(asm_rp_init.instruction,
                            20,
                            "0x%04X -> RAM_P",
                            var2->addr);
                        if (!cmp_asm_add_ins(&state, &asm_rp_init)) {
                            return false;
                        }
                        asm_ins_t asm_ram_read = {
                            .label = NULL,
                            .instruction = "RAM -> A",
                            .comment = NULL,
                        };
                        if (!cmp_asm_add_ins(&state, &asm_ram_read)) {
                            return false;
                        }
                        break;
                    }
                    default:
                        printf(ERROR "unknown assignment source %i" ENDL, ins_t->assignment_source);
                        return false;
                }
                asm_ins_t asm_rp_init = {
                    .label = NULL,
                    .instruction = malloc(20),
                    .comment = NULL,
                };
                if (asm_rp_init.instruction == NULL) {
                    printf(ERROR "out of memory" ENDL);
                    return false;
                }
                snprintf(asm_rp_init.instruction,
                    20,
                    "0x%04X -> RAM_P",
                    var->addr);
                if (!cmp_asm_add_ins(&state, &asm_rp_init)) {
                    return false;
                }
                asm_ins_t asm_ram_write = {
                    .label = NULL,
                    .instruction = "A -> RAM",
                    .comment = NULL,
                };
                if (!cmp_asm_add_ins(&state, &asm_ram_write)) {
                    return false;
                }
                break;
            }
            case INTER_INS_PUSH: {
                inter_ins_push_t *ins_t = (inter_ins_push_t *)inter_ins[i].ins;
                asm_var_t *var = cmp_asm_find_var(&state, ins_t->from);
                if (var == NULL) {
                    printf(ERROR "unknown intermediate variable \"%s\"" ENDL, ins_t->from);
                    return false;
                }
                asm_ins_t asm_rp_init = {
                    .label = NULL,
                    .instruction = malloc(20),
                    .comment = NULL,
                };
                if (asm_rp_init.instruction == NULL) {
                    printf(ERROR "out of memory" ENDL);
                    return false;
                }
                snprintf(asm_rp_init.instruction,
                    20,
                    "0x%04X -> RAM_P",
                    var->addr);
                if (!cmp_asm_add_ins(&state, &asm_rp_init)) {
                    return false;
                }
                asm_ins_t asm_ram_read = {
                    .label = NULL,
                    .instruction = "RAM -> A",
                    .comment = NULL,
                };
                if (!cmp_asm_add_ins(&state, &asm_ram_read)) {
                    return false;
                }
                asm_ins_t asm_push = {
                    .label = NULL,
                    .instruction = "A -> *PUSH",
                    .comment = NULL,
                };
                if (!cmp_asm_add_ins(&state, &asm_push)) {
                    return false;
                }
                break;
            }
            case INTER_INS_POP: {
                inter_ins_pop_t *ins_t = (inter_ins_pop_t *)inter_ins[i].ins;
                asm_var_t *var = cmp_asm_find_var(&state, ins_t->to);
                if (var == NULL) {
                    printf(ERROR "unknown intermediate variable \"%s\"" ENDL, ins_t->to);
                    return false;
                }
                asm_ins_t asm_pop = {
                    .label = NULL,
                    .instruction = "*POP -> A",
                    .comment = NULL,
                };
                if (!cmp_asm_add_ins(&state, &asm_pop)) {
                    return false;
                }
                asm_ins_t asm_rp_init = {
                    .label = NULL,
                    .instruction = malloc(20),
                    .comment = NULL,
                };
                if (asm_rp_init.instruction == NULL) {
                    printf(ERROR "out of memory" ENDL);
                    return false;
                }
                snprintf(asm_rp_init.instruction,
                    20,
                    "0x%04X -> RAM_P",
                    var->addr);
                if (!cmp_asm_add_ins(&state, &asm_rp_init)) {
                    return false;
                }
                asm_ins_t asm_ram_write = {
                    .label = NULL,
                    .instruction = "A -> RAM",
                    .comment = NULL,
                };
                if (!cmp_asm_add_ins(&state, &asm_ram_write)) {
                    return false;
                }
                break;
            }
            case INTER_INS_JUMP: {
                inter_ins_jump_t *ins_t = (inter_ins_jump_t *)inter_ins[i].ins;
                break;
            }
            case INTER_INS_RETURN: {
                inter_ins_return_t *ins_t = (inter_ins_return_t *)inter_ins[i].ins;
                break;
            }
            case INTER_INS_OPERATION: {
                inter_ins_operation_t *ins_t = (inter_ins_operation_t *)inter_ins[i].ins;
                break;
            }
            case INTER_INS_STACK_OPERATION: {
                inter_ins_stack_operation_t *ins_t = (inter_ins_stack_operation_t *)inter_ins[i].ins;
                asm_var_t *var = cmp_asm_find_var(&state, ins_t->to);
                if (var == NULL) {
                    printf(ERROR "unknown intermediate variable \"%s\"" ENDL, ins_t->to);
                    return false;
                }
                asm_ins_t asm_pop1 = {
                    .label = NULL,
                    .instruction = "*POP -> A",
                    .comment = NULL,
                };
                if (!cmp_asm_add_ins(&state, &asm_pop1)) {
                    return false;
                }
                if (ins_t->op2) {
                    asm_ins_t asm_pop2 = {
                        .label = NULL,
                        .instruction = "*POP[keep=A] -> B",
                        .comment = NULL,
                    };
                    if (!cmp_asm_add_ins(&state, &asm_pop2)) {
                        return false;
                    }
                }

                if (!ins_t->op2) {
                    switch (ins_t->op) {
                        case INTER_OPERATOR_UN_ADDR_OF: {
                            break;
                        }
                        case INTER_OPERATOR_UN_DEREFERENCE: {
                            break;
                        }
                        case INTER_OPERATOR_UN_INCREMENT: {
                            // TODO: write back to variable A
                            asm_ins_t asm_init_b = {
                                .label = NULL,
                                .instruction = "1 -> B",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_init_b)) {
                                return false;
                            }
                            asm_ins_t asm_add = {
                                .label = NULL,
                                .instruction = "ADD -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_add)) {
                                return false;
                            }
                            break;
                        }
                        case INTER_OPERATOR_UN_DECREMENT: {
                            // TODO: write back to variable A
                            asm_ins_t asm_init_b = {
                                .label = NULL,
                                .instruction = "-1 -> B",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_init_b)) {
                                return false;
                            }
                            asm_ins_t asm_add = {
                                .label = NULL,
                                .instruction = "ADD -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_add)) {
                                return false;
                            }
                            break;
                        }
                        case INTER_OPERATOR_UN_NEGATE: {
                            asm_ins_t asm_com = {
                                .label = NULL,
                                .instruction = "COM -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_com)) {
                                return false;
                            }
                            break;
                        }
                        case INTER_OPERATOR_UN_INVERT: {
                            asm_ins_t asm_not = {
                                .label = NULL,
                                .instruction = "*NOT -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_not)) {
                                return false;
                            }
                            break;
                        }
                        case INTER_OPERATOR_UN_TO_BOOL: {
                            // use this in combination with & | ~ instead of && || !
                            // A != 0
                            // maybe implement by +255 (must be zero if no carry -> STAT & 0b1)
                            break;
                        }
                        default:
                            printf(ERROR "unknown unary operator %i" ENDL, ins_t->op);
                            return false;
                    }
                } else {
                    switch (ins_t->op) {
                        case INTER_OPERATOR_BI_ADD: {
                            asm_ins_t asm_add = {
                                .label = NULL,
                                .instruction = "ADD -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_add)) {
                                return false;
                            }
                            break;
                        }
                        case INTER_OPERATOR_BI_SUB: {
                            asm_ins_t asm_sub = {
                                .label = NULL,
                                .instruction = "*SUB -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_sub)) {
                                return false;
                            }
                            break;
                        }
                        case INTER_OPERATOR_BI_MUL: {
                            // TODO: this is very temporary (*2)
                            asm_ins_t asm_mov = {
                                .label = NULL,
                                .instruction = "B -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_mov)) {
                                return false;
                            }
                            asm_ins_t asm_mul = {
                                .label = NULL,
                                .instruction = "ADD -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_mul)) {
                                return false;
                            }
                            break;
                        }
                        case INTER_OPERATOR_BI_DIV: {
                            break;
                        }
                        case INTER_OPERATOR_BI_MOD: {
                            break;
                        }
                        case INTER_OPERATOR_BI_AND: {
                            asm_ins_t asm_and = {
                                .label = NULL,
                                .instruction = "*AND -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_and)) {
                                return false;
                            }
                            break;
                        }
                        case INTER_OPERATOR_BI_OR: {
                            asm_ins_t asm_or = {
                                .label = NULL,
                                .instruction = "*OR -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_or)) {
                                return false;
                            }
                            break;
                        }
                        case INTER_OPERATOR_BI_XOR: {
                            asm_ins_t asm_xor = {
                                .label = NULL,
                                .instruction = "*XOR -> A",
                                .comment = NULL,
                            };
                            if (!cmp_asm_add_ins(&state, &asm_xor)) {
                                return false;
                            }
                            break;
                        }
                        case INTER_OPERATOR_BI_SHL: {
                            break;
                        }
                        case INTER_OPERATOR_BI_SHR: {
                            break;
                        }
                        case INTER_OPERATOR_BI_LESS: {
                            break;
                        }
                        case INTER_OPERATOR_BI_LESS_EQ: {
                            break;
                        }
                        case INTER_OPERATOR_BI_GREATER: {
                            break;
                        }
                        case INTER_OPERATOR_BI_GREATER_EQ: {
                            break;
                        }
                        case INTER_OPERATOR_BI_EQUAL: {
                            break;
                        }
                        case INTER_OPERATOR_BI_UNEQUAL: {
                            break;
                        }
                        default:
                            printf(ERROR "unknown binary operator %i" ENDL, ins_t->op);
                            break; // return false;
                    }
                }

                asm_ins_t asm_rp_init = {
                    .label = NULL,
                    .instruction = malloc(20),
                    .comment = NULL,
                };
                if (asm_rp_init.instruction == NULL) {
                    printf(ERROR "out of memory" ENDL);
                    return false;
                }
                snprintf(asm_rp_init.instruction,
                    20,
                    "0x%04X -> RAM_P",
                    var->addr);
                if (!cmp_asm_add_ins(&state, &asm_rp_init)) {
                    return false;
                }
                asm_ins_t asm_ram_write = {
                    .label = NULL,
                    .instruction = "A -> RAM",
                    .comment = NULL,
                };
                if (!cmp_asm_add_ins(&state, &asm_ram_write)) {
                    return false;
                }
                break;
            }
            default: {
                asm_ins_t asm_invalid = {
                    .label = NULL,
                    .instruction = "?",
                    .comment = "unknown intermediate instruction",
                };
                if (!cmp_asm_add_ins(&state, &asm_invalid)) {
                    return false;
                }
                break;
            }
        }
    }

    asm_ins_t asm_end = {
        .label =  "end",
        .instruction = "\"end\" -> PC",
        .comment = NULL,
    };
    if (!cmp_asm_add_ins(&state, &asm_end)) {
        return false;
    }

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

    fwrite("inc programs/assembly/defaults.ha;" FILE_ENDL FILE_ENDL, 1, strlen("inc programs/assembly/defaults.ha;" FILE_ENDL FILE_ENDL), f);

    for (uint32_t i = 0; i < asm_ins_count; i++) {
        if (asm_ins[i].label != NULL) {
            fwrite("\"", 1, strlen("\""), f);
            fwrite(asm_ins[i].label, 1, strlen(asm_ins[i].label), f);
            fwrite("\":", 1, strlen("\":"), f);
        }
        if (asm_ins[i].instruction != NULL) {
            if (asm_ins[i].label != NULL) {
                fwrite(FILE_ENDL, 1, strlen(FILE_ENDL), f);
            }
            fwrite(ASM_INDENT, 1, strlen(ASM_INDENT), f);
            fwrite(asm_ins[i].instruction, 1, strlen(asm_ins[i].instruction), f);
            fwrite(";", 1, strlen(";"), f);
        }
        if (asm_ins[i].comment != NULL) {
            if (asm_ins[i].label == NULL && asm_ins[i].instruction == NULL) {
                fwrite(ASM_INDENT, 1, strlen(ASM_INDENT), f);
            } else {
                fwrite(" ", 1, 1, f);
            }
            fwrite("# ", 1, strlen("# "), f);
            fwrite(asm_ins[i].comment, 1, strlen(asm_ins[i].comment), f);
        }
        fwrite(FILE_ENDL, 1, strlen(FILE_ENDL), f);
    }
    fflush(f);
    fclose(f);

    return true;
}

void cmp_asm_debug_print(asm_ins_t *asm_ins, uint32_t asm_ins_count) {
    printf("(asm_debug_print)" ENDL);
    for (uint32_t i = 0; i < asm_ins_count; i++) {
        if (asm_ins[i].label != NULL) {
            printf("\"%s\":", asm_ins[i].label);
        }
        if (asm_ins[i].instruction != NULL) {
            if (asm_ins[i].label != NULL) {
                printf(ENDL);
            }
            printf(ASM_INDENT "%s;", asm_ins[i].instruction);
        }
        if (asm_ins[i].comment != NULL) {
            if (asm_ins[i].label == NULL && asm_ins[i].instruction == NULL) {
                printf(ASM_INDENT);
            } else {
                printf(" ");
            }
            printf("# %s", asm_ins[i].comment);
        }
        printf(ENDL);
    }
}

#endif
