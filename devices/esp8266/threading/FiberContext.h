/********************************* FiberContext *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#ifndef __ESP866_FIBER_CONTEXT_H__
#define __ESP866_FIBER_CONTEXT_H__

#include <stdint.h>
#include "xtensa_context.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FiberContext {
    uint32_t a[16];   // registers a0..a15. 0...63
    uint32_t ps;      // processor status. 64
    uint32_t pc;      // program counter. 68
    uint32_t sp;      // mirror of a1. 72
    // uint32_t sar; // 76 
    // uint32_t lbeg; // 80 
    // uint32_t lend; // 84 
    // uint32_t lcount; // 88
};
// } __attribute__((aligned(16)));

void xtensa_save_context(struct FiberContext* ctx);
void xtensa_restore_context(const struct FiberContext* ctx);
void xtensa_restore_from_frame();

#ifdef __cplusplus
}
#endif

#endif
