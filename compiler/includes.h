#pragma once

#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEBUG 1
#define ENDL "\r\n"

#include "tokenizer.c"
#include "preprocessor.c"
#include "parser.c"
#include "intermediate.c"
#include "asm.c"
