/********************************* FiberContext *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/
#include "FiberContext.h"

// Layout recap:
// a[0..15] @ 0..63
// ps @ 64
// pc @ 68
// sp @ 72

__attribute__((naked))
void xtensa_save_context(struct FiberContext* ctx) {
    asm volatile (
        // Save PS
        "rsr.ps a3\n"
        "s32i   a3, a2, 64\n"

        // Save GPRs a0..a15
        "s32i   a0,  a2, 0\n"
        "s32i   a1,  a2, 4\n"
        "s32i   a2,  a2, 8\n"
        "s32i   a3,  a2, 12\n"
        "s32i   a4,  a2, 16\n"
        "s32i   a5,  a2, 20\n"
        "s32i   a6,  a2, 24\n"
        "s32i   a7,  a2, 28\n"
        "s32i   a8,  a2, 32\n"
        "s32i   a9,  a2, 36\n"
        "s32i   a10, a2, 40\n"
        "s32i   a11, a2, 44\n"
        "s32i   a12, a2, 48\n"
        "s32i   a13, a2, 52\n"
        "s32i   a14, a2, 56\n"
        "s32i   a15, a2, 60\n"

        // Save resume PC (ret target) and SP mirror
        "mov    a3, a0\n"
        "s32i   a3, a2, 68\n"        // ctx->pc (resume via ret target)
        "s32i   a1, a2, 72\n"        // ctx->sp (mirror of a1)

        "ret\n"
    );
}

__attribute__((naked))
void xtensa_save_context_ISR(struct FiberContext* ctx, void* exc_frame) {
    asm volatile (
        // a2 = ctx, a3 = exc_frame

        // Save GPRs a0..a15 (current window registers)
        "s32i   a0,  a2, 0\n"
        "s32i   a1,  a2, 4\n"
        "s32i   a2,  a2, 8\n"
        "s32i   a3,  a2, 12\n"
        "s32i   a4,  a2, 16\n"
        "s32i   a5,  a2, 20\n"
        "s32i   a6,  a2, 24\n"
        "s32i   a7,  a2, 28\n"
        "s32i   a8,  a2, 32\n"
        "s32i   a9,  a2, 36\n"
        "s32i   a10, a2, 40\n"
        "s32i   a11, a2, 44\n"
        "s32i   a12, a2, 48\n"
        "s32i   a13, a2, 52\n"
        "s32i   a14, a2, 56\n"
        "s32i   a15, a2, 60\n"

        // Load PS from exception frame
        "l32i   a4, a3, " XSTR(EXC_PS_OFFSET) "\n"          // offset 0: PS
        "s32i   a4, a2, 64\n"

        // Load PC from exception frame
        "l32i   a4, a3, " XSTR(EXC_PC_OFFSET) "\n"          // offset 4: PC
        "s32i   a4, a2, 68\n"

        // Load SP from exception frame
        "l32i   a4, a3, " XSTR(EXC_SP_OFFSET) "\n"          // offset 12: SP
        "s32i   a4, a2, 72\n"

        "ret\n"
    );
}

__attribute__((naked))
void xtensa_restore_context(const struct FiberContext* ctx) {
    asm volatile (
        // Keep ctx base in a3 (do not clobber until done)
        "mov    a3, a2\n"

        // Load PS and PC first
        "l32i   a4, a3, 64\n"        // a4 = ctx->ps
        "l32i   a5, a3, 68\n"        // a5 = ctx->pc

        // Program EPC1 and PS, then sync
        "wsr.epc1 a5\n"
        "wsr.ps   a4\n"
        "rsync\n"

        // Restore SP early
        "l32i   a1, a3, 72\n"

        // Restore GPRs except a2/a3 (do those last)
        "l32i   a0,  a3, 0\n"
        "l32i   a4,  a3, 16\n"
        "l32i   a5,  a3, 20\n"
        "l32i   a6,  a3, 24\n"
        "l32i   a7,  a3, 28\n"
        "l32i   a8,  a3, 32\n"
        "l32i   a9,  a3, 36\n"
        "l32i   a10, a3, 40\n"
        "l32i   a11, a3, 44\n"
        "l32i   a12, a3, 48\n"
        "l32i   a13, a3, 52\n"
        "l32i   a14, a3, 56\n"
        "l32i   a15, a3, 60\n"

        // Restore architectural a2 and a3 at the very end
        "l32i   a2,  a3, 8\n"        // saved a2 (function arg)
        "l32i   a4,  a3, 12\n"       // saved a3
        "mov    a3,  a4\n"

        // Return from exception → jumps to EPC1 with PS applied
        "rfe\n"
    );
}

