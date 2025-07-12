#ifndef INC_PARSER_C
#define INC_PARSER_C

#include "includes.h"

typedef enum cmp_parser_pure_type {
    PARSER_TYPE_INVALID,
    PARSER_TYPE_VOID,
    PARSER_TYPE_UINT8,
    PARSER_TYPE_UINT16,
    PARSER_TYPE_UINT32,
    PARSER_TYPE_UINT64,
    PARSER_TYPE_INT8,
    PARSER_TYPE_INT16,
    PARSER_TYPE_INT32,
    PARSER_TYPE_INT64,
    PARSER_TYPE_FLOAT,
    PARSER_TYPE_DOUBLE,
    PARSER_TYPE_OTHER,
} cmp_parser_pure_type_t;

// TODO: moving it here is sort of a temporary solution for circular dependency

typedef struct inter_context {
    // TODO: context description struct (all defined functions/variables)
    // TODO: add all variables (+functions?) to list (maybe prefix: filename_function_variable), before adding: check if name already exists and append something to differenciate
    int i;
} inter_context_t;

typedef enum inter_ins_type {
    INTER_INS_DECLARATION,
    INTER_INS_ASSIGNMENT,
    INTER_INS_PUSH,
    INTER_INS_POP,
    INTER_INS_JUMP,
    INTER_INS_RETURN,
    INTER_INS_OPERATION,
    INTER_INS_STACK_OPERATION,
} inter_ins_type_t;

typedef struct inter_ins_declaration {
    char *name;
    cmp_parser_pure_type_t type;
    bool fixed;
    uint16_t fixed_address;
} inter_ins_declaration_t;

typedef enum inter_assignment_source {
    INTER_INS_ASSIGNMENT_SOURCE_CONSTANT,
    INTER_INS_ASSIGNMENT_SOURCE_VARIABLE,
} inter_assignment_source_t;

typedef struct inter_ins_assignment {
    inter_assignment_source_t assignment_source;
    char *to;
    char *from_variable;
    uint16_t from_constant;
} inter_ins_assignment_t;

typedef struct inter_ins_push {
    char *from;
} inter_ins_push_t;

typedef struct inter_ins_pop {
    char *to;
} inter_ins_pop_t;

typedef enum inter_ins_jump_condition {
    INTER_INS_JUMP_CONDITION_NONE,
    INTER_INS_JUMP_CONDITION_CARRY,
    INTER_INS_JUMP_CONDITION_ZERO,
    INTER_INS_JUMP_CONDITION_NEGATIVE,
} inter_ins_jump_condition_t;

typedef struct inter_ins_jump {
    inter_ins_jump_condition_t jump_condition;
    char *to;
    bool subroutine;
} inter_ins_jump_t;

typedef struct inter_ins_return {
    bool are_you_sure_about_that;
} inter_ins_return_t;

typedef enum inter_operator {
    INTER_OPERATOR_UN_ADDR_OF,
    INTER_OPERATOR_UN_DEREFERENCE,
    INTER_OPERATOR_UN_INCREMENT,
    INTER_OPERATOR_UN_DECREMENT,
    INTER_OPERATOR_UN_NEGATE,
    INTER_OPERATOR_UN_INVERT,
    // use this in combination with & | ~ instead of && || !
    // A != 0
    // maybe implement by +255 (must be zero if no carry -> STAT & 0b1)
    INTER_OPERATOR_UN_TO_BOOL,
    INTER_OPERATOR_BI_ADD,
    INTER_OPERATOR_BI_SUB,
    INTER_OPERATOR_BI_MUL,
    INTER_OPERATOR_BI_DIV,
    INTER_OPERATOR_BI_MOD,
    INTER_OPERATOR_BI_AND,
    INTER_OPERATOR_BI_OR,
    INTER_OPERATOR_BI_XOR,
    INTER_OPERATOR_BI_SHL,
    INTER_OPERATOR_BI_SHR,
    INTER_OPERATOR_BI_LESS,
    INTER_OPERATOR_BI_LESS_EQ,
    INTER_OPERATOR_BI_GREATER,
    INTER_OPERATOR_BI_GREATER_EQ,
    INTER_OPERATOR_BI_EQUAL,
    INTER_OPERATOR_BI_UNEQUAL,
} inter_operator_t;

typedef struct inter_ins_operation {
    char *to;
    char *a;
    inter_operator_t op;
    char *b;
    cmp_parser_pure_type_t type_a;
    cmp_parser_pure_type_t type_b;
    bool op2;
} inter_ins_operation_t;

// operate (operands are in reversed order) -> A
typedef struct inter_ins_stack_operation {
    char *to;
    inter_operator_t op;
    cmp_parser_pure_type_t type_a;
    cmp_parser_pure_type_t type_b;
    bool op2;
} inter_ins_stack_operation_t;

typedef struct inter_ins {
    char *label;
    inter_ins_type_t type;
    void *ins;
} inter_ins_t;

// ACTUAL PARSER.H STARTS HERE

#define PARSER_VAR_NAME_PREFIX "HERA_"
#define PARSER_VAR_NAME_STACK_OP_TEMP (PARSER_VAR_NAME_PREFIX "stack_op_temp")
#define PARSER_VAR_NAME_STACK_OP_UN_PREFIX_TEMP (PARSER_VAR_NAME_PREFIX "stack_op_temp")
#define PARSER_VAR_NAME_STACK_OP_UN_SUFFIX_TEMP (PARSER_VAR_NAME_PREFIX "stack_op_temp")

typedef struct parser_state {
    bool error;
    inter_ins_t *ins;
    uint32_t ins_count;
    token_t *tokens;
    uint32_t tokens_count;
    uint32_t i;

    // differentiate:
    // - expressions (including assignment), this is most instructions
    // - expressions + declaration (also allowed in for-loop init)
    // - expressions + declaration + typedef + flow (function body)
    // - declaration + typedef + func definition/declaration (file)
    bool allow_declaration;
    bool allow_typedefs;
    bool allow_function;
    bool allow_expression;
    bool allow_flow_control;
} parser_state_t;

typedef enum cmp_parser_type_modifier {
    PARSER_MODIFIER_NONE,
    PARSER_MODIFIER_SIGNED,
    PARSER_MODIFIER_UNSIGNED,
    PARSER_MODIFIER_STATIC,
    PARSER_MODIFIER_VOLATILE,
    PARSER_MODIFIER_CONST,
    PARSER_MODIFIER_STRUCT,
} cmp_parser_type_modifier_t;

#define CMP_INTER_PREFIX_USER "hera_user_"
#define CMP_PARSER_TYPE_MAX_MODIFIERS 5

typedef struct cmp_parser_type {
    cmp_parser_type_modifier_t modifiers[CMP_PARSER_TYPE_MAX_MODIFIERS];
    cmp_parser_pure_type_t pure_type;
    char *other_type;
    uint8_t pointers;
} cmp_parser_type_t;

typedef struct cmp_parser_value {
    bool valid;
    cmp_parser_type_t type;
    uint16_t i;
} cmp_parser_value_t;

token_t *cmp_parser_get_token(parser_state_t *state, int32_t offset) {
    if (state->i + offset < 0) {
        return &state->tokens[0];
    } else if (state->i + offset >= state->tokens_count) {
        return &state->tokens[state->tokens_count - 1];
    } else {
        return &state->tokens[state->i + offset];
    }
}

bool cmp_parser_add_ins(parser_state_t *state, char *label, inter_ins_type_t type, void *ins, size_t ins_size) {
    state->ins_count++;
    state->ins = realloc(state->ins, state->ins_count * sizeof(inter_ins_t));
    if (state->ins == NULL) {
        printf(ERROR "out of memory");
        state->error = true;
        return false;
    }
    state->ins[state->ins_count - 1].label = label;
    state->ins[state->ins_count - 1].type = type;
    state->ins[state->ins_count - 1].ins = malloc(ins_size);
    if (state->ins[state->ins_count - 1].ins == NULL) {
        printf(ERROR "out of memory");
        state->error = true;
        return false;
    }
    memcpy(state->ins[state->ins_count - 1].ins, ins, ins_size);
    return true;
}

char *cmp_parser_new_temp(parser_state_t *state, const char *prefix) {
    char *prefix_num_buf = malloc(strlen(prefix + 9));
    if (prefix_num_buf == NULL) {
        printf(ERROR "out of memory");
        state->error = true;
        return NULL;
    }
    for (uint32_t num = 0; num < 10000000; num++) {
        sprintf(prefix_num_buf, "%s_%u", prefix, num);
        bool name_free = true;
        for (uint32_t i = 0; i < state->ins_count; i++) {
            if (state->ins[i].type == INTER_INS_DECLARATION) {
                if (strcmp(((inter_ins_declaration_t *)state->ins[i].ins)->name, prefix_num_buf) == 0) {
                    name_free = false;
                    break;
                }
            }
        }
        if (name_free) {
            return prefix_num_buf;
        }
    }

    free(prefix_num_buf);
    return NULL;
}

// returns index of token for closing bracket, 0 if not found
uint32_t cmp_parser_find_closing_bracket(parser_state_t *state) {
    uint32_t level = 0;
    for (uint32_t offset = 0; offset < state->tokens_count; offset++) {
        token_t *current_token = cmp_parser_get_token(state, offset);
        switch (current_token->type) {
            case TOKEN_EOF:
                return 0;
            case TOKEN_BRACKET_R_L:
            case TOKEN_BRACKET_C_L:
            case TOKEN_BRACKET_S_L:
                level++;
                break;
            case TOKEN_BRACKET_R_R:
            case TOKEN_BRACKET_C_R:
            case TOKEN_BRACKET_S_R:
                if (level-- <= 1)
                    return state->i + offset;
                break;
            default:
                break;
        }
    }
    return 0;
}

// TODO: first parse each constant/variable/function call as typed value
// then consider operand types for each operation
//   -> which determines push/pop operations (especially how many times)
//   -> which returns a resulting value type

cmp_parser_value_t cmp_parser_get_value(token_t *t) {
    if (t == NULL) {
        return (cmp_parser_value_t){
            .valid = false,
        };
    }
    switch (t->type) {
        case TOKEN_INT_BIN: {
            uint16_t n = 0;
            for (uint32_t i = 0; i < strlen(t->str); i++) {
                n += t->str[i] - '0';
                n *= 2;
            }
            return (cmp_parser_value_t){
                .valid = true,
                .type = {.pure_type = PARSER_TYPE_UINT16, .pointers = 0},
                .i = n,
            };
        }
        case TOKEN_INT_OCT: {
            uint16_t n = 0;
            for (uint32_t i = 0; i < strlen(t->str); i++) {
                n += t->str[i] - '0';
                n *= 8;
            }
            return (cmp_parser_value_t){
                .valid = true,
                .type = {.pure_type = PARSER_TYPE_UINT16, .pointers = 0},
                .i = n,
            };
        }
        case TOKEN_INT_DEC: {
            uint16_t n = 0;
            for (uint32_t i = 0; i < strlen(t->str); i++) {
                n += t->str[i] - '0';
                n *= 10;
            }
            return (cmp_parser_value_t){
                .valid = true,
                .type = {.pure_type = PARSER_TYPE_UINT16, .pointers = 0},
                .i = n,
            };
        }
        case TOKEN_INT_HEX: {
            uint16_t n = 0;
            for (uint32_t i = 0; i < strlen(t->str); i++) {
                if (t->str[i] >= 'a' && t->str[i] <= 'f') {
                    n += t->str[i] - 'a' + 10;
                } else if (t->str[i] >= 'A' && t->str[i] <= 'F') {
                    n += t->str[i] - 'A' + 10;
                } else {
                    n += t->str[i] - '0';
                }
                n *= 16;
            }
            return (cmp_parser_value_t){
                .valid = true,
                .type = {.pure_type = PARSER_TYPE_UINT16, .pointers = 0},
                .i = n,
            };
        }
        case TOKEN_CHAR: {
            return (cmp_parser_value_t){
                .valid = true,
                .type = {.pure_type = PARSER_TYPE_INT8, .pointers = 0},
                .i = t->str[0],
            };
        }
        case TOKEN_STRING: {
            // TODO: call constant allocator (has to be placed in ram by init routine, either at start or (preferably) at assignment)
            // and return address
            return (cmp_parser_value_t){
                .valid = true,
                .type = {.pure_type = PARSER_TYPE_INT8, .pointers = 1},
                .i = t->str[0],
            };
        }
        default:
            return (cmp_parser_value_t){
                .valid = false,
            };
    }
}

// returns false if not a variable, constant or function call
bool cmp_parser_push_value(parser_state_t *state) {
    token_t *t = cmp_parser_get_token(state, 0);
    cmp_parser_value_t constant_value = cmp_parser_get_value(t);
    if (constant_value.valid) {
        cmp_parser_add_ins(state, NULL, INTER_INS_DECLARATION, &(inter_ins_declaration_t){
            .name = "temp_const",
            .type = constant_value.type.pure_type,
            .fixed = false,
            .fixed_address = 0x0000,
        }, sizeof(inter_ins_push_t));
        cmp_parser_add_ins(state, NULL, INTER_INS_ASSIGNMENT, &(inter_ins_assignment_t){
            .to = "temp_const",
            .from_constant = constant_value.i,
            .assignment_source = INTER_INS_ASSIGNMENT_SOURCE_CONSTANT,
        }, sizeof(inter_ins_assignment_t));
        cmp_parser_add_ins(state, NULL, INTER_INS_PUSH, &(inter_ins_push_t){
            .from = "temp_const",
        }, sizeof(inter_ins_push_t));
        return true;
    } else if (t->type == TOKEN_KEYWORD) {
        // TODO: variable or function call?
        cmp_parser_add_ins(state, NULL, INTER_INS_PUSH, &(inter_ins_push_t){ .from = t->str }, sizeof(inter_ins_push_t));
        return true;
    } else {
        return false;
    }
}

// returns false if not an expression
bool cmp_parser_parse_expression(parser_state_t *state) {
    uint32_t i_start = state->i;

    // TEMP: simple function call
    if (cmp_parser_get_token(state, 0)->type == TOKEN_KEYWORD &&
        cmp_parser_get_token(state, 1)->type == TOKEN_BRACKET_R_L) {
        char *func_name = cmp_parser_get_token(state, 0)->str;
        state->i += 2;
        while (cmp_parser_get_token(state, 0)->type != TOKEN_BRACKET_R_R) {
            // TODO: argument count and type checking
            if (!cmp_parser_parse_expression(state)) {
                printf(TOKEN_POS_FORMAT ERROR "expected function parameter or closing bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(state, 0)));
                return false;
            }
            if (cmp_parser_get_token(state, 0)->type == TOKEN_COMMA) {
                state->i++;
            } else if (cmp_parser_get_token(state, 0)->type != TOKEN_BRACKET_R_R) {
                printf(TOKEN_POS_FORMAT ERROR "expected comma or closing bracket after parameter" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(state, 0)));
                return false;
            }
        }
        state->i++;
        if (!cmp_parser_add_ins(state, NULL, INTER_INS_JUMP, &(inter_ins_jump_t){
            .jump_condition = INTER_INS_JUMP_CONDITION_NONE,
            .to = func_name,
            .subroutine = true,
        }, sizeof(inter_ins_jump_t))) {
            return false;
        }

        return true;
    }

    // TEMP: support prefix unary operator
    if (cmp_tokenizer_precedence_prefix_un_operator(cmp_parser_get_token(state, 0)->type) != 0) {
        token_t *t_operator = cmp_parser_get_token(state, 0);
        state->i++;
        if (!cmp_parser_push_value(state)) {
            state->i = i_start;
            return false;
        }
        state->i++;

        inter_operator_t inter_op = INTER_OPERATOR_UN_ADDR_OF;
        switch (t_operator->type) {
            case TOKEN_AMPERSAND:
                inter_op = INTER_OPERATOR_UN_ADDR_OF;
                break;
            case TOKEN_STAR:
                inter_op = INTER_OPERATOR_UN_DEREFERENCE;
                break;
            case TOKEN_DOUBLE_PLUS:
                inter_op = INTER_OPERATOR_UN_INCREMENT;
                break;
            case TOKEN_DOUBLE_MINUS:
                inter_op = INTER_OPERATOR_UN_DECREMENT;
                break;
            case TOKEN_MINUS:
                inter_op = INTER_OPERATOR_UN_NEGATE;
                break;
            case TOKEN_TILDE:
                inter_op = INTER_OPERATOR_UN_INVERT;
                break;
            default:
                printf(TOKEN_POS_FORMAT ERROR "unknown operator" ENDL, TOKEN_POS_FORMAT_VALUES(*t_operator));
                break;
        }
        // INTER_OPERATOR_UN_TO_BOOL
        cmp_parser_add_ins(state, NULL, INTER_INS_STACK_OPERATION, &(inter_ins_stack_operation_t){
            .to = "temp_result",
            .op = inter_op,
            .op2 = false,
            .type_a = PARSER_TYPE_UINT16,
        }, sizeof(inter_ins_stack_operation_t));
        cmp_parser_add_ins(state, NULL, INTER_INS_PUSH, &(inter_ins_push_t){ .from = "temp_result" }, sizeof(inter_ins_push_t));

        return true;
    }

    // TEMP: or support chain of binary operators from right to left
    uint32_t bracket_level = 0;
    for (; cmp_parser_get_token(state, 0)->type != TOKEN_EOF; state->i++) {
        bool end = false;
        switch (cmp_parser_get_token(state, 0)->type) {
            case TOKEN_BRACKET_R_L:
                bracket_level++;
                break;
            case TOKEN_BRACKET_R_R:
                end = bracket_level == 0;
                bracket_level--;
                break;
            case TOKEN_BRACKET_S_L:
                bracket_level++;
                break;
            case TOKEN_BRACKET_S_R:
                end = bracket_level == 0;
                bracket_level--;
                break;
            case TOKEN_BRACKET_C_L:
                bracket_level++;
                break;
            case TOKEN_BRACKET_C_R:
                end = bracket_level == 0;
                bracket_level--;
                break;
            case TOKEN_SEMICOLON:
                end = true;
                break;
            case TOKEN_COMMA:
                end = true;
                break;
            default:
                break;
        }
        if (end) {
            break;
        }
    }
    if (cmp_parser_get_token(state, 0)->type == TOKEN_EOF) {
        state->i = i_start;
        return false;
    }
    uint32_t i_end = state->i;
    for (uint32_t i = i_start; i < i_end - 1; i++) {
        if (state->tokens[i].type == TOKEN_KEYWORD &&
            state->tokens[i + 1].type == TOKEN_KEYWORD) {
            // is declaration (e.g. int x)
            state->i = i_start;
            return false;
        }
    }
    state->i--;
    if (!cmp_parser_push_value(state)) {
        state->i = i_start;
        return false;
    }
    state->i--;
    for (; state->i > i_start; state->i--) {
        token_t *t_operator = cmp_parser_get_token(state, 0);
        state->i--;
        if (!cmp_parser_push_value(state)) {
            state->i = i_start;
            return false;
        }
        inter_operator_t inter_op = INTER_OPERATOR_UN_ADDR_OF;
        switch (t_operator->type) {
            case TOKEN_ASSIGN:
                // TODO
                // inter_op = INTER_OPERATOR_BI_ASSIGN;
                break;
            case TOKEN_PLUS:
                inter_op = INTER_OPERATOR_BI_ADD;
                break;
            case TOKEN_MINUS:
                inter_op = INTER_OPERATOR_BI_SUB;
                break;
            case TOKEN_STAR:
                inter_op = INTER_OPERATOR_BI_MUL;
                break;
            case TOKEN_SLASH:
                inter_op = INTER_OPERATOR_BI_DIV;
                break;
            case TOKEN_PERCENT:
                inter_op = INTER_OPERATOR_BI_MOD;
                break;
            case TOKEN_AMPERSAND:
                inter_op = INTER_OPERATOR_BI_AND;
                break;
            case TOKEN_BAR:
                inter_op = INTER_OPERATOR_BI_OR;
                break;
            case TOKEN_CIRCUMFLEX:
                inter_op = INTER_OPERATOR_BI_XOR;
                break;
            case TOKEN_DOUBLE_LEFT:
                inter_op = INTER_OPERATOR_BI_SHL;
                break;
            case TOKEN_DOUBLE_RIGHT:
                inter_op = INTER_OPERATOR_BI_SHR;
                break;
            case TOKEN_LESS:
                inter_op = INTER_OPERATOR_BI_LESS;
                break;
            case TOKEN_LESS_EQ:
                inter_op = INTER_OPERATOR_BI_LESS_EQ;
                break;
            case TOKEN_GREATER:
                inter_op = INTER_OPERATOR_BI_GREATER;
                break;
            case TOKEN_GREATER_EQ:
                inter_op = INTER_OPERATOR_BI_GREATER_EQ;
                break;
            case TOKEN_EQUAL:
                inter_op = INTER_OPERATOR_BI_EQUAL;
                break;
            case TOKEN_UNEQUAL:
                inter_op = INTER_OPERATOR_BI_UNEQUAL;
                break;
            default:
                printf(TOKEN_POS_FORMAT ERROR "unknown operator" ENDL, TOKEN_POS_FORMAT_VALUES(*t_operator));
                break;
        }
        cmp_parser_add_ins(state, NULL, INTER_INS_STACK_OPERATION, &(inter_ins_stack_operation_t){
            .to = "temp_result",
            .op = inter_op,
            .op2 = true,
            .type_a = PARSER_TYPE_UINT16,
            .type_b = PARSER_TYPE_UINT16,
        }, sizeof(inter_ins_stack_operation_t));
        cmp_parser_add_ins(state, NULL, INTER_INS_PUSH, &(inter_ins_push_t){ .from = "temp_result" }, sizeof(inter_ins_push_t));
    }
    state->i = i_end;
    return true;

    /*
    TODO: function calls -> push parameters, subroutine jump, (push return value). make sure to pop all parameters and return value (or don't even push if not used)
    TODO: assignment at declaration (for now only at declaration) -> pop expression result into variable
    TODO: only expression -> pop expression result into void/temp
    */

    token_t *prefix_un_op = NULL;
    if (cmp_tokenizer_precedence_prefix_un_operator(cmp_parser_get_token(state, 0)->type) != 0) {
        prefix_un_op = cmp_parser_get_token(state, 0);
        state->i++;
    }

    if (!cmp_parser_push_value(state)) {
        state->i = i_start;
        return false;
    }

    // op
    token_t *op = cmp_parser_get_token(state, 0);
    state->i++;
    if (prefix_un_op != NULL) {
        // prefix
        char *var_temp = cmp_parser_new_temp(state, PARSER_VAR_NAME_STACK_OP_UN_PREFIX_TEMP);
        if (var_temp == NULL) {
            state->i = i_start;
            return false;
        }
        inter_operator_t inter_op;
        switch (prefix_un_op->type) {
            case TOKEN_DOUBLE_PLUS:
                inter_op = INTER_OPERATOR_UN_INCREMENT;
                break;
            case TOKEN_DOUBLE_MINUS:
                inter_op = INTER_OPERATOR_UN_DECREMENT;
                break;
            case TOKEN_PLUS:
                return true;
            case TOKEN_MINUS:
                inter_op = INTER_OPERATOR_UN_NEGATE;
                break;
            case TOKEN_STAR:
                inter_op = INTER_OPERATOR_UN_DEREFERENCE;
                break;
            case TOKEN_AMPERSAND:
                inter_op = INTER_OPERATOR_UN_ADDR_OF;
                break;
            case TOKEN_EXCLAMATION:
                // TODO: INTER_OPERATOR_UN_TO_BOOL
                inter_op = INTER_OPERATOR_UN_INVERT;
                break;
            case TOKEN_TILDE:
                inter_op = INTER_OPERATOR_UN_INVERT;
                break;
            default:
                printf("not implemented prefix unary operator %i" ENDL, prefix_un_op->type);
                break;
        }
        cmp_parser_add_ins(state, NULL, INTER_INS_STACK_OPERATION, &(inter_ins_stack_operation_t){
                                                                       .to = var_temp,
                                                                       .type_a = PARSER_TYPE_UINT16,
                                                                       .type_b = PARSER_TYPE_INVALID,
                                                                       .op2 = false,
                                                                       .op = inter_op,
                                                                   },
                           sizeof(inter_ins_stack_operation_t));
        cmp_parser_add_ins(state, NULL, INTER_INS_PUSH, &(inter_ins_push_t){
                                                            .from = var_temp,
                                                        },
                           sizeof(inter_ins_push_t));
        return true;
    } else if (cmp_tokenizer_precedence_suffix_un_operator(op->type) != 0) {
        // suffix
        char *var_temp = cmp_parser_new_temp(state, PARSER_VAR_NAME_STACK_OP_UN_SUFFIX_TEMP);
        if (var_temp == NULL) {
            state->i = i_start;
            return false;
        }
        inter_operator_t inter_op;
        switch (op->type) {
            case TOKEN_DOUBLE_PLUS:
                inter_op = INTER_OPERATOR_UN_INCREMENT;
                break;
            case TOKEN_DOUBLE_MINUS:
                inter_op = INTER_OPERATOR_UN_DECREMENT;
                break;
            default:
                printf("not implemented suffix unary operator %i" ENDL, op->type);
                break;
        }
        cmp_parser_add_ins(state, NULL, INTER_INS_STACK_OPERATION, &(inter_ins_stack_operation_t){
                                                                       .to = var_temp,
                                                                       .type_a = PARSER_TYPE_UINT16,
                                                                       .type_b = PARSER_TYPE_INVALID,
                                                                       .op2 = false,
                                                                       .op = inter_op,
                                                                   },
                           sizeof(inter_ins_stack_operation_t));
        cmp_parser_add_ins(state, NULL, INTER_INS_PUSH, &(inter_ins_push_t){
                                                            .from = var_temp,
                                                        },
                           sizeof(inter_ins_push_t));
        return true;
    } else if (cmp_tokenizer_precedence_bi_operator(op->type) == 0) {
        // expected binary op
        state->i = i_start;
        return false;
    }

    if (!cmp_parser_push_value(state)) {
        state->i = i_start;
        return false;
    }

    char *var_temp = cmp_parser_new_temp(state, PARSER_VAR_NAME_STACK_OP_TEMP);
    if (var_temp == NULL) {
        state->i = i_start;
        return false;
    }
    inter_operator_t inter_op;
    switch (op->type) {
        // TODO
        // case TOKEN_BRACKET_S_L:
        // case TOKEN_DOT:
        // case TOKEN_ARROW:
        case TOKEN_STAR:
            inter_op = INTER_OPERATOR_BI_MUL;
            break;
        case TOKEN_SLASH:
            inter_op = INTER_OPERATOR_BI_DIV;
            break;
        case TOKEN_PERCENT:
            inter_op = INTER_OPERATOR_BI_MOD;
            break;
        case TOKEN_PLUS:
            inter_op = INTER_OPERATOR_BI_ADD;
            break;
        case TOKEN_MINUS:
            inter_op = INTER_OPERATOR_BI_SUB;
            break;
        case TOKEN_DOUBLE_LEFT:
            inter_op = INTER_OPERATOR_BI_SHL;
            break;
        case TOKEN_DOUBLE_RIGHT:
            inter_op = INTER_OPERATOR_BI_SHR;
            break;
        case TOKEN_LESS:
            inter_op = INTER_OPERATOR_BI_LESS;
            break;
        case TOKEN_LESS_EQ:
            inter_op = INTER_OPERATOR_BI_LESS_EQ;
            break;
        case TOKEN_GREATER:
            inter_op = INTER_OPERATOR_BI_GREATER;
            break;
        case TOKEN_GREATER_EQ:
            inter_op = INTER_OPERATOR_BI_GREATER_EQ;
            break;
        case TOKEN_EQUAL:
            inter_op = INTER_OPERATOR_BI_EQUAL;
            break;
        case TOKEN_UNEQUAL:
            inter_op = INTER_OPERATOR_BI_UNEQUAL;
            break;
        case TOKEN_AMPERSAND:
            inter_op = INTER_OPERATOR_BI_AND;
            break;
        case TOKEN_CIRCUMFLEX:
            inter_op = INTER_OPERATOR_BI_XOR;
            break;
        case TOKEN_BAR:
            inter_op = INTER_OPERATOR_BI_OR;
            break;
        case TOKEN_DOUBLE_AMPERSAND:
            // TODO: INTER_OPERATOR_UN_TO_BOOL
            inter_op = INTER_OPERATOR_BI_AND;
            break;
        case TOKEN_DOUBLE_BAR:
            // TODO: INTER_OPERATOR_UN_TO_BOOL
            inter_op = INTER_OPERATOR_BI_OR;
            break;
        // case TOKEN_ASSIGN:
        // case TOKEN_ASSIGN_PLUS:
        // case TOKEN_ASSIGN_MINUS:
        // case TOKEN_ASSIGN_STAR:
        // case TOKEN_ASSIGN_SLASH:
        // case TOKEN_ASSIGN_PERCENT:
        // case TOKEN_ASSIGN_CIRCUMFLEX:
        // case TOKEN_ASSIGN_BAR:
        // case TOKEN_ASSIGN_AMPERSAND:
        // case TOKEN_ASSIGN_DOUBLE_LEFT:
        // case TOKEN_ASSIGN_DOUBLE_RIGHT:
        default:
            printf("not implemented operator %i" ENDL, op->type);
            break;
    }
    cmp_parser_add_ins(state, NULL, INTER_INS_STACK_OPERATION, &(inter_ins_stack_operation_t){
                                                                   .to = var_temp,
                                                                   .type_a = PARSER_TYPE_UINT16,
                                                                   .type_b = PARSER_TYPE_UINT16,
                                                                   .op2 = true,
                                                                   .op = inter_op,
                                                               },
                       sizeof(inter_ins_stack_operation_t));
    cmp_parser_add_ins(state, NULL, INTER_INS_PUSH, &(inter_ins_push_t){
                                                        .from = var_temp,
                                                    },
                       sizeof(inter_ins_push_t));
    return true;

    /*

    x + 3 * (*y)

    x     3   y
    |     |   |
    |     | deref
    |     |   |
    |    multiply
    |       |
    \- add -/
        |

    */

    // TODO: construct list of operands and operators (if operand expected, '*' is deref, not multiply)
    bool expect_operand = true;
    bool end_of_expression = false;
    for (uint32_t offset = 0; !end_of_expression && cmp_parser_get_token(state, offset)->type != TOKEN_EOF; offset++) {
        token_t *token1 = cmp_parser_get_token(state, offset);
        if (expect_operand) {
            if (cmp_tokenizer_precedence_prefix_un_operator(token1->type) != 0) {
                // TODO: add operation with following expression as operand
                //  -> always use nested calls? functions needs to end at closing bracket or operator
                //  -> parse_operand WOULD MAKE SENSE (with prefix/postfix operators including typecast -> returns expression)
                //  -> how to handle brackets in parse_operand?
                //  -> parse_operand -> expect operator, brackets or end -> repeat
                continue;
            }
            switch (token1->type) {
                case TOKEN_KEYWORD: {
                    token_t *token2 = cmp_parser_get_token(state, ++offset);
                    if (token2->type == TOKEN_BRACKET_R_L) {
                        // function call
                        // TODO: nested expression parser call, then expect closing round bracket
                        uint32_t end = cmp_parser_find_closing_bracket(state);
                    } else {
                        // variable
                        // TODO: struct access, struct indirect access, array access
                    }
                    break;
                }
                case TOKEN_INT_DEC:
                case TOKEN_INT_OCT:
                case TOKEN_INT_BIN:
                case TOKEN_INT_HEX:
                case TOKEN_FLOAT:
                case TOKEN_CHAR:
                case TOKEN_STRING:
                    break;
                case TOKEN_BRACKET_C_L:
                    // TODO: typecast expected first
                    printf(TOKEN_POS_FORMAT ERROR "compound literal not implemented" ENDL, TOKEN_POS_FORMAT_VALUES(*token1));
                    break;
                case TOKEN_BRACKET_R_L:
                    // could be typecast (try by expecting bracket-keyword-bracket, even though that may be a variable), could be order of operations (else)
                    break;
                default:
                    printf(TOKEN_POS_FORMAT ERROR "expected operand" ENDL, TOKEN_POS_FORMAT_VALUES(*token1));
                    return false;
            }
        } else {
            switch (token1->type) {
                case TOKEN_BRACKET_S_L:
                    // TODO: array access
                    // TODO: nested expression parser call, then expect closing square bracket
                    break;
                case TOKEN_DOT:
                case TOKEN_ARROW:
                    // TODO: struct access
                    break;
                case TOKEN_STAR:
                case TOKEN_SLASH:
                case TOKEN_PERCENT:
                case TOKEN_PLUS:
                case TOKEN_MINUS:
                case TOKEN_DOUBLE_LEFT:
                case TOKEN_DOUBLE_RIGHT:
                case TOKEN_LESS_EQ:
                case TOKEN_GREATER_EQ:
                case TOKEN_LESS:
                case TOKEN_GREATER:
                case TOKEN_EQUAL:
                case TOKEN_UNEQUAL:
                case TOKEN_AMPERSAND:
                case TOKEN_CIRCUMFLEX:
                case TOKEN_BAR:
                case TOKEN_DOUBLE_AMPERSAND:
                case TOKEN_DOUBLE_BAR:
                case TOKEN_ASSIGN:
                case TOKEN_ASSIGN_PLUS:
                case TOKEN_ASSIGN_MINUS:
                case TOKEN_ASSIGN_STAR:
                case TOKEN_ASSIGN_SLASH:
                case TOKEN_ASSIGN_PERCENT:
                case TOKEN_ASSIGN_CIRCUMFLEX:
                case TOKEN_ASSIGN_BAR:
                case TOKEN_ASSIGN_AMPERSAND:
                case TOKEN_ASSIGN_DOUBLE_LEFT:
                case TOKEN_ASSIGN_DOUBLE_RIGHT:
                    break;
                default:
                    state->i += offset;
                    end_of_expression = true;
                    break;
            }
        }
        expect_operand = !expect_operand;
    }
    // TODO: if list is only one operand, add and return true

    for (int8_t search_prc = 5; search_prc >= 0; search_prc--) {
        for (uint32_t exp_i = 0; exp_i < 100000; exp_i++) {
            // TODO: if current operator is of search_prc
            if (true) {
                // TODO: set in single AST element -> parse operands (1 or 2, left or right, check if list contains left or right elements) as expressions (or use AST elements from previously parsed expressions)
                //   -> TODO: recursive variant with certain range instead of i
                // TODO: somehow store AST element (copy from single element) as replacement for token index range -> use as operand from now on
            }
        }
    }
    // TODO: single element should now contain expression root node, if still empty return false

    return false;
}

// returns false if not an instruction
bool cmp_parser_parse_expression_1(parser_state_t *state) {
    // function((parameters))
    // (((modifiers) type) lvalue = ) expression
    // if (expression) {} else if {} else {}
    // while (expression) {}
    // do {} while (expression)
    // for (instruction; expression; instruction)
    // switch (expression) { case expression }
    // return expression
    // break/continue

    return true;
}

// returns false if not a declaration
bool cmp_parser_parse_declaration(parser_state_t *state, cmp_parser_type_t *decl_type, char **decl_name) {
    uint32_t i_start = state->i;

    for (uint8_t i = 0; i < CMP_PARSER_TYPE_MAX_MODIFIERS; i++) {
        decl_type->modifiers[i] = PARSER_MODIFIER_NONE;
    }
    decl_type->pointers = 0;
    decl_type->pure_type = PARSER_TYPE_INVALID;
    decl_type->other_type = NULL;

    // volatile, unsigned, static
    bool type_unsigned = false;
    for (uint8_t i = 0; i <= CMP_PARSER_TYPE_MAX_MODIFIERS; i++) {
        token_t *token_modifier = cmp_parser_get_token(state, 0);
        if (token_modifier->type == TOKEN_KEYWORD) {
            cmp_parser_type_modifier_t found_modifier = PARSER_MODIFIER_NONE;
            if (!strcmp(token_modifier->str, "signed")) {
                type_unsigned = false;
                found_modifier = PARSER_MODIFIER_SIGNED;
            } else if (!strcmp(token_modifier->str, "unsigned")) {
                type_unsigned = true;
                found_modifier = PARSER_MODIFIER_UNSIGNED;
            } else if (!strcmp(token_modifier->str, "static")) {
                found_modifier = PARSER_MODIFIER_STATIC;
            } else if (!strcmp(token_modifier->str, "volatile")) {
                found_modifier = PARSER_MODIFIER_VOLATILE;
            } else if (!strcmp(token_modifier->str, "const")) {
                found_modifier = PARSER_MODIFIER_CONST;
            } else if (!strcmp(token_modifier->str, "struct")) {
                found_modifier = PARSER_MODIFIER_STRUCT;
            } else {
                break;
            }
            if (found_modifier == PARSER_MODIFIER_NONE) {
                break;
            } else {
                if (i == CMP_PARSER_TYPE_MAX_MODIFIERS) {
                    printf("too many type modifiers!" ENDL);
                }
                decl_type->modifiers[i] = found_modifier;
                state->i++;
            }
        } else {
            break;
        }
    }

    // char, int, long long, uint32_t
    token_t *token_type = cmp_parser_get_token(state, 0);
    if (token_type->type != TOKEN_KEYWORD) {
        state->i = i_start;
        return false;
    }
    if (!strcmp(token_type->str, "void")) {
        decl_type->pure_type = PARSER_TYPE_VOID;
        state->i++;
    } else if (!strcmp(token_type->str, "char")) {
        // char = 8 bit
        decl_type->pure_type = type_unsigned ? PARSER_TYPE_UINT8 : PARSER_TYPE_INT8;
        state->i++;
    } else if (!strcmp(token_type->str, "short")) {
        // short = 8 bit
        decl_type->pure_type = type_unsigned ? PARSER_TYPE_UINT8 : PARSER_TYPE_INT8;
        state->i++;
        token_t *token_type_ext = cmp_parser_get_token(state, 0);
        if (token_type_ext->type == TOKEN_KEYWORD) {
            if (!strcmp(token_type_ext->str, "int")) {
                // short int = 8 bit
                state->i++;
            }
        }
    } else if (!strcmp(token_type->str, "int")) {
        // int = 16 bit
        decl_type->pure_type = type_unsigned ? PARSER_TYPE_UINT16 : PARSER_TYPE_INT16;
        state->i++;
    } else if (!strcmp(token_type->str, "long")) {
        // long = 32 bit
        decl_type->pure_type = type_unsigned ? PARSER_TYPE_UINT32 : PARSER_TYPE_INT32;
        state->i++;
        token_t *token_type_ext = cmp_parser_get_token(state, 0);
        if (token_type_ext->type == TOKEN_KEYWORD) {
            if (!strcmp(token_type_ext->str, "long")) {
                // long long = 64 bit
                decl_type->pure_type = type_unsigned ? PARSER_TYPE_UINT64 : PARSER_TYPE_INT64;
                state->i++;
            } else if (!strcmp(token_type_ext->str, "int")) {
                // long int = 32 bit
                state->i++;
            }
        }
    } else if (!strcmp(token_type->str, "float")) {
        // float = 32 bit
        decl_type->pure_type = PARSER_TYPE_FLOAT;
        state->i++;
    } else if (!strcmp(token_type->str, "double")) {
        // double = 64 bit
        decl_type->pure_type = PARSER_TYPE_DOUBLE;
        state->i++;
    } else {
        decl_type->pure_type = PARSER_TYPE_OTHER;
        decl_type->other_type = malloc(strlen(token_type->str) + 1);
        memcpy(decl_type->other_type, token_type->str, strlen(token_type->str) + 1);
        state->i++;
    }

    // ***
    while (cmp_parser_get_token(state, 0)->type == TOKEN_STAR) {
        decl_type->pointers++;
        state->i++;
    }

    // name
    token_t *token_name = cmp_parser_get_token(state, 0);
    if (token_name->type != TOKEN_KEYWORD) {
        free(decl_type->other_type);
        state->i = i_start;
        return false;
    }
    *decl_name = malloc(strlen(CMP_INTER_PREFIX_USER) + strlen(token_name->str) + 1);
    memcpy(*decl_name, CMP_INTER_PREFIX_USER, strlen(CMP_INTER_PREFIX_USER));
    memcpy((*decl_name) + strlen(CMP_INTER_PREFIX_USER), token_name->str, strlen(token_name->str) + 1);
    state->i++;

    // [][][]
    while (cmp_parser_get_token(state, 0)->type == TOKEN_BRACKET_S_L) {
        decl_type->pointers++;
        state->i++;
        while (cmp_parser_get_token(state, 0)->type != TOKEN_BRACKET_S_R) {
            state->i++;  // TODO: array with specified size
        }
        state->i++;
    }

    return true;
}

// returns false on error
bool cmp_parser_run(const char *f_path, inter_ins_t **ins, uint32_t *ins_count) {
    token_t *tokens1 = NULL;
    uint32_t tokens1_count = 0;

    if (!cmp_tokenizer_run(f_path, &tokens1, &tokens1_count)) {
        return false;
    }
    if (tokens1 == NULL) {
        printf("empty file?" ENDL);
        return false;
    }

    token_t *tokens2 = NULL;
    uint32_t tokens2_count = 0;
    if (!cmp_preprocessor_run(tokens1, tokens1_count, &tokens2, &tokens2_count)) {
        cmp_tokenizer_free(tokens1, tokens1_count);
        return false;
    }
    cmp_tokenizer_free(tokens1, tokens1_count);

    parser_state_t state = {
        .error = false,
        .tokens = tokens2,
        .tokens_count = tokens2_count,
        .ins = NULL,
        .ins_count = 0,
        .allow_declaration = true,
        .allow_typedefs = true,
        .allow_function = true,
        .allow_expression = false,
        .allow_flow_control = false,
    };

    /*
    TODO:

    index tree:
    [1, 2, 5, 1, 3]
    hera_file_x_function_x_loop2_if5_loop1_loop3
      -> inter label

    set function name as prefix
    push new loop
    pop/increment ending loop (pop by setting the next nested one to 0?)
    */

    while (cmp_parser_get_token(&state, 0)->type != TOKEN_EOF && !state.error) {
        if (cmp_parser_get_token(&state, 0)->type == TOKEN_KEYWORD) {
            token_t *token_next = cmp_parser_get_token(&state, 0);
            if (!strcmp(token_next->str, "typedef")) {
                if (!state.allow_typedefs) {
                    printf(TOKEN_POS_FORMAT ERROR "typedef at this point is inconceivable" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                // TODO: typedef
                continue;
            } else if (!strcmp(token_next->str, "if")) {
                if (!state.allow_flow_control) {
                    printf(TOKEN_POS_FORMAT ERROR "if statement at this point is inconceivable" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_L) {
                    printf(TOKEN_POS_FORMAT ERROR "expected opening bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_parse_expression(&state)) {
                    printf(TOKEN_POS_FORMAT ERROR "expected condition expression" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_add_ins(&state, NULL, INTER_INS_POP, &(inter_ins_pop_t){ .to = NULL }, sizeof(inter_ins_pop_t))) {
                    return false;
                }
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_R) {
                    printf(TOKEN_POS_FORMAT ERROR "expected closing bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                // TODO: evaluate condition by pop and branch
                if (cmp_parser_get_token(&state, 0)->type == TOKEN_BRACKET_C_L) {
                    // TODO: body
                    while (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_C_R) {
                        state.i++;
                    }
                    state.i++;
                }
                if (cmp_parser_get_token(&state, 0)->type == TOKEN_BRACKET_C_R) {
                    continue;
                }
                // TODO: single expression body
                // TODO: EXPECT EXPRESSION RIGHT HERE
                continue;
            } else if (!strcmp(token_next->str, "while")) {
                if (!state.allow_flow_control) {
                    printf(TOKEN_POS_FORMAT ERROR "while loop at this point is inconceivable" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_L) {
                    printf(TOKEN_POS_FORMAT ERROR "expected opening bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_parse_expression(&state)) {
                    printf(TOKEN_POS_FORMAT ERROR "expected condition expression" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_add_ins(&state, NULL, INTER_INS_POP, &(inter_ins_pop_t){ .to = NULL }, sizeof(inter_ins_pop_t))) {
                    return false;
                }
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_R) {
                    printf(TOKEN_POS_FORMAT ERROR "expected closing bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                // TODO: evaluate condition by pop and branch
                if (cmp_parser_get_token(&state, 0)->type == TOKEN_BRACKET_C_L) {
                    // TODO: body
                    while (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_C_R) {
                        state.i++;
                    }
                    state.i++;
                }
                if (cmp_parser_get_token(&state, 0)->type == TOKEN_BRACKET_C_R) {
                    continue;
                }
                // TODO: single expression body
                // TODO: EXPECT EXPRESSION RIGHT HERE
                continue;
            } else if (!strcmp(token_next->str, "do")) {
                if (!state.allow_flow_control) {
                    printf(TOKEN_POS_FORMAT ERROR "do-while loop at this point is inconceivable" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_C_L) {
                    printf(TOKEN_POS_FORMAT ERROR "expected opening curly bracket after \"do\"" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                // TODO: body
                while (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_C_R) {
                    state.i++;
                }
                state.i++;
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_KEYWORD) {
                    printf(TOKEN_POS_FORMAT ERROR "expected ending \"while\"" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (strcmp(cmp_parser_get_token(&state, 0)->str, "while")) {
                    printf(TOKEN_POS_FORMAT ERROR "expected ending \"while\"" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_L) {
                    printf(TOKEN_POS_FORMAT ERROR "expected opening bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_parse_expression(&state)) {
                    printf(TOKEN_POS_FORMAT ERROR "expected condition expression" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_add_ins(&state, NULL, INTER_INS_POP, &(inter_ins_pop_t){ .to = NULL }, sizeof(inter_ins_pop_t))) {
                    return false;
                }
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_R) {
                    printf(TOKEN_POS_FORMAT ERROR "expected closing bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_SEMICOLON) {
                    printf(TOKEN_POS_FORMAT ERROR "expected semicolon" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                // TODO: evaluate condition by pop and branch
                continue;
            } else if (!strcmp(token_next->str, "for")) {
                if (!state.allow_flow_control) {
                    printf(TOKEN_POS_FORMAT ERROR "for loop at this point is inconceivable" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_L) {
                    printf(TOKEN_POS_FORMAT ERROR "expected opening bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_parse_expression(&state)) {
                    printf(TOKEN_POS_FORMAT ERROR "expected init expression" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_add_ins(&state, NULL, INTER_INS_POP, &(inter_ins_pop_t){ .to = NULL }, sizeof(inter_ins_pop_t))) {
                    return false;
                }
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_SEMICOLON) {
                    printf(TOKEN_POS_FORMAT ERROR "expected semicolon" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                if (!cmp_parser_parse_expression(&state)) {
                    printf(TOKEN_POS_FORMAT ERROR "expected condition expression" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_add_ins(&state, NULL, INTER_INS_POP, &(inter_ins_pop_t){ .to = NULL }, sizeof(inter_ins_pop_t))) {
                    return false;
                }
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_SEMICOLON) {
                    printf(TOKEN_POS_FORMAT ERROR "expected semicolon" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                if (!cmp_parser_parse_expression(&state)) {
                    printf(TOKEN_POS_FORMAT ERROR "expected loop expression" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_add_ins(&state, NULL, INTER_INS_POP, &(inter_ins_pop_t){ .to = NULL }, sizeof(inter_ins_pop_t))) {
                    return false;
                }
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_R) {
                    printf(TOKEN_POS_FORMAT ERROR "expected closing bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;

                // TODO: evaluate condition by pop and branch, execute rest
                if (cmp_parser_get_token(&state, 0)->type == TOKEN_BRACKET_C_L) {
                    // TODO: body
                    while (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_C_R) {
                        state.i++;
                    }
                    state.i++;
                }
                if (cmp_parser_get_token(&state, 0)->type == TOKEN_BRACKET_C_R) {
                    continue;
                }
                // TODO: single expression body
                // TODO: EXPECT EXPRESSION RIGHT HERE
                continue;
            } else if (!strcmp(token_next->str, "switch")) {
                if (!state.allow_flow_control) {
                    printf(TOKEN_POS_FORMAT ERROR "switch at this point is inconceivable" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_L) {
                    printf(TOKEN_POS_FORMAT ERROR "expected opening bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_parse_expression(&state)) {
                    printf(TOKEN_POS_FORMAT ERROR "expected value expression" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (!cmp_parser_add_ins(&state, NULL, INTER_INS_POP, &(inter_ins_pop_t){ .to = NULL }, sizeof(inter_ins_pop_t))) {
                    return false;
                }
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_R) {
                    printf(TOKEN_POS_FORMAT ERROR "expected closing bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                // TODO: find cases, pop to function vector table
                if (cmp_parser_get_token(&state, 0)->type == TOKEN_BRACKET_C_L) {
                    // TODO: body
                    while (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_C_R) {
                        state.i++;
                    }
                    state.i++;
                }
                continue;
            } else if (!strcmp(token_next->str, "break")) {
                state.i++;
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_SEMICOLON) {
                    printf(TOKEN_POS_FORMAT ERROR "expected semicolon" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                // TODO
                continue;
            } else if (!strcmp(token_next->str, "continue")) {
                state.i++;
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_SEMICOLON) {
                    printf(TOKEN_POS_FORMAT ERROR "expected semicolon" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                // TODO
                continue;
            } else if (!strcmp(token_next->str, "return")) {
                state.i++;
                bool has_value = false;
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_SEMICOLON) {
                    if (!cmp_parser_parse_expression(&state)) {
                        printf(TOKEN_POS_FORMAT ERROR "expected expression or semicolon after return" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                        return false;
                    }
                    has_value = true;
                }
                state.i++;
                if (has_value) {
                }
                // TODO
                continue;
            }
        }
        if (cmp_parser_get_token(&state, 0)->type == TOKEN_BRACKET_C_R) {
            if (!state.allow_function) {
                state.allow_declaration = true;
                state.allow_typedefs = true;
                state.allow_function = true;
                state.allow_expression = false;
                state.allow_flow_control = false;
                state.i++;
                continue;
            } else {
                printf(TOKEN_POS_FORMAT ERROR "unexpected closing bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                return false;
            }
        }
        cmp_parser_type_t decl_type;
        char *decl_name;
        if (cmp_parser_parse_declaration(&state, &decl_type, &decl_name)) {
            if (cmp_parser_get_token(&state, 0)->type == TOKEN_BRACKET_R_L) {
                if (!state.allow_function) {
                    printf(TOKEN_POS_FORMAT ERROR "function declaration at this point is inconceivable" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                // function declaration
                state.i++;
                while (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_R) {
                    cmp_parser_type_t arg_type;
                    char *arg_name;
                    if (!cmp_parser_parse_declaration(&state, &arg_type, &arg_name)) {
                        printf(TOKEN_POS_FORMAT ERROR "expected valid argument declaration" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                        return false;
                    }
                    if (cmp_parser_get_token(&state, 0)->type == TOKEN_COMMA) {
                        state.i++;
                    } else if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_R_R) {
                        printf(TOKEN_POS_FORMAT ERROR "expected comma or closing bracket" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                        return false;
                    }
                }
                state.i++;
                if (cmp_parser_get_token(&state, 0)->type == TOKEN_SEMICOLON) {
                    // TODO: store function declaration (this should be done anyway to typecheck and stuff)
                } else if (cmp_parser_get_token(&state, 0)->type != TOKEN_BRACKET_C_L) {
                    printf(TOKEN_POS_FORMAT ERROR "expected opening curly bracket for function body" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                // TODO: either parse if/while/for body in place (meh) or count number of nested blocks to know when to return to file-level context
                state.allow_declaration = true;
                state.allow_typedefs = true;
                state.allow_function = false;
                state.allow_expression = true;
                state.allow_flow_control = true;
                continue;
            } else if (cmp_parser_get_token(&state, 0)->type == TOKEN_ASSIGN || cmp_parser_get_token(&state, 0)->type == TOKEN_SEMICOLON) {
                // TODO: variable modifiers
                if (!cmp_parser_add_ins(&state, NULL, INTER_INS_DECLARATION, &(inter_ins_declaration_t){
                    .fixed = false,
                    .fixed_address = 0x0000,
                    .name = decl_name,
                    .type = decl_type.pure_type,
                }, sizeof(inter_ins_declaration_t))) {
                    return false;
                }
                if (!state.allow_declaration) {
                    printf(TOKEN_POS_FORMAT ERROR "variable declaration at this point is inconceivable" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                if (cmp_parser_get_token(&state, 0)->type == TOKEN_ASSIGN) {
                    state.i++;
                    if (!cmp_parser_parse_expression(&state)) {
                        printf(TOKEN_POS_FORMAT ERROR "variable assignment expected expression" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                        return false;
                    }
                    if (cmp_parser_get_token(&state, 0)->type != TOKEN_SEMICOLON) {
                        printf(TOKEN_POS_FORMAT ERROR "expected semicolon after variable assignment" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                        return false;
                    }
                    state.i++;
                    if (!cmp_parser_add_ins(&state, NULL, INTER_INS_POP, &(inter_ins_pop_t){
                        .to = decl_name,
                    }, sizeof(inter_ins_pop_t))) {
                        return false;
                    }
                    continue;
                } else if (cmp_parser_get_token(&state, 0)->type == TOKEN_SEMICOLON) {
                    state.i++;
                    continue;
                } else {
                    printf(TOKEN_POS_FORMAT ERROR "expected assignment or semicolon after variable declaration" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
            }
        }
        if (state.allow_expression) {
            if (cmp_parser_parse_expression(&state)) {
                if (cmp_parser_get_token(&state, 0)->type != TOKEN_SEMICOLON) {
                    printf(TOKEN_POS_FORMAT ERROR "expected semicolon" ENDL, TOKEN_POS_FORMAT_VALUES(*cmp_parser_get_token(&state, 0)));
                    return false;
                }
                state.i++;
                if (!cmp_parser_add_ins(&state, NULL, INTER_INS_POP, &(inter_ins_pop_t){ .to = "temp_expression" }, sizeof(inter_ins_pop_t))) {
                    return false;
                }
                continue;
            }
        }
        // TODO: all directives should be resolved in preprocessor i think
        if (cmp_parser_get_token(&state, 0)->type == TOKEN_DIRECTIVE) {
            state.i += 2;
            continue;
        }
        printf("skipping token %i" ENDL, cmp_parser_get_token(&state, 0)->type);
        state.i++;
    }

    cmp_tokenizer_free(tokens2, tokens2_count);

    *ins = state.ins;
    *ins_count = state.ins_count;

    if (state.error) {
        printf(ERROR "parser error");
        return false;
    }

    return true;
}

void cmp_inter_debug_print_pure_type(cmp_parser_pure_type_t pure_type, char *other_type) {
    switch (pure_type) {
        case PARSER_TYPE_INVALID:
            printf("invalid type?");
            break;
        case PARSER_TYPE_VOID:
            printf("void");
            break;
        case PARSER_TYPE_UINT8:
            printf("uint8");
            break;
        case PARSER_TYPE_UINT16:
            printf("uint16");
            break;
        case PARSER_TYPE_UINT32:
            printf("uint32");
            break;
        case PARSER_TYPE_UINT64:
            printf("uint64");
            break;
        case PARSER_TYPE_INT8:
            printf("int8");
            break;
        case PARSER_TYPE_INT16:
            printf("int16");
            break;
        case PARSER_TYPE_INT32:
            printf("int32");
            break;
        case PARSER_TYPE_INT64:
            printf("int64");
            break;
        case PARSER_TYPE_FLOAT:
            printf("fp_single");
            break;
        case PARSER_TYPE_DOUBLE:
            printf("fp_double");
            break;
        case PARSER_TYPE_OTHER:
            printf("\"%s\"", other_type);
            break;
        default:
            printf("unknown type?");
            break;
    }
}

void cmp_inter_debug_print_type(cmp_parser_type_t type) {
    for (uint8_t i = 0; i < CMP_PARSER_TYPE_MAX_MODIFIERS && type.modifiers[i] != PARSER_MODIFIER_NONE; i++) {
        switch (type.modifiers[i]) {
            case PARSER_MODIFIER_SIGNED:
                printf("signed ");
                break;
            case PARSER_MODIFIER_UNSIGNED:
                printf("unsigned ");
                break;
            case PARSER_MODIFIER_STATIC:
                printf("static ");
                break;
            case PARSER_MODIFIER_VOLATILE:
                printf("volatile ");
                break;
            case PARSER_MODIFIER_CONST:
                printf("const ");
                break;
            default:
                break;
        }
    }
    cmp_inter_debug_print_pure_type(type.pure_type, type.other_type);
    for (uint8_t i = 0; i < type.pointers; i++) {
        printf("*");
    }
}

void cmp_inter_debug_print_un_operator(inter_operator_t op) {
    switch (op) {
        case INTER_OPERATOR_UN_ADDR_OF:
            printf("&");
            break;
        case INTER_OPERATOR_UN_DEREFERENCE:
            printf("*");
            break;
        case INTER_OPERATOR_UN_INCREMENT:
            printf("++");
            break;
        case INTER_OPERATOR_UN_DECREMENT:
            printf("--");
            break;
        case INTER_OPERATOR_UN_NEGATE:
            printf("-");
            break;
        case INTER_OPERATOR_UN_INVERT:
            printf("~");
            break;
        case INTER_OPERATOR_UN_TO_BOOL:
            printf("(bool)");
            break;
        default:
            printf("unknown unary operator %i?", op);
            break;
    }
}

void cmp_inter_debug_print_bi_operator(inter_operator_t op) {
    switch (op) {
        case INTER_OPERATOR_BI_ADD:
            printf("+");
            break;
        case INTER_OPERATOR_BI_SUB:
            printf("-");
            break;
        case INTER_OPERATOR_BI_MUL:
            printf("*");
            break;
        case INTER_OPERATOR_BI_DIV:
            printf("/");
            break;
        case INTER_OPERATOR_BI_MOD:
            printf("%%");
            break;
        case INTER_OPERATOR_BI_AND:
            printf("&");
            break;
        case INTER_OPERATOR_BI_OR:
            printf("|");
            break;
        case INTER_OPERATOR_BI_XOR:
            printf("^");
            break;
        case INTER_OPERATOR_BI_SHL:
            printf("<<");
            break;
        case INTER_OPERATOR_BI_SHR:
            printf(">>");
            break;
        case INTER_OPERATOR_BI_LESS:
            printf("<");
            break;
        case INTER_OPERATOR_BI_LESS_EQ:
            printf("<=");
            break;
        case INTER_OPERATOR_BI_GREATER:
            printf(">");
            break;
        case INTER_OPERATOR_BI_GREATER_EQ:
            printf(">=");
            break;
        case INTER_OPERATOR_BI_EQUAL:
            printf("==");
            break;
        case INTER_OPERATOR_BI_UNEQUAL:
            printf("!=");
            break;
        default:
            printf("unknown binary operator %i?", op);
            break;
    }
}

void cmp_inter_debug_print(inter_ins_t *ins, uint32_t ins_count) {
    for (uint32_t i = 0; i < ins_count; i++) {
        if (ins[i].label != NULL) {
            printf("%s:" ENDL, ins[i].label);
        }
        printf("\t");
        switch (ins[i].type) {
            case INTER_INS_DECLARATION: {
                inter_ins_declaration_t *ins_t = (inter_ins_declaration_t *)ins[i].ins;
                printf("DECL ");
                cmp_inter_debug_print_pure_type(ins_t->type, NULL);
                printf(" %s", ins_t->name);
                if (ins_t->fixed) {
                    printf(" @ 0x%04X", ins_t->fixed_address);
                }
                printf(ENDL);
                break;
            }
            case INTER_INS_ASSIGNMENT: {
                inter_ins_assignment_t *ins_t = (inter_ins_assignment_t *)ins[i].ins;
                printf("%s = ", ins_t->to);
                switch (ins_t->assignment_source) {
                    case INTER_INS_ASSIGNMENT_SOURCE_CONSTANT:
                        printf("0x%04X (%u)", ins_t->from_constant, ins_t->from_constant);
                        break;
                    case INTER_INS_ASSIGNMENT_SOURCE_VARIABLE:
                        printf("%s", ins_t->from_variable);
                        break;
                    default:
                        printf("unknown assignment source?");
                        break;
                }
                printf(ENDL);
                break;
            }
            case INTER_INS_PUSH: {
                inter_ins_push_t *ins_t = (inter_ins_push_t *)ins[i].ins;
                printf("PUSH %s" ENDL, ins_t->from);
                break;
            }
            case INTER_INS_POP: {
                inter_ins_pop_t *ins_t = (inter_ins_pop_t *)ins[i].ins;
                printf("POP -> %s" ENDL, ins_t->to);
                break;
            }
            case INTER_INS_JUMP: {
                inter_ins_jump_t *ins_t = (inter_ins_jump_t *)ins[i].ins;
                if (ins_t->subroutine) {
                    printf("JSR %s" ENDL, ins_t->to);
                } else {
                    printf("JMP %s" ENDL, ins_t->to);
                }
                break;
            }
            case INTER_INS_RETURN: {
                // inter_ins_return_t *ins_t = (inter_ins_return_t *)ins[i].ins;
                printf("RET" ENDL);
                break;
            }
            case INTER_INS_OPERATION: {
                inter_ins_operation_t *ins_t = (inter_ins_operation_t *)ins[i].ins;
                printf("%s = ", ins_t->to);
                if (!ins_t->op2) {
                    cmp_inter_debug_print_un_operator(ins_t->op);
                    printf("(");
                    cmp_inter_debug_print_pure_type(ins_t->type_a, NULL);
                    printf(")%s" ENDL, ins_t->a);
                } else {
                    printf("(");
                    cmp_inter_debug_print_pure_type(ins_t->type_a, NULL);
                    printf(")%s ", ins_t->a);
                    cmp_inter_debug_print_bi_operator(ins_t->op);
                    printf(" (");
                    cmp_inter_debug_print_pure_type(ins_t->type_b, NULL);
                    printf(")%s" ENDL, ins_t->b);
                }
                break;
            }
            case INTER_INS_STACK_OPERATION: {
                inter_ins_stack_operation_t *ins_t = (inter_ins_stack_operation_t *)ins[i].ins;
                printf("%s = ", ins_t->to);
                if (!ins_t->op2) {
                    cmp_inter_debug_print_un_operator(ins_t->op);
                    printf("(");
                    cmp_inter_debug_print_pure_type(ins_t->type_a, NULL);
                    printf(")" ENDL);
                } else {
                    printf("(");
                    cmp_inter_debug_print_pure_type(ins_t->type_a, NULL);
                    printf(") ");
                    cmp_inter_debug_print_bi_operator(ins_t->op);
                    printf(" (");
                    cmp_inter_debug_print_pure_type(ins_t->type_b, NULL);
                    printf(")" ENDL);
                }
                break;
            }
            default:
                printf("unknown instruction type?" ENDL);
                break;
        }
    }
}

// returns false on error
bool cmp_inter_generate_context_from_file(const char *f_path, inter_context_t *context) {
    // TODO: path transformer for relative (to file or to cwd?) and absolute paths and search in include paths
    // TODO: look in cache if file re-included
    /*
    ast_element_t ast;
    bool success = cmp_parser_run(f_path, &ast);
    if (!success) {
        return false;
    }
    cmp_inter_generate_context_from_ast(&ast, context);
    */
    return true;
}

// returns false on error
bool cmp_inter_generate_from_context(inter_context_t *context, const char *func, inter_ins_t **inter_ins, uint32_t *inter_ins_count) {
    *inter_ins = NULL;
    *inter_ins_count = 0;

    /*
    TODO:
     - generate intermediate code for function "func" in context
     - resolve compile-time calls like sizeof -> check if expression is compile-time or runtime for switch-case
     - resolve constant expressions into context (enums, defines, const globals), calculate and store known value
     - resolve string literals to pointer to const
    */

    return true;
}

// returns false on error
bool cmp_inter_generate(const char *f_path, inter_ins_t **inter_ins, uint32_t *inter_ins_count) {
    inter_context_t ctx;
    cmp_inter_generate_context_from_file(f_path, &ctx);
    /*
    TODO: get all contexts that are included
    include magic:
    - compile all given .c files
    - in case of include, parse file in place (let parser build ast of external file, extend base ast by it -> recursive includes are recursive function calls)
      - care about #pragma once
      - all used functions should be *declared* in current context (not neccessarily defined -> search in all other compiled files)
    */
    // TODO: all functions at corresponding labels (temporary inter_ins_t[], then copy to big one including labels)
    cmp_inter_generate_from_context(&ctx, "main", inter_ins, inter_ins_count);
    // TODO: add code at 0x0000 to jump to label main

    return true;
}

#endif
