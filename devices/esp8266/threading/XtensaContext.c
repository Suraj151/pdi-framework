/******************************** XtensaContext *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/
#include "XtensaContext.h"

// Layout recap:
// a[0..15] @ 0..63
// ps @ 64
// pc @ 68
// sp @ 72

/**
 * Save the current context in the struct provided
 */
__attribute__((naked))
void xtensa_save_context(struct XtensaContext* ctx) {
    
    asm volatile (

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

        // Save PC
        "s32i   a0, a2, 68\n"

        // Save PS
        "rsr.ps a3\n"  "s32i   a3, a2, 64\n"

        // Save SAR
        "rsr.sar a3\n" "s32i   a3, a2, 76\n"

        // Save SP mirror (mirror of a1)
        "s32i   a1, a2, 72\n"

        "ret\n"
    );
}

/**
 * Restore the provided context for cooperative tasks
 */
__attribute__((naked))
void xtensa_restore_context(const struct XtensaContext* ctx) {

    asm volatile (

        // Restore SP early
        "l32i   a1, a2, 72\n"

        // Load SAR
        "l32i   a4, a2, 76\n" "wsr.sar   a4\n"

        // Restore GPRs except a2/a2 (do those last)
        "l32i   a0,  a2, 0\n"
        "l32i   a3,  a2, 12\n"
        // "l32i   a4,  a2, 16\n"
        "l32i   a5,  a2, 20\n"
        "l32i   a6,  a2, 24\n"
        "l32i   a7,  a2, 28\n"
        "l32i   a8,  a2, 32\n"
        "l32i   a9,  a2, 36\n"
        "l32i   a10, a2, 40\n"
        "l32i   a11, a2, 44\n"
        "l32i   a12, a2, 48\n"
        "l32i   a13, a2, 52\n"
        "l32i   a14, a2, 56\n"
        "l32i   a15, a2, 60\n"

        // Load PC
        "l32i   a4, a2, 68\n" "wsr.epc1  a4\n"

        // Load PS
        "l32i   a4, a2, 64\n" "wsr.ps    a4\n"

        // Restore a2 & a4 at the very end
        "l32i   a4,  a2, 16\n"
        "l32i   a2,  a2, 8\n"

        // Sync
        "rsync\n"

        // Return from exception → jumps to EPC1 with PS applied
        "rfe\n"
    );
}

/**
 * Restore the provided context for preemptive tasks from ISR
 * Note : This is not usual/standard context switch as most of cooperative tasks or other OS implements.
 * Here for arduino-esp8266 codebased arch, small reverse engineering has been done which mimic
 * the real preemptive context switch. Here exception frame has been updated in a way that the return path
 * to stub will restore the updated exception frame context.
 */
__attribute__((naked))
void xtensa_restore_context_isr(const struct XtensaContext* ctx) {

    asm volatile (

        // Backup all GPR's
        "s32i a0, a2, 84\n"
        "s32i a1, a2, 88\n"
        // "s32i a2, a2, 92\n"
        "s32i a3, a2, 96\n"
        "s32i a4, a2, 100\n"
        "s32i a5, a2, 104\n"
        "s32i a6, a2, 108\n"
        "s32i a7, a2, 112\n"
        "s32i a8, a2, 116\n"
        "s32i a9, a2, 120\n"
        "s32i a10, a2, 124\n"
        "s32i a11, a2, 128\n"
        "s32i a12, a2, 132\n"
        "s32i a13, a2, 136\n"
        "s32i a14, a2, 140\n"
        "s32i a15, a2, 144\n"

        // First cal diff count to adjust context SP
        "l32i a4, a2, 80\n"
        "sub a6, a4, a1\n"
        "addi a5, a6, 256\n"    // a5 = count (n)

        // Adjust the diff in context SP
        "l32i a4, a2, 72\n"
        "sub a4, a4, a5\n"
        "s32i a4, a2, 72\n"     // Updated context SP

        // Adjust the diff in context exc frame
        "add a4, a4, a6\n"
        "s32i a4, a2, 80\n"     // Updated context exc frame

        // Exchange data within current & context SP
        "mov a3, a1\n"          // a3 = source (current a1/sp)
        "l32i a4, a2, 72\n"     // a4 = destination (context a1/sp)
                                
        "loop_copy:\n"
        "beqz a5, done_copy\n"  // if count == 0, exit
        "l32i a6, a3, 0\n"      // load word from [a3]
        "s32i a6, a4, 0\n"      // store word to [a4]
        "addi a3, a3, 4\n"      // advance source
        "addi a4, a4, 4\n"      // advance destination
        "addi a5, a5, -4\n"     // decrement count
        "j loop_copy\n"
        "done_copy:\n"

        // Take context exc frame
        "l32i a3, a2, 80\n"

        // Replace PC
        "l32i a4, a2, 68\n" "s32i a4, a3, 0\n"

        // Replace PS
        "l32i a4, a2, 64\n" "s32i a4, a3, 4\n"

        // Replace SAR
        "l32i a4, a2, 76\n" "s32i a4, a3, 8\n"

        // Replace GPR's
        "l32i a4, a2, 0\n" "s32i a4, a3, 16\n"
        "l32i a4, a2, 8\n" "s32i a4, a3, 20\n"
        "l32i a4, a2, 12\n" "s32i a4, a3, 24\n"
        "l32i a4, a2, 16\n" "s32i a4, a3, 28\n"
        "l32i a4, a2, 20\n" "s32i a4, a3, 32\n"
        "l32i a4, a2, 24\n" "s32i a4, a3, 36\n"
        "l32i a4, a2, 28\n" "s32i a4, a3, 40\n"
        "l32i a4, a2, 32\n" "s32i a4, a3, 44\n"
        "l32i a4, a2, 36\n" "s32i a4, a3, 48\n"
        "l32i a4, a2, 40\n" "s32i a4, a3, 52\n"
        "l32i a4, a2, 44\n" "s32i a4, a3, 56\n"
        "l32i a4, a2, 48\n" "s32i a4, a3, 60\n"
        "l32i a4, a2, 52\n" "s32i a4, a3, 64\n"
        "l32i a4, a2, 56\n" "s32i a4, a3, 68\n"
        "l32i a4, a2, 60\n" "s32i a4, a3, 72\n"

        // Replace SP
        "l32i a1, a2, 72\n"

        // Restore all GPR's
        "l32i a0, a2, 84\n"
        // "l32i a1, a2, 88\n"
        // "l32i a2, a2, 92\n"
        "l32i a3, a2, 96\n"
        "l32i a4, a2, 100\n"
        "l32i a5, a2, 104\n"
        "l32i a6, a2, 108\n"
        "l32i a7, a2, 112\n"
        "l32i a8, a2, 116\n"
        "l32i a9, a2, 120\n"
        "l32i a10, a2, 124\n"
        "l32i a11, a2, 128\n"
        "l32i a12, a2, 132\n"
        "l32i a13, a2, 136\n"
        "l32i a14, a2, 140\n"
        "l32i a15, a2, 144\n"
    );
}

