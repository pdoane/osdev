// ------------------------------------------------------------------------------------------------
// console_cmd.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

typedef struct ConsoleCmd
{
    const char* name;
    void (*exec)(const char* line);
} ConsoleCmd;

extern ConsoleCmd console_cmd_table[];
