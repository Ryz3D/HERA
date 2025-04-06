#include "includes.h"

typedef enum ast_type {
    AST_UNKNOWN,
    AST_EXPRESSION,
} ast_type_t;

typedef struct ast_element {
    ast_type_t type;
    char *str;
    struct ast_element *children;
    uint32_t children_count;
} ast_element_t;

typedef struct parser_state {
    uint32_t i;
} parser_state_t;

// expect instruction (function/for) or expect expression (assignment/argument)

bool cmp_parser_run(FILE *f, ast_element_t **ast) {
    token_t *tokens = NULL;
    uint32_t tokens_count;

    if (!cmp_tokenizer_run(f, &tokens, &tokens_count)) {
        return false;
    }
    if (tokens == NULL) {
        printf("empty file?" ENDL);
        return false;
    }

    for (uint32_t i = 0; i < tokens_count; i++)
        printf("token %u: %i" ENDL, i, tokens[i].type);

    // TODO: run preprocessor over tokens

    ast = malloc(1);

    return;
}
