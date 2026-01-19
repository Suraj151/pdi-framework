/******************************** XtensaContext *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#ifndef __ESP866_XTENSA_CONTEXT_H__
#define __ESP866_XTENSA_CONTEXT_H__

#include <stdint.h>
#include "xtensa_context.h"

#ifdef __cplusplus
extern "C" {
#endif

struct XtensaContext {
    uint32_t a[16];   // registers a0..a15. 0...63
    uint32_t ps;      // processor status. 64
    uint32_t pc;      // program counter. 68
    uint32_t sp;      // mirror of a1. 72
    uint32_t sar;     // 76 
    uint32_t excframe; // 80
    uint32_t t[16]; //84...
    // uint32_t lcount; // 80
    // uint32_t lbeg; // 84 
    // uint32_t lend; // 88 
    // uint32_t acclo; // 92 
    // uint32_t acchi; // 96 
    // uint32_t m0; // 100 
    // uint32_t m1; // 104 
    // uint32_t m2; // 108 
    // uint32_t m3; // 112 
};
// } __attribute__((aligned(16)));

void xtensa_save_context(struct XtensaContext* ctx);
void xtensa_restore_context(const struct XtensaContext* ctx);
void xtensa_restore_context_isr(const struct XtensaContext* ctx);

#ifdef __cplusplus
}
#endif

#endif
