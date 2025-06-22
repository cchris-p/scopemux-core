/**
 * Direct test processor include file to ensure proper linkage
 */
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Include the original implementation file directly to force symbol inclusion */
#include "test_processor.c"
