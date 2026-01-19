/********************************* XtensaTimer *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2025
******************************************************************************/

#ifndef __ESP866_XTENSA_TIMER_H__
#define __ESP866_XTENSA_TIMER_H__

#include <stdint.h>
#include "XtensaContext.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global context variable to capture at entry point  in ISR 
//load initial_value to timer1
// #define FRC1_LOAD_ADDRESS                  0x00
// //timer1's counter value(count from initial_value to 0)
// #define FRC1_COUNT_ADDRESS                 0x04
// #define FRC1_CTRL_ADDRESS                  0x08
// //clear timer1's interrupt when write this address
// #define FRC1_INT_ADDRESS                   0x0c
// #define FRC1_INT_CLR_MASK                  0x00000001

// FRC1 Control Register (T1C) bit fields
#define FRC1_INT_TYPE_EDGE   (0 << 0)   // Interrupt type: edge
#define FRC1_INT_TYPE_LEVEL  (1 << 0)   // Interrupt type: level

#define FRC1_PRESCALE_DIV1   (0 << 2)   // Prescale ÷1   (12.5 ns/tick)
#define FRC1_PRESCALE_DIV16  (1 << 2)   // Prescale ÷16  (0.2 µs/tick)
#define FRC1_PRESCALE_DIV256 (2 << 2)   // Prescale ÷256 (3.2 µs/tick)

#define FRC1_AUTO_LOAD       (1 << 6)   // Auto reload
#define FRC1_ENABLE          (1 << 7)   // Timer enable
#define FRC1_INT_STATUS      (1 << 8)   // Interrupt status (read‑only)

// referred from https://github.com/espressif/ESP8266_NONOS_SDK/blob/master/driver_lib/driver/hw_timer.c
#define US_TO_RTC_TIMER_TICKS(t)          \
    ((t) ?                                   \
     (((t) > 0x35A) ?                   \
      (((t)>>2) * ((APB_CLK_FREQ>>4)/250000) + ((t)&0x3) * ((APB_CLK_FREQ>>4)/1000000))  :    \
      (((t) *(APB_CLK_FREQ>>4)) / 1000000)) :    \
     0)

// Naked ISR: accepts exc_frame pointer 
void IRAM_ATTR __attribute__((naked)) timer1_isr(void* arg);
void IRAM_ATTR timer1_start_us(uint32_t period_us);
void IRAM_ATTR timer1_update_us(uint32_t period_us);
void IRAM_ATTR timer1_clear();
extern void IRAM_ATTR __attribute__((naked)) timer1_isr_coroutine(XtensaContext* ctx) __attribute__((weak));


#ifdef __cplusplus
}
#endif

#endif
