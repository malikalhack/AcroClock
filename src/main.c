/**
 * @file    main.c
 * @version 0.1.0
 * @authors Anton Chernov
 * @date    2026-08-15
 * @date    @showdate "%Y-%m-%d"
 */

/******************************** Included files ******************************/
#include "bsp.h"
#include "acrosched.h"
#include "acrosched_defs.h" /* IGNORE_RETURN */
#include "debug_log.h"
#include "rtc_ds3231.h"

/********************************* Definitions ********************************/

/**
 * @def CHECK_STATUS
 * @brief Checks an AcroSched status and stops on failure.
 */
#define CHECK_STATUS(x) do { \
    if ((x) != eAcroOk) { \
        error_handler("AcroSched operation failed"); \
    } \
} while (0)

/**
 * @enum ETaskDescriptors
 * @brief Task descriptors.
 */
enum ETaskDescriptors {
    eLogTask = 1,
    eMaxDscr
};

/**
 * @enum ETaskPriority
 * @brief Dispatch priorities. Higher value dispatches earlier.
 */
enum ETaskPriority {
    ePrioNormal = 0
};

/****************************** Module variables ******************************/

static volatile AcroTick_t system_tick = 0U;

/***************************** Private prototypes *****************************/

/**
 * @brief Logs a fatal error and halts execution.
 * @param[in] message - description of the failure.
 */
_Noreturn static void error_handler(char const *message);

/**
 * @brief Reads the RTC and logs the current time or a read error.
 * @param[in] params - unused scheduler task parameter.
 */
static void rtc_log_task(AcroParam_t params);

/**
 * @brief Logs a human-readable description of an RTC error.
 * @param[in] status - status returned by the DS3231 driver.
 */
static void log_rtc_error(ERtcStatus_t status);

/**
 * @brief Logs the time fields from a decoded RTC calendar.
 * @param[in] date_time - decoded date and time.
 */
static void log_rtc_time(SRtcDateTime_t const *date_time);

/****************************** Private functions *****************************/

_Noreturn static void error_handler(char const *message) {
    debug_log_error(message);
    /* Stop here; continuing after an initialization/runtime failure is unsafe. */
    for (;;) { }
}
/*----------------------------------------------------------------------------*/

static void log_rtc_time(SRtcDateTime_t const *date_time) {
    char message[] = "RTC 00:00:00";

    message[4]  = (char)('0' + (date_time->hours / 10U));
    message[5]  = (char)('0' + (date_time->hours % 10U));
    message[7]  = (char)('0' + (date_time->minutes / 10U));
    message[8]  = (char)('0' + (date_time->minutes % 10U));
    message[10] = (char)('0' + (date_time->seconds / 10U));
    message[11] = (char)('0' + (date_time->seconds % 10U));
    debug_log_info(message);
}
/*----------------------------------------------------------------------------*/

static void rtc_log_task(AcroParam_t params) {
    SRtcDateTime_t date_time;
    ERtcStatus_t status;

    (void)params;
    status = rtc_ds3231_read_time(&date_time);
    if (status != eRtcOk) {
        log_rtc_error(status);
    }
    else {
        log_rtc_time(&date_time);
    }
}
/*----------------------------------------------------------------------------*/

static void log_rtc_error(ERtcStatus_t status) {
    switch (status) {
        case eRtcInvalidParam:
            debug_log_error("DS3231 called with invalid argument");
            break;
        case eRtcNotInitialized:
            debug_log_error("BSP I2C is not initialized");
            break;
        case eRtcBusError:
            debug_log_error("DS3231 I2C bus error or NACK");
            break;
        case eRtcTimeout:
            debug_log_error("DS3231 I2C timeout");
            break;
        case eRtcInvalidData:
            debug_log_error("DS3231 returned invalid calendar data");
            break;
        case eRtcOk:
            break;
        default:
            debug_log_error("Unknown DS3231 status");
            break;
    }
}

/********************** Application Programming Interface *********************/

void acroWatchdogRefresh(void) {
    /* The scheduler calls this hook from its dispatcher loop. */
    bspWatchdogKick();
}
/*----------------------------------------------------------------------------*/

int main(void) {
    EBspStatus_t bsp_status;
    EAcroStatus_t scheduler_status;

    bsp_status = bspInit(&system_tick);
    if (bsp_status != eBspOk) {
        error_handler("BSP initialization failed");
    }
    debug_log_info("AcroClock starting");

    CHECK_STATUS(acroInit(&system_tick));
    scheduler_status = acroAddTask(
        eLogTask,
        rtc_log_task,
        NULL,
        ePeriodic,
        BSP_TICKS_PER_SEC,
        ePrioNormal
    );
    if (scheduler_status != eAcroOk) {
        error_handler("RTC task registration failed");
    }

    bspWatchdogStart();
    IGNORE_RETURN(acroRun()); /* Should not return after successful init. */
    error_handler("Scheduler stopped unexpectedly");
    return 1;
}
/******************************************************************************/
