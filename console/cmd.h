// ------------------------------------------------------------------------------------------------
// console/cmd.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

typedef struct ConsoleCmd
{
    const char* name;
    void (*exec)(uint argc, const char** argv);
} ConsoleCmd;

extern ConsoleCmd console_cmd_table[];
