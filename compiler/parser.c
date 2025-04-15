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

    cmp_tokenizer_free(tokens2, tokens2_count);

    return true;
}
