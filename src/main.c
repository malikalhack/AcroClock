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
#include "acrosched_ipc.h"
#include "debug_log.h"
#include "rtc_ds3231.h"

/********************************* Definitions ********************************/

/**
 * @def RTC_SECOND_EVENT
 * @brief AcroSched event flag set by each DS3231 falling edge.
 */
#define RTC_SECOND_EVENT                0x01UL

/**
 * @def RTC_EVENT_POLL_INTERVAL_TICKS
 * @brief Maximum scheduler polling interval for the RTC event flag.
 */
#define RTC_EVENT_POLL_INTERVAL_TICKS   10U

/**
 * @def RTC_TEMPERATURE_INTERVAL_TICKS
 * @brief Temperature logging period in scheduler ticks (one minute).
 */
#define RTC_TEMPERATURE_INTERVAL_TICKS  (60U * BSP_TICKS_PER_SEC)

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
    eLedBlinkTask = 1,
    eRtcTask,
    eRtcTempTask,
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
static SAcroEventGroup_t rtc_events;

/***************************** Private prototypes *****************************/

/**
 * @brief Logs a fatal error and halts execution.
 * @param[in] message - description of the failure.
 */
_Noreturn static void error_handler(char const *message);

/**
 * @brief Toggles the on-board LED from the scheduled blink task.
 * @param[in] params - unused scheduler task parameter.
 */
static void led_blink_task(AcroParam_t params);

/**
 * @brief Reads the RTC and logs the current time or a read error.
 * @param[in] params - unused scheduler task parameter.
 */
static void rtc_log_task(AcroParam_t params);

/**
 * @brief Reads and logs DS3231 temperature once per minute.
 * @param[in] params - unused scheduler task parameter.
 */
static void rtc_temperature_task(AcroParam_t params);

/**
 * @brief Signals the RTC event group from the EXTI0 interrupt context.
 */
static void rtc_sqw_event_callback(void);

/**
 * @brief Atomically tests and consumes the pending RTC second event.
 * @returns 1 when an event was consumed, otherwise 0.
 */
static uint8_t rtc_take_second_event(void);

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

/**
 * @brief Logs the DS3231 temperature to the nearest quarter degree Celsius.
 * @param[in] temperature_quarters_c - temperature multiplied by four.
 */
static void log_rtc_temperature(int16_t temperature_quarters_c);

/****************************** Private functions *****************************/

static void led_blink_task(AcroParam_t params) {
    (void)params;
    bspLedToggle();
}
/*----------------------------------------------------------------------------*/

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

static void log_rtc_temperature(int16_t temperature_quarters_c) {
    char message[] = "RTC temp: +00.00 C";
    uint16_t magnitude_quarters;
    uint8_t degrees;
    uint8_t fraction;

    if (temperature_quarters_c < 0) {
        message[10] = '-';
        magnitude_quarters = (uint16_t)(-temperature_quarters_c);
    }
    else {
        message[10] = '+';
        magnitude_quarters = (uint16_t)temperature_quarters_c;
    }
    degrees = (uint8_t)(magnitude_quarters / 4U);
    fraction = (uint8_t)((magnitude_quarters % 4U) * 25U);

    message[11] = (char)('0' + (degrees / 10U));
    message[12] = (char)('0' + (degrees % 10U));
    message[14] = (char)('0' + (fraction / 10U));
    message[15] = (char)('0' + (fraction % 10U));
    debug_log_info(message);
}
/*----------------------------------------------------------------------------*/

static void rtc_log_task(AcroParam_t params) {
    SRtcDateTime_t date_time;
    ERtcStatus_t status;

    (void)params;
    if (rtc_take_second_event() == 0U) {
        return;
    }

    status = rtc_ds3231_read_time(&date_time);
    if (status != eRtcOk) {
        log_rtc_error(status);
    }
    else {
        log_rtc_time(&date_time);
    }
}
/*----------------------------------------------------------------------------*/

static void rtc_temperature_task(AcroParam_t params) {
    int16_t temperature_quarters_c;
    ERtcStatus_t status;

    (void)params;
    status = rtc_ds3231_read_temperature(&temperature_quarters_c);
    if (status != eRtcOk) {
        log_rtc_error(status);
    }
    else {
        log_rtc_temperature(temperature_quarters_c);
    }
}
/*----------------------------------------------------------------------------*/

static void rtc_sqw_event_callback(void) {
    acroEventSet(&rtc_events, RTC_SECOND_EVENT);
}
/*----------------------------------------------------------------------------*/

static uint8_t rtc_take_second_event(void) {
    uint8_t event_pending;

    event_pending = 0U;
    ACROSCHED_CRITICAL(
        event_pending = (uint8_t)(
            (acroEventGet(&rtc_events) & RTC_SECOND_EVENT) != 0U
        );
        if (event_pending != 0U) {
            acroEventClear(&rtc_events, RTC_SECOND_EVENT);
        }
    );

    return event_pending;
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
            debug_log_error("DS3231 returned invalid data");
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
    ERtcStatus_t rtc_status;

    bsp_status = bspInit(&system_tick);
    if (bsp_status != eBspOk) {
        error_handler("BSP initialization failed");
    }
    debug_log_info("AcroClock starting");

    CHECK_STATUS(acroInit(&system_tick));
    rtc_status = rtc_ds3231_configure_sqw_1hz();
    if (rtc_status != eRtcOk) {
        error_handler("DS3231 square-wave configuration failed");
    }
    bsp_status = bspRtcSqwInit(rtc_sqw_event_callback);
    if (bsp_status != eBspOk) {
        error_handler("RTC square-wave interrupt initialization failed");
    }

    scheduler_status = acroAddTask(
        eLedBlinkTask,
        led_blink_task,
        NULL,
        ePeriodic,
        BSP_TICKS_PER_SEC / 2U,
        ePrioNormal
    );
    if (scheduler_status != eAcroOk) {
        error_handler("LED blink task registration failed");
    }

    scheduler_status = acroAddTask(
        eRtcTask,
        rtc_log_task,
        NULL,
        ePeriodic,
        RTC_EVENT_POLL_INTERVAL_TICKS,
        ePrioNormal
    );
    if (scheduler_status != eAcroOk) {
        error_handler("RTC task registration failed");
    }

    scheduler_status = acroAddTask(
        eRtcTempTask,
        rtc_temperature_task,
        NULL,
        ePeriodic,
        RTC_TEMPERATURE_INTERVAL_TICKS,
        ePrioNormal
    );
    if (scheduler_status != eAcroOk) {
        error_handler("RTC temperature task registration failed");
    }

    bspWatchdogStart();
    IGNORE_RETURN(acroRun()); /* Should not return after successful init. */
    error_handler("Scheduler stopped unexpectedly");
    return 1;
}
/******************************************************************************/
