#ifndef INC_PREPROCESSOR_C
#define INC_PREPROCESSOR_C

#pragma once

#include "includes.h"

// returns false on error
bool cmp_preprocessor_run(token_t *tokens, uint32_t tokens_count, token_t **tokens_out, uint32_t *tokens_out_count) {
    *tokens_out = malloc(tokens_count * sizeof(token_t));
    if (*tokens_out == NULL) {
        return false;
    }
    *tokens_out_count = tokens_count;

    for (uint32_t i = 0; i < tokens_count; i++) {
        (*tokens_out)[i] = tokens[i];
        if (tokens[i].allocated) {
            (*tokens_out)[i].str = malloc(strlen(tokens[i].str) + 1);
            strcpy((*tokens_out)[i].str, tokens[i].str);
        }
    }

    // TODO:
    //  - resolve defines by tokens (replacing parameter tokens)
    //  - concatenate string literals

    return true;
}

#endif
