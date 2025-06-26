#ifndef INC_PARSER_C
#define INC_PARSER_C

#include "includes.h"

typedef enum cmp_parser_pure_type {
    PARSER_TYPE_INVALID,
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
    PARSER_TYPE_STRUCT,
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
    inter_ins_t *ins;
    uint32_t ins_count;
    token_t *tokens;
    uint32_t tokens_count;
    uint32_t i;
} parser_state_t;

typedef struct cmp_parser_type {
    cmp_parser_pure_type_t pure_type;
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

bool cmp_parser_add_ins(parser_state_t *state, inter_ins_type_t type, void *ins, size_t ins_size) {
    state->ins_count++;
    state->ins = realloc(state->ins, state->ins_count * sizeof(inter_ins_t));
    if (state->ins == NULL) {
        printf(ERROR "out of memory");
        return false;
    }
    state->ins[state->ins_count - 1].type = type;
    state->ins[state->ins_count - 1].ins = malloc(ins_size);
    if (state->ins[state->ins_count - 1].ins == NULL) {
        printf(ERROR "out of memory");
        return false;
    }
    memcpy(state->ins[state->ins_count - 1].ins, ins, ins_size);
    return true;
}

char *cmp_parser_new_temp(parser_state_t *state, const char *prefix) {
    char *prefix_num_buf = malloc(strlen(prefix + 9));
    if (prefix_num_buf == NULL) {
        printf(ERROR "out of memory");
        return NULL;
    }
    for (uint32_t num = 0; num < 10000000; num++) {
        sprintf(prefix_num_buf, "%s_%lu", prefix, num);
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
        // TODO: cmp_parser_add_ins(state, );
        // TODO: intermediate instruction types for math/bit operation (maybe only variable-variable, constant will be loaded into register or ram)
        //         -> this might replace unary operators in assignment ins
    } else if (t->type == TOKEN_KEYWORD) {
        // TODO: variable or function call?
        cmp_parser_add_ins(state, INTER_INS_PUSH, &(inter_ins_push_t){
            .from = t->str,
        }, sizeof(inter_ins_push_t));
        // remember: you generate intermediate code, you can simply use the variable name
    }
    return false;
}

// returns false if not an expression
bool cmp_parser_parse_expression(parser_state_t *state) {
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
        return false;
    }

    // op
    token_t *op = cmp_parser_get_token(state, 0);
    state->i++;
    if (prefix_un_op != NULL) {
        // prefix
        char *var_temp = cmp_parser_new_temp(state, PARSER_VAR_NAME_STACK_OP_UN_PREFIX_TEMP);
        if (var_temp == NULL) {
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
        cmp_parser_add_ins(state, INTER_INS_STACK_OPERATION, &(inter_ins_stack_operation_t){
            .to = var_temp,
            .type_a = PARSER_TYPE_UINT16,
            .type_b = PARSER_TYPE_INVALID,
            .op2 = false,
            .op = inter_op,
        }, sizeof(inter_ins_stack_operation_t));
        cmp_parser_add_ins(state, INTER_INS_PUSH, &(inter_ins_push_t){
            .from = var_temp,
        }, sizeof(inter_ins_push_t));
        return true;
    } else if (cmp_tokenizer_precedence_suffix_un_operator(op->type) != 0) {
        // suffix
        char *var_temp = cmp_parser_new_temp(state, PARSER_VAR_NAME_STACK_OP_UN_SUFFIX_TEMP);
        if (var_temp == NULL) {
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
        cmp_parser_add_ins(state, INTER_INS_STACK_OPERATION, &(inter_ins_stack_operation_t){
            .to = var_temp,
            .type_a = PARSER_TYPE_UINT16,
            .type_b = PARSER_TYPE_INVALID,
            .op2 = false,
            .op = inter_op,
        }, sizeof(inter_ins_stack_operation_t));
        cmp_parser_add_ins(state, INTER_INS_PUSH, &(inter_ins_push_t){
            .from = var_temp,
        }, sizeof(inter_ins_push_t));
        return true;
    } else if (cmp_tokenizer_precedence_bi_operator(op->type) == 0) {
        // expected binary op
        return false;
    }

    if (!cmp_parser_push_value(state)) {
        return false;
    }

    char *var_temp = cmp_parser_new_temp(state, PARSER_VAR_NAME_STACK_OP_TEMP);
    if (var_temp == NULL) {
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
    cmp_parser_add_ins(state, INTER_INS_STACK_OPERATION, &(inter_ins_stack_operation_t){
        .to = var_temp,
        .type_a = PARSER_TYPE_UINT16,
        .type_b = PARSER_TYPE_UINT16,
        .op2 = true,
        .op = inter_op,
    }, sizeof(inter_ins_stack_operation_t));
    cmp_parser_add_ins(state, INTER_INS_PUSH, &(inter_ins_push_t){
        .from = var_temp,
    }, sizeof(inter_ins_push_t));
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

    // differentiate:
    // - expressions (even assignment), this is most instructions
    // - expressions + declaration (also allowed in for-loop init)
    // - expressions + declaration + typedef + flow (function body)
    // - declaration + typedef + func definition/declaration (file)

    return true;
}

// returns false if not a declaration
bool cmp_parser_parse_var_declaration(parser_state_t *state) {
    // TODO: importantly increment state->i
    return true;
}

bool cmp_parser_parse_declaration(parser_state_t *state) {
    // TODO
    return false;
}

inter_ins_t test_ins[] = {
    { .label = "main", .type = INTER_INS_DECLARATION,        .ins = &(inter_ins_declaration_t){ .type = PARSER_TYPE_UINT16, .name = "x", .fixed = false } },
    { .label = NULL,   .type = INTER_INS_ASSIGNMENT,         .ins = &(inter_ins_assignment_t){ .to = "x", .from_constant = 0xfe08, .assignment_source = INTER_INS_ASSIGNMENT_SOURCE_CONSTANT } },
    { .label = NULL,   .type = INTER_INS_PUSH,               .ins = &(inter_ins_push_t){ .from = "x" } },
    { .label = NULL,   .type = INTER_INS_DECLARATION,        .ins = &(inter_ins_declaration_t){ .type = PARSER_TYPE_UINT16, .name = "y", .fixed = false } },
    { .label = NULL,   .type = INTER_INS_ASSIGNMENT,         .ins = &(inter_ins_assignment_t){ .to = "y", .from_constant = 0xffff, .assignment_source = INTER_INS_ASSIGNMENT_SOURCE_CONSTANT } },
    { .label = NULL,   .type = INTER_INS_PUSH,               .ins = &(inter_ins_push_t){ .from = "y" } },
    { .label = NULL,   .type = INTER_INS_DECLARATION,        .ins = &(inter_ins_declaration_t){ .type = PARSER_TYPE_UINT16, .name = "GPOA", .fixed = true, .fixed_address = 0x0200 } },
    { .label = NULL,   .type = INTER_INS_STACK_OPERATION,    .ins = &(inter_ins_stack_operation_t){ .op2 = true, .op = INTER_OPERATOR_BI_ADD, .to = "GPOA", .type_a = PARSER_TYPE_UINT16, .type_b = PARSER_TYPE_UINT16 } }
};

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
        .tokens = tokens2,
        .tokens_count = tokens2_count,
        .ins = NULL,
        .ins_count = 0,
    };

    while (state.i < state.tokens_count) {
        if (cmp_parser_parse_declaration(&state)) {
            continue;
        } else {
            printf(TOKEN_POS_FORMAT ERROR "unexpected token. expected top-level declaration" ENDL, TOKEN_POS_FORMAT_VALUES(state.tokens[state.i]));
            break; // return false;
        }
    }

    cmp_tokenizer_free(tokens2, tokens2_count);

    *ins = state.ins;
    *ins_count = state.ins_count;

    *ins = test_ins;
    *ins_count = 8;

    return true;
}

void cmp_inter_debug_print_type(cmp_parser_pure_type_t type) {
    switch (type) {
        case PARSER_TYPE_INVALID:
            printf("invalid type?");
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
        case PARSER_TYPE_STRUCT:
            printf("struct");
            break;
        default:
            printf("unknown type?");
            break;
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
                cmp_inter_debug_print_type(ins_t->type);
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
                    cmp_inter_debug_print_type(ins_t->type_a);
                    printf(")%s" ENDL, ins_t->a);
                } else {
                    printf("(");
                    cmp_inter_debug_print_type(ins_t->type_a);
                    printf(")%s ", ins_t->a);
                    cmp_inter_debug_print_bi_operator(ins_t->op);
                    printf(" (");
                    cmp_inter_debug_print_type(ins_t->type_b);
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
                    cmp_inter_debug_print_type(ins_t->type_a);
                    printf(")" ENDL);
                } else {
                    printf("(");
                    cmp_inter_debug_print_type(ins_t->type_a);
                    printf(") ");
                    cmp_inter_debug_print_bi_operator(ins_t->op);
                    printf(" (");
                    cmp_inter_debug_print_type(ins_t->type_b);
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
