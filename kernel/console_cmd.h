// ------------------------------------------------------------------------------------------------
// console_cmd.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

typedef struct ConsoleCmd
{
    const char* name;
    void (*exec)(uint argc, const char** argv);
} ConsoleCmd;

extern ConsoleCmd console_cmd_table[];
