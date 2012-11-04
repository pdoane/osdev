// ------------------------------------------------------------------------------------------------
// console/console_mock.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

void Expect_ConsoleOnKeyDown(uint code);
void Expect_ConsoleOnKeyUp(uint code);
void Expect_ConsoleOnChar(char ch);

void Mock_ConsoleInit();
