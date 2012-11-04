// ------------------------------------------------------------------------------------------------
// console/console.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

void ConsoleInit();
void ConsolePutChar(char c);
void ConsolePrint(const char *fmt, ...);

uint ConsoleGetCursor();
char *ConsoleGetInputLine();

void ConsoleOnKeyDown(uint code);
void ConsoleOnKeyUp(uint code);
void ConsoleOnChar(char ch);
