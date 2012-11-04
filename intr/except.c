// ------------------------------------------------------------------------------------------------
// intr/except.c
// ------------------------------------------------------------------------------------------------

#include "intr/except.h"
#include "console/console.h"

// ------------------------------------------------------------------------------------------------
static const char *s_exceptionDesc[20] =
{
    [0] = "Divide Error",
    [1] = "Debug",
    [2] = "Nonmaskable Interrupt",
    [3] = "Breakpoint",
    [4] = "Overflow",
    [5] = "Bound Range Exceeded",
    [6] = "Invalid Opcode",
    [7] = "Device Not Available",
    [8] = "Double Fault",
    [9] = "Coprocessor Segment Overrun",
    [10] = "Invalid TSS",
    [11] = "Segment Not Present",
    [12] = "Stack-Segment Fault",
    [13] = "General Protection",
    [14] = "Page Fault",
    [16] = "Floating Point Error",
    [17] = "Alignment Check",
    [18] = "Machine Check",
    [19] = "SIMD Exception"
};

// ------------------------------------------------------------------------------------------------
void ExceptionDump(Registers regs)
{
    const char *desc = "Unknown";
    if (regs.intNum < 20)
    {
        desc = s_exceptionDesc[regs.intNum];
    }

    // TODO - don't use console system, but write to screen directly.
    ConsolePrint("Exception: %s\n", desc);
    ConsolePrint("  rax=%016x\n", regs.rax);
    ConsolePrint("  rbx=%016x\n", regs.rbx);
    ConsolePrint("  rcx=%016x\n", regs.rcx);
    ConsolePrint("  rdx=%016x\n", regs.rdx);
    ConsolePrint("  rsi=%016x\n", regs.rsi);
    ConsolePrint("  rdi=%016x\n", regs.rdi);
    ConsolePrint("  rip=%016x\n", regs.rip);
    ConsolePrint("  rsp=%016x\n", regs.rsp);
    ConsolePrint("  cs=%02x\n", regs.cs);
    ConsolePrint("  ss=%02x\n", regs.ss);

    for (;;) {}
}