// ------------------------------------------------------------------------------------------------
// console/console_mock.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

void expect_console_on_keydown(uint code);
void expect_console_on_keyup(uint code);
void expect_console_on_char(char ch);

void mock_console_init();
