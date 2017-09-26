/* *****************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * ****************************************************************************/
# modified by NG to give some useful info to __error handler

# On an exception, push a fake stack thread mode stack frame and redirect
# thread execution to a thread mode error handler

# From RM008:
# The SP is decremented by eight words by the completion of the stack push.
# Figure 5-1 shows the contents of the stack after an exception pre-empts the
# current program flow.
#
# Old SP--> <previous>
#           xPSR        28
#           PC          24
#           LR          20
#           r12         16
#           r3          12
#           r2          8
#           r1          4
#    SP-->  r0          0

.text
.globl HardFault_Handler
.globl NMI_Handler
.globl MemManage_Handler
.globl BusFault_Handler
.globl UsageFault_Handler
.globl __default_exc

.code 16
.thumb_func
HardFault_Handler:
    mov r0, #2
    b __default_exc

.thumb_func
MemManage_Handler:
    mov r0, #3
    b __default_exc

.thumb_func
BusFault_Handler:
    mov r0, #4
    b __default_exc

.thumb_func
UsageFault_Handler:
    mov r0, #5
    b __default_exc

.thumb_func
FLASH_IRQHandler:
    mov r0, #6
    b __default_exc


.thumb_func
__default_exc:
    ldr r3, [sp, #24]           @ PC of exception - if access to wrong address
    ldr r1, [sp, #20]           @ LR of exception - if call to wrong address

    ldr r2, BFAR                @ for debug
    ldr r2, CFSR
    ldr r2, HFSR
    ldr r2, DFSR
    ldr r2, AFSR
    ldr r2, SHCSR

    mov r12, r1
    ldr r2, NVIC_CCR            @ Enable returning to thread mode even if there are
    mov r1 ,#1                  @ pending exceptions. See flag NONEBASETHRDENA - http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0337e/Cihcbadd.html
    str r1, [r2]
    cpsid i                     @ Disable global interrupts
    ldr r2, SYSTICK_CSR         @ Disable systick handler
    mov r1, #0
    str r1, [r2]
    ldr r1, CPSR_MASK           @ Set default CPSR
    mov r2, r12
    push {r1}                   @                        SP+4
    ldr r1, TARGET_PC           @ Set target pc
    push {r1}                   @                        SP+8
    push {r1}                   @ target LR - the same   SP+12
    ldr r1, [sp, #12]           @ R0 of interrupted program 
    push {r1}                   @  will be in R12        SP+16
    ldr r1, [sp, #28]           @ R3 of interrupted program (12+16)
    push {r1}                   @  will be in R3
    push {r2}                   @ R2 - LR
    push {r3}                   @ R1 - PC of exception
    push {r0}                   @ R0 - exception code

    ldr r1, EXC_RETURN          @ Return to thread mode
    mov lr, r1
    bx lr                       @ Exception exit

.align 4
CPSR_MASK:     .word 0x61000000
EXC_RETURN:    .word 0xFFFFFFF9
TARGET_PC:     .word __error
NVIC_CCR:      .word 0xE000ED14    @ NVIC configuration control register
SYSTICK_CSR:   .word 0xE000E010    @ Systick control register

BFAR:          .word 0xE000ED38
CFSR:          .word 0xE000ED28
HFSR:          .word 0xE000ED2C
DFSR:          .word 0xE000ED30
AFSR:          .word 0xE000ED3C
SHCSR:         .word 0xE000ED24
