#pragma once

#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEBUG 0
#define DEBUG_SRC "./programs/c/test.c"
#define ENDL "\r\n"
#define ERROR "\e[1;5;31mERROR:\e[m "

#include "tokenizer.c"
#include "preprocessor.c"
#include "parser.c"
#include "intermediate.c"
#include "asm.c"
