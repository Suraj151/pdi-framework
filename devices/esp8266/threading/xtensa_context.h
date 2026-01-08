/*******************************************************************************
Copyright (c) 2006-2008 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

        XTENSA CONTEXT FRAMES AND MACROS FOR RTOS ASSEMBLER SOURCES

This header contains definitions and macros for use primarily by Xtensa
RTOS assembly coded source files. It includes and uses the Xtensa hardware
abstraction layer (HAL) to deal with config specifics. It may also be
included in C source files.

!! Supports only Xtensa Exception Architecture 2 (XEA2). XEA1 not supported. !!

NOTE: The Xtensa architecture requires stack pointer alignment to 16 bytes.

*******************************************************************************/

#ifndef XTENSA_CONTEXT_H
#define XTENSA_CONTEXT_H

/*
Align a value up to nearest n-byte boundary, where n is a power of 2.
*/
#define ALIGNUP(n, val) (((val) + (n)-1) & -(n))


/*******************************************************************************

INTERRUPT STACK FRAME FOR A THREAD OR NESTED INTERRUPT

A stack frame of this structure is allocated for any interrupt or exception.
It goes on the current stack. If the RTOS has a system stack for handling 
interrupts, every thread stack must allow space for just one interrupt stack 
frame, then nested interrupt stack frames go on the system stack.

The frame includes basic registers (explicit) and "extra" registers introduced 
by user TIE or the use of the MAC16 option in the user's Xtensa config.
The frame size is minimized by omitting regs not applicable to user's config.

For Windowed ABI, this stack frame includes the interruptee's base save area,
another base save area to manage gcc nested functions, and a little temporary 
space to help manage the spilling of the register windows.

*******************************************************************************/

#define XT_STK_EXIT             0x00    /* (offset 0) exit point for dispatch */
#define XT_STK_PC               0x04    /* return address */
#define XT_STK_PS               0x08    /* at level 1 PS.EXCM is set here */
#define XT_STK_A0               0x0C
#define XT_STK_A1               0x10    /* stack ptr before interrupt */
#define XT_STK_A2               0x14
#define XT_STK_A3               0x18
#define XT_STK_A4               0x1C
#define XT_STK_A5               0x20
#define XT_STK_A6               0x24
#define XT_STK_A7               0x28
#define XT_STK_A8               0x2C
#define XT_STK_A9               0x30
#define XT_STK_A10              0x34
#define XT_STK_A11              0x38
#define XT_STK_A12              0x3C    /* Call0 callee-save */
#define XT_STK_A13              0x40    /* Call0 callee-save */
#define XT_STK_A14              0x44    /* Call0 callee-save */
#define XT_STK_A15              0x48    /* Call0 callee-save */
#define XT_STK_SAR              0x4C
#define XT_STK_EXCCAUSE         0x50
#define XT_STK_EXCVADDR         0x54
#define XT_STK_NEXT1            0x50    /* next unused offset */
#define XT_STK_NEXT2            XT_STK_NEXT1   
/*
Windowed - 
    Need some temp space for saving stuff during window spill.
    Also add 16 bytes to skip over interruptee's base save area 
    and another 16 bytes in case of gcc nested functions: these 
    must be at physical top (logical base) of frame.
*/
#define XT_STK_N_TMP            3       /* # of 4-byte temp. slots */
#define XT_STK_TMP              XT_STK_NEXT2   
#define XT_STK_NEXT3            XT_STK_TMP    + (4 * XT_STK_N_TMP)
#define XT_STK_FRMSZ            (ALIGNUP(0x10, XT_STK_NEXT3) + 0x20)

#define PS_UM   0x00000020
#define PS_EXCM 0x00000010

// Offsets into the Xtensa exception frame (ESP8266 LX106)
#define EXC_PS_OFFSET   0   // saved processor status
#define EXC_PC_OFFSET   4   // saved program counter
#define EXC_A0_OFFSET   8   // return address register
#define EXC_SP_OFFSET   12  // saved stack pointer (A1)

#define XSTR(x) STR(x)
#define STR(x) #x

// You can extend this with other registers if needed

#endif /* XTENSA_CONTEXT_H */
