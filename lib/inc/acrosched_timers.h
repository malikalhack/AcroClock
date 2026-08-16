/**
 * @file    acrosched_timers.h
 * @version 2.2.0
 * @authors Anton Chernov
 * @date    2026-06-24
 * @date    @showdate "%Y-%m-%d"
 */

#ifndef ACROSCHED_TIMERS_H_
#define ACROSCHED_TIMERS_H_

/******************************** Included files ******************************/
#include <stdint.h>
#include "acrosched_config.h"
#include "acrosched.h"

/********************************* Definitions ********************************/

#if (ACROSCHED_USE_TIMERS == 1)

/**
 * @brief Software-timer handle.
 * @details Zero-based index into the static timer pool, returned by
 * acroTimerCreate() and passed to acroTimerStart() / acroTimerStop(). Valid
 * values are 0 .. (ACROSCHED_MAX_TIMERS - 1).
 */
typedef uint8_t AcroTimerId_t;

/*----------------------------------------------------------------------------*/

/**
 * @enum EAcroTimerMode
 * @brief Expiry behaviour of a software timer.
 */
typedef enum EAcroTimerMode {
    eAcroTimerOneShot  = 0, /**< Fires once, then stops automatically.        */
    eAcroTimerPeriodic = 1, /**< Re-arms after each expiry and fires again.   */
    eAcroTimerModeMax  = 2  /**< Sentinel - do not use as a mode value.       */
} EAcroTimerMode_t;

/*----------------------------------------------------------------------------*/

/**
 * @brief Software-timer callback signature.
 * @details Invoked from the cooperative dispatcher loop (never from an ISR)
 * when the timer expires. The callback receives the firing timer's handle and
 * the user context supplied at creation, so a single callback may serve
 * several timers and may re-arm or stop itself by handle.
 * @param[in] id    - handle of the timer that fired.
 * @param[in] param - user context passed to acroTimerCreate().
 */
typedef void AcroTimerCallback_t(AcroTimerId_t id, AcroParam_t param);

/********************* Application Programming Interface *********************/

/**
 * @brief Initialises the software-timer module.
 * @details Stores the system tick source and clears the timer pool. Call once
 * after acroInit() and before creating any timer. The tick source is the same
 * volatile counter passed to acroInit().
 * @param[in] pTimeSource - pointer to the volatile system tick counter
 *                          (must not be NULL).
 * @returns Operation status.
 * @retval eAcroOk           Module initialised successfully.
 * @retval eAcroInvalidParam pTimeSource is NULL.
 */
EAcroStatus_t acroTimersInit(volatile AcroTick_t const *pTimeSource);

/**
 * @brief Allocates a software timer from the static pool.
 * @details The timer is created in the stopped state; call acroTimerStart() to
 * arm it. The module must have been initialised with acroTimersInit() first.
 * @param[in]  pCallback - callback to invoke on expiry (must not be NULL).
 * @param[in]  mode      - eAcroTimerOneShot or eAcroTimerPeriodic.
 * @param[in]  param     - user context handed to the callback (may be NULL).
 * @param[out] pId       - receives the handle of the new timer (must not be
 *                         NULL).
 * @returns Operation status.
 * @retval eAcroOk           Timer created; *pId holds the handle.
 * @retval eAcroInvalidParam A pointer is NULL, mode is out of range, or the
 *                           module is not initialised.
 * @retval eAcroFull         The timer pool is full; no free slot available.
 */
EAcroStatus_t acroTimerCreate(AcroTimerCallback_t *pCallback,
                              EAcroTimerMode_t      mode,
                              AcroParam_t           param,
                              AcroTimerId_t        *pId);

/**
 * @brief Starts (arms) a software timer.
 * @details Records the current tick as the reference and sets the expiry
 * interval. Safe to call from an ISR; the update is protected by
 * ACROSCHED_ENTER_CRITICAL() / ACROSCHED_EXIT_CRITICAL(). Calling it on an
 * already-running timer restarts it with the new period.
 * @param[in] id     - handle returned by acroTimerCreate().
 * @param[in] period - expiry interval in ticks (must be greater than 0).
 * @returns Operation status.
 * @retval eAcroOk           Timer armed.
 * @retval eAcroInvalidParam id is out of range, the slot is not allocated, or
 *                           period is 0.
 */
EAcroStatus_t acroTimerStart(AcroTimerId_t id, AcroTick_t period);

/**
 * @brief Stops (disarms) a software timer.
 * @details The timer keeps its allocation and may be re-armed later with
 * acroTimerStart(). Safe to call from an ISR; the update is protected by a
 * critical section.
 * @param[in] id - handle returned by acroTimerCreate().
 * @returns Operation status.
 * @retval eAcroOk           Timer stopped.
 * @retval eAcroInvalidParam id is out of range or the slot is not allocated.
 */
EAcroStatus_t acroTimerStop(AcroTimerId_t id);

/**
 * @brief Services all software timers and dispatches expired callbacks.
 * @details Called once per iteration of the cooperative dispatcher loop. Each
 * running timer whose interval has elapsed has its callback invoked from the
 * loop context (outside any critical section); periodic timers re-arm, one-shot
 * timers stop. Never call this from an ISR.
 */
void acroTimersService(void);

/**
 * @brief Returns the number of ticks until the soonest software-timer expiry.
 * @details Read-only helper for tickless / deep-sleep ports. It scans the
 * timer pool and reports how many ticks remain until the earliest running
 * timer fires; a timer already due reports 0. When no timer is running the
 * function returns the maximum AcroTick_t value, meaning the timer module
 * imposes no wake-up deadline. The scan runs inside a critical section so the
 * snapshot is consistent. Combine with acroTimeToNextEvent() (idle-hook
 * module) to obtain an overall sleep budget.
 * @returns Ticks until the next timer expiry; 0 if one is already due, or the
 * maximum AcroTick_t value if no timer is running.
 */
AcroTick_t acroTimersTimeToNext(void);

#endif /* ACROSCHED_USE_TIMERS */

/******************************************************************************/

#endif //! ACROSCHED_TIMERS_H_
