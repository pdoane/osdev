// ------------------------------------------------------------------------------------------------
// except.c
// ------------------------------------------------------------------------------------------------

#include "except.h"
#include "console.h"

// ------------------------------------------------------------------------------------------------
const char* s_exception_desc[20] =
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
void exception_dump(Registers regs)
{
    const char* desc = "Unknown";
    if (regs.int_num < 20)
    {
        desc = s_exception_desc[regs.int_num];
    }

    // TODO - don't use console system, but write to screen directly.
    console_print("Exception: %s\n", desc);
    console_print("  rax=%016x\n", regs.rax);
    console_print("  rbx=%016x\n", regs.rbx);
    console_print("  rcx=%016x\n", regs.rcx);
    console_print("  rdx=%016x\n", regs.rdx);
    console_print("  rsi=%016x\n", regs.rsi);
    console_print("  rdi=%016x\n", regs.rdi);
    console_print("  rip=%016x\n", regs.rip);
    console_print("  rsp=%016x\n", regs.rsp);
    console_print("  cs=%02x\n", regs.cs);
    console_print("  ss=%02x\n", regs.ss);

    for (;;) {}
}