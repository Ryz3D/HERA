#ifndef INC_PARSER_C
#define INC_PARSER_C

#include "includes.h"

typedef struct parser_state {
    inter_ins_t *ins;
    uint32_t ins_count;
    token_t *tokens;
    uint32_t tokens_count;
    uint32_t i;
} parser_state_t;

token_t *cmp_parser_get_token(parser_state_t *state, int32_t offset) {
    if (state->i + offset < 0) {
        return &state->tokens[0];
    } else if (state->i + offset >= state->tokens_count) {
        return &state->tokens[state->tokens_count - 1];
    } else {
        return &state->tokens[state->i + offset];
    }
}

bool cmp_parser_add_ins(parser_state_t *state, inter_ins_t ins) {
    state->ins_count++;
    state->ins = realloc(state->ins, state->ins_count * sizeof(inter_ins_t));
    if (state->ins == NULL) {
        printf(ERROR "out of memory");
        return false;
    }
    return true;
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

typedef struct cmp_parser_type {
    cmp_parser_pure_type_t pure_type;
    uint8_t pointers;
} cmp_parser_type_t;

typedef struct cmp_parser_value {
    bool valid;
    cmp_parser_type_t type;
    uint16_t i;
} cmp_parser_value_t;

bool cmp_parser_is_value(token_type_t type) {
    switch (type) {
        case TOKEN_INT_BIN:
        case TOKEN_INT_OCT:
        case TOKEN_INT_DEC:
        case TOKEN_INT_HEX:
        case TOKEN_CHAR:
        case TOKEN_STRING:
            return true;
        default:
            return false;
    }
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
        case TOKEN_INT_BIN:
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
        case TOKEN_INT_OCT:
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
        case TOKEN_INT_DEC:
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
        case TOKEN_INT_HEX:
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
        case TOKEN_CHAR:
            return (cmp_parser_value_t){
                .valid = true,
                .type = {.pure_type = PARSER_TYPE_INT8, .pointers = 0},
                .i = t->str[0],
            };
        case TOKEN_STRING:
            // TODO: call constant allocator (has to be placed in ram by init routine, either at start or (preferably) at assignment)
            // and return address
            return (cmp_parser_value_t){
                .valid = true,
                .type = {.pure_type = PARSER_TYPE_INT8, .pointers = 1},
                .i = t->str[0],
            };
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
        // TODO: intermediate instruction types for push/pop
        // TODO: intermediate instruction types for push/pop
    } else if (t->type == TOKEN_KEYWORD) {
        // TODO: variable or function call?
        // remember: you generate intermediate code, you can simply use the variable name
    }
    return false;
}

// returns false if not an expression
bool cmp_parser_parse_expression(parser_state_t *state) {
    /*
    TODO: temp algorithm
    - accept prefix unary operator
    - expect variable, constant or function call
    - if unary operator
     - process and exit
    - else
     - accept suffix unary operator
     - if unary operator
      - process and exit
     - else
      - expect binary operator
      - expect variable, constant or function call
    */

    token_t *prefix_un_op = NULL;
    if (cmp_tokenizer_precedence_prefix_un_operator(cmp_parser_get_token(state, 0)->type) != 0) {
        prefix_un_op = cmp_parser_get_token(state, 0);
        state->i++;
    }
    if (cmp_parser_get_token(state, 0)->type == TOKEN_KEYWORD) {
    } else if (cmp_parser_get_token(state, 0)->type == TOKEN_INT_BIN) {
    } else if (cmp_parser_get_token(state, 0)->type == TOKEN_INT_OCT) {
    } else if (cmp_parser_get_token(state, 0)->type == TOKEN_INT_DEC) {
    } else if (cmp_parser_get_token(state, 0)->type == TOKEN_INT_HEX) {
    }

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

// returns false on error
bool cmp_parser_run(const char *f_path, ast_element_t *ast) {
    cmp_parser_init_ast_element(ast);
    ast->type = AST_ROOT;

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
        .ast = ast,
    };

    while (state.i < state.tokens_count) {
        if (cmp_parser_parse_declaration(&state)) {
            continue;
        } else {
            printf(TOKEN_POS_FORMAT ERROR "unexpected token. expected top-level declaration" ENDL, TOKEN_POS_FORMAT_VALUES(state.tokens[state.i]));
            return false;
        }
    }

    // TODO: cmp_parser_free(ast);

    cmp_tokenizer_free(tokens2, tokens2_count);

    return true;
}

#endif
