#ifndef INC_PARSER_C
#define INC_PARSER_C

#include "includes.h"

typedef enum ast_type {
    // purely structural (no "str" content)
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
    uint32_t i;
} parser_state_t;

void cmp_parser_init_ast_element(ast_element_t *ast) {
    ast->type = AST_UNKNOWN;
    ast->token.type = TOKEN_UNKNOWN;
    ast->token.allocated = false;
    ast->token.str = NULL;
    ast->children = NULL;
    ast->children_count = 0;
}

// returns false if not an expression
bool cmp_parser_parse_expression(ast_element_t **ast) {
    return true;
}

// returns false if not an instruction
bool cmp_parser_parse_instruction(ast_element_t **ast) {
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
bool cmp_parser_parse_declaration(ast_element_t **ast) {
    // typedef
    // global variable
    // function
    return true;
}

// returns false on error
bool cmp_parser_run(const char *f_path, ast_element_t **ast) {
    *ast = NULL;

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

    for (uint32_t i = 0; i < tokens2_count; i++) {
        if (tokens2[i].type < TOKEN_STR_COUNT) {
            printf("token %u: %i\t%s" ENDL, i, tokens2[i].type, token_str[tokens2[i].type]);
        } else {
            printf("token %u: %i\t%s" ENDL, i, tokens2[i].type, tokens2[i].str);
        }
    }

    ast = malloc(1);
    free(ast);

    // TODO: parse tokens using cmp_parser_parse_declaration
    // TODO: cmp_parser_free(ast);

    cmp_tokenizer_free(tokens2, tokens2_count);

    return true;
}

#endif
