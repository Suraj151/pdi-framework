/********************************* XtensaTimer *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#include "../esp8266.h"
#include "XtensaTimer.h"

static XtensaContext __isr_entry_ctx;
static os_timer_t delay_timer;

// typedef struct {
//     uint32_t pc;       // UEXC_pc
//     uint32_t ps;       // UEXC_ps
//     uint32_t sar;      // UEXC_sar
//     uint32_t vpri;     // UEXC_vpri
//     uint32_t a0;       // UEXC_a0 (CALL0)
//     uint32_t a2, a3, a4, a5;
//     uint32_t a6, a7, a8, a9, a10, a11, a12, a13, a14, a15; // CALL0
//     uint32_t exccause; // UEXC_exccause
//     // optional: lcount/lbeg/lend, MAC16 fields, pad
// } UserFrame;

/**
 * Timer1 ISR routine to use for context switch
 */
void IRAM_ATTR __attribute__((naked)) timer1_isr(void* arg, void *frame) {

    asm volatile (
        // base = &__isr_entry_ctx
        "mov a2, %0\n"
        // "mov a3, %1\n"              

        // Save exc frame
        "s32i a3, a2, 80\n"

        // Save PC
        "l32i a4, a3, 0\n" "s32i a4, a2, 68\n"

        // Save PS
        "l32i a4, a3, 4\n" "s32i a4, a2, 64\n"
        // "rsr.ps a4\n" "s32i a4, a2, 64\n"
        // "rsr.excsave1 a4\n" "s32i a4, a2, 80\n"

        // Save SAR
        "l32i a4, a3, 8\n" "s32i a4, a2, 76\n"

        // Not using  : vpri
        // "l32i a4, a3, 12\n"

        // Save GPRs
        "l32i a4, a3, 16\n" "s32i a4, a2, 0\n"
        "l32i a4, a3, 20\n" "s32i a4, a2, 8\n"
        "l32i a4, a3, 24\n" "s32i a4, a2, 12\n"
        "l32i a4, a3, 28\n" "s32i a4, a2, 16\n"
        "l32i a4, a3, 32\n" "s32i a4, a2, 20\n"
        "l32i a4, a3, 36\n" "s32i a4, a2, 24\n"
        "l32i a4, a3, 40\n" "s32i a4, a2, 28\n"
        "l32i a4, a3, 44\n" "s32i a4, a2, 32\n"
        "l32i a4, a3, 48\n" "s32i a4, a2, 36\n"
        "l32i a4, a3, 52\n" "s32i a4, a2, 40\n"
        "l32i a4, a3, 56\n" "s32i a4, a2, 44\n"
        "l32i a4, a3, 60\n" "s32i a4, a2, 48\n"
        "l32i a4, a3, 64\n" "s32i a4, a2, 52\n"
        "l32i a4, a3, 68\n" "s32i a4, a2, 56\n"
        "l32i a4, a3, 72\n" "s32i a4, a2, 60\n"

        // Save SP (true interrupted SP is live a1 here)
        "addi a4, a3, 256\n" 
        "s32i a4, a2, 72\n"
        "s32i a4, a2, 4\n"

        :
        : "r"(&__isr_entry_ctx), "r"(frame)
    );

    timer1_isr_coroutine(&__isr_entry_ctx);
    // asm volatile ("rfi 1\n");
}

/**
 * Timer1 start api with period as needed
 */
void IRAM_ATTR timer1_start_us(uint32_t period_us) {

    // Configure Timer1: enable, auto‑reload, prescale ÷16, level interrupt
    TIMER_REG_WRITE(FRC1_CTRL_ADDRESS,
                FRC1_ENABLE |
                FRC1_AUTO_LOAD |
                FRC1_PRESCALE_DIV16 |
                FRC1_INT_TYPE_EDGE);

    // Attach ISR
    ETS_FRC_TIMER1_INTR_ATTACH(timer1_isr, NULL);
    TM1_EDGE_INT_ENABLE();
    ETS_FRC1_INTR_ENABLE();

    // Load period in ticks. //maximum ticks 8388607
    TIMER_REG_WRITE(FRC1_LOAD_ADDRESS, US_TO_RTC_TIMER_TICKS(period_us));

    // Clear interrupt
    // TIMER_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
}

/**
 * Timer1 update api with period as needed
 */
void IRAM_ATTR timer1_update_us(uint32_t period_us) {

    // Load period in ticks. //maximum ticks 8388607
    TIMER_REG_WRITE(FRC1_LOAD_ADDRESS, US_TO_RTC_TIMER_TICKS(period_us));
    TM1_EDGE_INT_ENABLE();
    // ETS_FRC1_INTR_ENABLE();
}

/**
 * Timer1 clear
 */
void IRAM_ATTR timer1_clear() {

    TIMER_REG_WRITE(FRC1_CTRL_ADDRESS, 0);
    // Dettach ISR
    TM1_EDGE_INT_DISABLE();
    ETS_FRC1_INTR_DISABLE();

    // TIMER_REG_WRITE(FRC1_INT_ADDRESS, 0);
}
