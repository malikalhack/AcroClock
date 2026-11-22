/**
 * @copyright 2021 Anthony Chernov
 * Licensed under the MIT License; see LICENSE in the project root.
 * @file    acrosched_ipc.h
 * @version 2.7.1
 * @authors Anton Chernov
 * @date    2026-05-14
 * @date    @showdate "%Y-%m-%d"
 */

#ifndef ACROSCHED_IPC_H
#define ACROSCHED_IPC_H

/******************************** Included files ******************************/
#include <stdint.h>
#include "acrosched_config.h"
#include "acrosched.h"
#if (ACROSCHED_USE_PREEMPT_EVENTS == 1)
#include "acrosched_preempt.h"
#endif /* ACROSCHED_USE_PREEMPT_EVENTS */

/********************************* Definitions ********************************/

#if (ACROSCHED_USE_IPC == 1)

/**
 * @def ACRO_EVENT_ALL_BITS
 * @brief Mask covering all event bits in an AcroEventGroup_t.
 */
#define ACRO_EVENT_ALL_BITS     (0xFFFFFFFFUL)

/*----------------------------------------------------------------------------*/

/**
 * @brief Bit mask type for event group operations.
 * @details Each bit represents one independent event signal.
 * Up to 32 distinct events per group.
 */
typedef uint32_t AcroEventBits_t;

/*----------------------------------------------------------------------------*/

/**
 * @brief Event group object for ISR-to-task signalling.
 * @details Declare statically in application code:
 * @code
 * static SAcroEventGroup_t xUartEvents;
 * @endcode
 * Initialise to zero before use (C static storage duration guarantees this
 * for file-scope objects).
 */
typedef struct SAcroEventGroup {
    volatile AcroEventBits_t bits; /* Current event bits. */
} SAcroEventGroup_t;

/********************* Application Programming Interface *********************/

/**
 * @brief Sets one or more event bits in an event group.
 * @details Safe to call from an ISR. The operation is protected by
 * ACROSCHED_ENTER_CRITICAL() / ACROSCHED_EXIT_CRITICAL().
 * @param[in] pGroup    - pointer to the event group (must not be NULL).
 * @param[in] bitsToSet - bit mask of the events to signal; already-set
 * bits are unaffected.
 */
void acroEventSet(SAcroEventGroup_t *pGroup, AcroEventBits_t bitsToSet);

/**
 * @brief Returns the current event bits of an event group.
 * @details The read is protected by a critical section to ensure atomicity
 * on 8-bit platforms where a 32-bit read is not inherently atomic.
 * @param[in] pGroup - pointer to the event group (must not be NULL).
 * @returns Snapshot of the current event bits at the time of the call.
 */
AcroEventBits_t acroEventGet(SAcroEventGroup_t const *pGroup);

/**
 * @brief Clears one or more event bits in an event group.
 * @details Protected by ACROSCHED_ENTER_CRITICAL() / ACROSCHED_EXIT_CRITICAL().
 * Typically called by the task after it has processed the signalled event.
 * @param[in] pGroup      - pointer to the event group (must not be NULL).
 * @param[in] bitsToClear - bit mask of the events to clear; bits not
 * included in the mask are unaffected.
 */
void acroEventClear(SAcroEventGroup_t *pGroup, AcroEventBits_t bitsToClear);

/*----------------------------------------------------------------------------*/

#if (ACROSCHED_USE_PREEMPT_EVENTS == 1)

/**
 * @brief Waits for event bits, blocking the calling task until signalled.
 * @details Task context only; available with the preemptive kernel
 * (ACROSCHED_USE_PREEMPT_EVENTS). Suspends the caller until the requested
 * bits are set by acroEventSet() (which may run in an ISR) or the timeout
 * expires. Under wait-all semantics (waitAll non-zero) every requested bit
 * must be set; under wait-any (waitAll zero) any single requested bit is
 * enough. On success the bits that satisfied the wait are optionally reported
 * through pBitsOut and, when clearOnExit is non-zero, cleared atomically in
 * the same critical section; clearOnExit is ignored on failure. A zero
 * timeout polls without blocking. A wake-up is only a hint: the condition is
 * re-checked on resumption, so a concurrent consumer may clear the bits
 * first; the original deadline covers every retry.
 * @param[in]  pGroup      - pointer to the event group (must not be NULL).
 * @param[in]  bitsToWait  - bit mask to wait for (must not be zero).
 * @param[in]  waitAll     - non-zero: wait for all bits; zero: wait for any.
 * @param[in]  clearOnExit - non-zero: clear the satisfying bits on success.
 * @param[in]  timeout     - 0, up to ACROSCHED_MAX_BLOCK_TIMEOUT ticks, or
 * ACROSCHED_WAIT_FOREVER.
 * @param[out] pBitsOut    - receives the bits that satisfied the wait, or
 * NULL when the caller is not interested. Unchanged on failure.
 * @returns Operation status.
 * @retval eAcroOk           The requested bits were signalled.
 * @retval eAcroTimeout      The timeout expired (or condition false at
 * timeout 0).
 * @retval eAcroInvalidParam pGroup is NULL, bitsToWait is zero, or the finite
 * timeout is out of range.
 * @retval eAcroError        Blocking call outside a running application task.
 */
EAcroStatus_t acroEventWaitBlocking (
    SAcroEventGroup_t *pGroup,
    AcroEventBits_t bitsToWait,
    uint8_t waitAll,
    uint8_t clearOnExit,
    AcroTick_t timeout,
    AcroEventBits_t *pBitsOut
);

#endif /* ACROSCHED_USE_PREEMPT_EVENTS */

/*----------------------------------------------------------------------------*/

/**
 * @brief Message data type for mailbox operations.
 * @details 32-bit unsigned integer. Cast to/from the required type at
 * the call site. The width is sufficient for a pointer on all supported
 * 8/16/32-bit platforms.
 */
typedef uint32_t AcroMailData_t;

/*----------------------------------------------------------------------------*/

/**
 * @brief Single-slot mailbox object for ISR-to-task message passing.
 * @details Holds at most one unread message at a time. Declare statically:
 * @code
 * static SAcroMailbox_t xAdcMailbox;
 * @endcode
 * Initialise to zero before use (C static storage duration guarantees this
 * for file-scope objects).
 */
typedef struct SAcroMailbox {
    volatile AcroMailData_t data; /* Message payload.              */
    volatile uint8_t        full; /* Non-zero when a message waits */
} SAcroMailbox_t;

/**
 * @brief Posts a message to a mailbox.
 * @details Safe to call from an ISR. The operation is protected by
 * ACROSCHED_ENTER_CRITICAL() / ACROSCHED_EXIT_CRITICAL(). If the mailbox
 * already holds an unread message the new value is discarded and
 * eAcroFull is returned; the existing message is preserved.
 * @param[in] pMbox - pointer to the mailbox (must not be NULL).
 * @param[in] data  - value to post.
 * @returns Operation status.
 * @retval eAcroOk   Message posted successfully.
 * @retval eAcroFull Previous message has not been fetched yet.
 */
EAcroStatus_t acroMailPost(SAcroMailbox_t *pMbox, AcroMailData_t data);

/**
 * @brief Fetches a message from a mailbox.
 * @details Protected by ACROSCHED_ENTER_CRITICAL() / ACROSCHED_EXIT_CRITICAL().
 * On success the mailbox slot is marked empty so the ISR may post again.
 * @param[in]  pMbox  - pointer to the mailbox (must not be NULL).
 * @param[out] pData  - pointer to the variable that receives the message
 * (must not be NULL).
 * @returns Operation status.
 * @retval eAcroOk           Message fetched; *pData is valid.
 * @retval eAcroError        No message is pending; *pData is unchanged.
 * @retval eAcroInvalidParam pMbox or pData is NULL.
 */
EAcroStatus_t acroMailFetch(SAcroMailbox_t *pMbox, AcroMailData_t *pData);

#endif // ACROSCHED_USE_IPC

/******************************************************************************/
#endif // !ACROSCHED_IPC_H
