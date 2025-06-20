#ifndef INC_INTERMEDIATE_C
#define INC_INTERMEDIATE_C

#include "includes.h"

typedef struct inter_context {
    // TODO: context description struct (all defined functions/variables)
    // TODO: add all variables (+functions?) to list (maybe prefix: filename_function_variable), before adding: check if name already exists and append something to differenciate
    int i;
} inter_context_t;

typedef enum inter_ins_type {
    INTER_INS_DECLARATION,
    INTER_INS_ASSIGNMENT,
    INTER_INS_JUMP,
} inter_ins_type_t;

typedef struct inter_ins_declaration {
    char *name;
    uint8_t bits;
} inter_ins_declaration_t;

typedef enum inter_ins_assignment_source {
    INTER_INS_ASSIGNMENT_SOURCE_VARIABLE,
    INTER_INS_ASSIGNMENT_SOURCE_INDIRECT,
    INTER_INS_ASSIGNMENT_SOURCE_ADDRESSOF,
    INTER_INS_ASSIGNMENT_SOURCE_INDEXED,
} inter_ins_assignment_source_t;

typedef struct inter_ins_assignment {
    char *to;
    inter_ins_assignment_source_t assignment_source;
    char *from;
    int32_t index;
} inter_ins_assignment_t;

typedef struct inter_ins_jump {
    char *to;
    bool subroutine;
} inter_ins_jump_t;

// TODO: maybe increment variable suffix if re-declared (possibly useful for temp variables from different parser functions)

typedef struct inter_ins {
   inter_ins_type_t type;
   inter_ins_declaration_t *ins_declaration;
   inter_ins_assignment_source_t *ins_assignment;
   inter_ins_jump_t *ins_jump;
} inter_ins_t;

/*
// returns false on error
bool cmp_inter_generate_context_from_ast(ast_element_t *ast, inter_context_t *context) {
    return true;
}

// returns false on error
bool cmp_inter_generate_context_from_file(const char *f_path, inter_context_t *context) {
    // TODO: path transformer for relative (to file or to cwd?) and absolute paths and search in include paths
    // TODO: look in cache if file re-included
    ast_element_t ast;
    bool success = cmp_parser_run(f_path, &ast);
    if (!success) {
        return false;
    }
    cmp_inter_generate_context_from_ast(&ast, context);
    return true;
}
*/

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
    // TODO: expression steps to ordered instructions (resolve pointers, do arithmetics)

    return true;
}

#endif
