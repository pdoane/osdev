// ------------------------------------------------------------------------------------------------
// console/console.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

void console_init();
void console_putchar(char c);
void console_print(const char* fmt, ...);

uint console_get_cursor();
char* console_get_input_line();

void console_on_keydown(uint code);
void console_on_keyup(uint code);
void console_on_char(char ch);
