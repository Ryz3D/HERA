#ifndef INC_PARSER_C
#define INC_PARSER_C

#include "includes.h"

typedef enum ast_type {
    // purely structural (no "token.str" content)
    AST_ROOT,                 // children: {AST_FUNCTION_DECL|AST_VARIABLE_DECL|AST_TYPEDEF}[]
    AST_EXPRESSION,           // now this is tricky (arrays, structs, struct initializers, casts, dereferencing, in/decrementing)
    AST_INSTRUCTION,          // children: (AST_BODY|AST_FUNCTION_CALL|AST_VARIABLE_DECL|AST_ASSIGNMENT|AST_EXPRESSION|AST_IF|AST_WHILE|AST_DO_WHILE|AST_FOR|AST_SWITCH)
    AST_BODY,                 // children: (AST_INSTRUCTION[])
    AST_TYPE,                 // children: AST_TYPE_NORMAL|AST_TYPE_STRUCT|AST_TYPE_ENUM
    AST_TYPE_NORMAL,          // children: (AST_C_TYPE_MODIFIER[]), AST_C_PURE_TYPE
    AST_TYPE_STRUCT,          // children: (AST_C_TYPE_MODIFIER[]), (AST_NAME), (AST_STRUCT_DECL[])
    AST_STRUCT_MEMBER,        // children: AST_TYPE, AST_C_NAME, (AST_STRUCT_MEMBER_WIDTH)
    AST_STRUCT_MEMBER_WIDTH,  // children: AST_EXPRESSION
    AST_TYPE_ENUM,            // children: (AST_C_TYPE_MODIFIER[]), (AST_NAME), (AST_ENUM_MEMBER[])
    AST_ENUM_MEMBER,          // children: AST_C_NAME, (AST_EXPRESSION)
    AST_FUNCTION_DECL,        // children: AST_TYPE, AST_C_NAME, (AST_VARIABLE_DECL[]), (AST_BODY)
    AST_RETURN,               // children: (AST_EXPRESSION)
    AST_FUNCTION_CALL,        // children: AST_C_NAME, (AST_EXPRESSION[])
    AST_VARIABLE_DECL,        // children: AST_TYPE, AST_C_NAME, (AST_EXPRESSION)
    AST_ASSIGNMENT,           // children: AST_EXPRESSION, AST_EXPRESSION
    AST_IF,                   // children: AST_EXPRESSION, AST_BODY, (AST_ELSE_IF[]), (AST_ELSE)
    AST_ELSE,                 // children: AST_BODY
    AST_ELSE_IF,              // children: AST_EXPRESSION, AST_BODY
    AST_WHILE,                // children: AST_EXPRESSION, AST_BODY
    AST_DO_WHILE,             // children: AST_BODY, AST_EXPRESSION
    AST_FOR,                  // children: AST_INSTRUCTION, AST_EXPRESSION, AST_INSTRUCTION, AST_BODY
    AST_SWITCH,               // children: AST_CASE[]
    AST_CASE,                 // children: AST_EXPRESSION, (AST_INSTRUCTION[])
    AST_TYPEDEF,              // children: AST_TYPE, AST_NAME
    // purely contentful (no children)
    AST_C_NAME,
    AST_C_TYPE_MODIFIER,
    AST_C_PURE_TYPE,
    // invalid
    AST_UNKNOWN,
} ast_type_t;

typedef struct ast_element {
    ast_type_t type;
    token_t token;
    struct ast_element *children;
    uint32_t children_count;
} ast_element_t;

typedef struct parser_state {
    ast_element_t *ast;
    token_t *tokens;
    uint32_t tokens_count;
    uint32_t i;
} parser_state_t;

token_t *cmp_parser_get_token(parser_state_t *state, uint32_t offset) {
    if (state->i + offset >= state->tokens_count) {
        return &state->tokens[state->tokens_count - 1];
    } else {
        return &state->tokens[state->i + offset];
    }
}

void cmp_parser_init_ast_element(ast_element_t *ast) {
    ast->type = AST_UNKNOWN;
    ast->token.type = TOKEN_UNKNOWN;
    ast->token.allocated = false;
    ast->token.str = NULL;
    ast->children = NULL;
    ast->children_count = 0;
}

ast_element_t *cmp_parser_add_ast_element(ast_element_t *ast) {
    ast->children_count++;
    ast->children = realloc(ast->children, ast->children_count * sizeof(ast_element_t));
    if (ast->children == NULL) {
        printf(ERROR "out of memory");
        return NULL;
    }
    return &ast->children[ast->children_count - 1];
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
                if (level-- <= 1) // should equal 1
                    return state->i + offset;
                break;
            default:
                break;
        }
    }
    return 0;
}

// returns false if not an expression
bool cmp_parser_parse_expression(parser_state_t *state) {
    // TODO: all operations as AST types (expression can contain variables, calls, literals and any operation, operations can contain expressions as operands)
    // TODO: shunting yard algorithm?
    // TODO: how is space for temporary variables calculated? probably due to tree depth (levels of nested complex expressions with child operations as both operands)
    //        -> allocate at compile-time for most complex calculation in program?
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
    if (cmp_parser_get_token(state, 0)->type == TOKEN_KEYWORD) {
        state->i++;
        if (cmp_parser_get_token(state, 0)->type == TOKEN_BRACKET_R_L) {
            // function call
            state->i = cmp_parser_find_closing_bracket(state);
            return true;
        } else {
            // variable
            return true;
        }
    } else {
        // probably literal
    }
    return false;
}

// returns false if not an instruction
bool cmp_parser_parse_instruction(parser_state_t *state) {
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
bool cmp_parser_parse_declaration(parser_state_t *state) {
    // TODO: importantly increment state->i
    // typedef
    // global variable
    // function
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
