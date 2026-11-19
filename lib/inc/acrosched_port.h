/**
 * @copyright 2021 Anthony Chernov
 * Licensed under the MIT License; see LICENSE in the project root.
 * @file    acrosched_port.h
 * @version 2.7.1
 * @authors Anton Chernov
 * @date    2026-05-14
 * @date    @showdate "%Y-%m-%d"
 */

#ifndef ACROSCHED_PORT_H
#define ACROSCHED_PORT_H

/******************************** Included files ******************************/
#include "cmsis_compiler.h"

/********************************* Definitions ********************************/

/**
 * @def ACROSCHED_TICK_TYPE
 * @brief Override tick type to 32-bit for Cortex-M targets.
 */
#define ACROSCHED_TICK_TYPE         uint32_t

/**
 * @def ACROSCHED_STACK_WORD_TYPE
 * @brief Unsigned word type used to allocate and measure preemptive stacks.
 * @details Cortex-M stacks use 32-bit words. Stack size settings and the
 * high-water mark are expressed in units of this type.
 */
#define ACROSCHED_STACK_WORD_TYPE   uint32_t

/**
 * @def ACROSCHED_STACK_ALIGNMENT
 * @brief Required preemptive stack alignment in bytes.
 * @details This port requires 8-byte alignment at exception boundaries.
 * Stack allocations are rounded up to this alignment.
 */
#define ACROSCHED_STACK_ALIGNMENT   8U

/**
 * @def ACROSCHED_STACK_GROWS_DOWN
 * @brief Direction in which preemptive task stacks grow.
 * @details Set to 1 for Cortex-M, where stacks grow toward lower addresses;
 * 0 denotes a stack that grows toward higher addresses.
 */
#define ACROSCHED_STACK_GROWS_DOWN  1

/**
 * @def ACROSCHED_STACK_MIN_WORDS
 * @brief Minimum uniform task stack size in port-defined words.
 * @details Enforced for ACROSCHED_STACK_SIZE. This floor does not guarantee
 * that a stack is large enough for a task's context and worst-case usage.
 */
#define ACROSCHED_STACK_MIN_WORDS   32U

/**
 * @def ACROSCHED_TICK_READ_NEEDS_CRITICAL
 * @brief A naturally aligned 32-bit tick read is atomic on Cortex-M.
 */
#define ACROSCHED_TICK_READ_NEEDS_CRITICAL  0

/**
 * @def ACROSCHED_DECLARE_CRITICAL
 * @brief Local storage for the saved interrupt-enable state (PRIMASK).
 */
#define ACROSCHED_DECLARE_CRITICAL()  uint32_t _acroPriMask

/**
 * @def ACROSCHED_ENTER_CRITICAL
 * @brief Save the current PRIMASK and disable all interrupts (CMSIS intrinsic).
 * @details Nesting-safe: ACROSCHED_EXIT_CRITICAL() restores the saved PRIMASK,
 * so a nested entry made while interrupts are already disabled does not
 * re-enable them on exit.
 */
#define ACROSCHED_ENTER_CRITICAL()      \
    do {                                \
        _acroPriMask = __get_PRIMASK(); \
        __disable_irq();                \
    } while (0)

/**
 * @def ACROSCHED_EXIT_CRITICAL
 * @brief Restore the PRIMASK saved by ACROSCHED_ENTER_CRITICAL().
 */
#define ACROSCHED_EXIT_CRITICAL()   __set_PRIMASK(_acroPriMask)

/**
 * @def ACROSCHED_WAIT_FOR_EVENT
 * @brief Enter sleep until the next interrupt (Cortex-M Wait-For-Interrupt).
 * @details Used by the optional idle-hook module (ACROSCHED_USE_IDLE_HOOK) to
 * stop the core when no task ran during a dispatcher iteration. The system
 * tick interrupt (or any other enabled interrupt) wakes the core. For deeper
 * tickless sleep, replace this with a sequence that reprograms a low-power
 * wake timer using acroTimeToNextEvent() before entering a deep-sleep mode.
 */
#define ACROSCHED_WAIT_FOR_EVENT()  __WFI()

#if defined(STM32F10X_LD) || defined(STM32F10X_LD_VL) || \
    defined(STM32F10X_MD) || defined(STM32F10X_MD_VL) || \
    defined(STM32F10X_HD) || defined(STM32F10X_HD_VL) || \
    defined(STM32F10X_XL) || defined(STM32F10X_CL)
/** @brief Selects the STM32F1 device-specific port bindings. */
#define ACROSCHED_PORT_STM32F1
#endif /* STM32F10X_* */

#if (ACROSCHED_USE_TICKLESS == 1) && \
    (defined(STM32U575xx) || defined(ACROSCHED_PORT_STM32F1) || \
     defined(STM32C071xx))
/** @brief Tickless transaction: U575 WUT/Stop 2, F1/C071 RTC alarm/Stop. */
#define ACROSCHED_TICKLESS_IDLE(budget)  acroPortTicklessIdle(budget)

/** @brief Empty: sleep accounting completes before any ISR runs. */
#define ACROSCHED_TICKLESS_WAKE()        do { } while (0)
#endif /* ACROSCHED_USE_TICKLESS / U575 / F1 / C071 */

#if (ACROSCHED_USE_TICKLESS == 1) && defined(ACROSCHED_PORT_STM32F1)
/**
 * @def ACROSCHED_STM32F1_TICKLESS_RTC_SHIFT
 * @brief RTC prescaler shift k: the adapter sets PRL = 2^k - 1, so RTC_CNT
 * counts at 32768 / 2^k Hz from LSE. Valid range 1..4; default 2 (8192 Hz).
 */
#ifndef ACROSCHED_STM32F1_TICKLESS_RTC_SHIFT
#define ACROSCHED_STM32F1_TICKLESS_RTC_SHIFT  2
#endif /* ACROSCHED_STM32F1_TICKLESS_RTC_SHIFT */

#if (ACROSCHED_STM32F1_TICKLESS_RTC_SHIFT < 1) || \
    (ACROSCHED_STM32F1_TICKLESS_RTC_SHIFT > 4)
#error "ACROSCHED_STM32F1_TICKLESS_RTC_SHIFT must be 1..4."
#endif /* ACROSCHED_STM32F1_TICKLESS_RTC_SHIFT */
#endif /* ACROSCHED_USE_TICKLESS / ACROSCHED_PORT_STM32F1 */

#if (ACROSCHED_USE_TICKLESS == 1) && defined(STM32C071xx)
/**
 * @def ACROSCHED_STM32C0_TICKLESS_RTC_SHIFT
 * @brief RTC prescaler shift k: the adapter sets PREDIV_A = 2^k - 1 and
 * PREDIV_S = 2^(15 - k) - 1, so RTC_SSR counts at 32768 / 2^k Hz from LSE
 * and the calendar keeps 1 Hz. Valid range 0..4; default 2 (8192 Hz).
 */
#ifndef ACROSCHED_STM32C0_TICKLESS_RTC_SHIFT
#define ACROSCHED_STM32C0_TICKLESS_RTC_SHIFT  2
#endif /* ACROSCHED_STM32C0_TICKLESS_RTC_SHIFT */

#if (ACROSCHED_STM32C0_TICKLESS_RTC_SHIFT < 0) || \
    (ACROSCHED_STM32C0_TICKLESS_RTC_SHIFT > 4)
#error "ACROSCHED_STM32C0_TICKLESS_RTC_SHIFT must be 0..4."
#endif /* ACROSCHED_STM32C0_TICKLESS_RTC_SHIFT */
#endif /* ACROSCHED_USE_TICKLESS / STM32C071xx */

/******************************************************************************/
#endif // !ACROSCHED_PORT_H
