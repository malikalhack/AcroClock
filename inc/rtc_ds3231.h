/**
 * @file    rtc_ds3231.h
 * @version 0.1.0
 * @authors Anton Chernov
 * @date    2026-11-21
 * @date    @showdate "%Y-%m-%d"
 */

#ifndef RTC_DS3231_H_
#define RTC_DS3231_H_

/******************************** Included files ******************************/
#include <stdint.h>

/********************************* Definitions ********************************/

/**
 * @struct SRtcDateTime_t
 * @brief Calendar fields read from the DS3231.
 * @details Year is the two-digit value 00..99 and weekday is 1..7.
 */
typedef struct SRtcDateTime {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekday;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} SRtcDateTime_t;

/**
 * @enum ERtcStatus
 * @brief Result of a DS3231 operation.
 */
typedef enum ERtcStatus {
    eRtcOk           = 0, /**< The operation completed successfully. */
    eRtcInvalidParam = 1, /**< An argument is invalid. */
    eRtcNotInitialized = 2, /**< The BSP I2C peripheral is not initialized. */
    eRtcBusError     = 3, /**< The I2C bus reported an error or NACK. */
    eRtcTimeout      = 4, /**< The I2C transaction timed out. */
    eRtcInvalidData  = 5  /**< The calendar registers contain invalid BCD. */
} ERtcStatus_t;

/********************* Application Programming Interface *********************/

/**
 * @brief Reads and validates the DS3231 calendar registers.
 * @param[out] date_time - destination for the decoded calendar fields.
 * @returns Operation status.
 * @retval eRtcOk           The calendar was read and validated.
 * @retval eRtcInvalidParam date_time is NULL.
 * @retval eRtcNotInitialized The BSP I2C peripheral is not initialized.
 * @retval eRtcBusError     The I2C bus reported an error or NACK.
 * @retval eRtcTimeout      The I2C transfer timed out.
 * @retval eRtcInvalidData  A register value is invalid.
 */
ERtcStatus_t rtc_ds3231_read_time(SRtcDateTime_t *date_time);

/**
 * @brief Reads the DS3231 temperature in quarter-degrees Celsius.
 * @param[out] temperature_quarters_c - temperature multiplied by four.
 * @returns Operation status.
 * @retval eRtcOk The temperature was read successfully.
 * @retval eRtcInvalidParam temperature_quarters_c is NULL.
 * @retval eRtcNotInitialized The BSP I2C peripheral is not initialized.
 * @retval eRtcBusError The I2C bus reported an error or NACK.
 * @retval eRtcTimeout The I2C transfer timed out.
 */
ERtcStatus_t rtc_ds3231_read_temperature(int16_t *temperature_quarters_c);

/**
 * @brief Configures the DS3231 square-wave output for 1 Hz.
 * @details Selects square-wave mode, disables alarm interrupts and disables
 * square-wave output on battery backup while preserving unrelated control
 * register bits.
 * @returns Operation status.
 * @retval eRtcOk The output was configured successfully.
 * @retval eRtcNotInitialized The BSP I2C peripheral is not initialized.
 * @retval eRtcBusError The I2C bus reported an error or NACK.
 * @retval eRtcTimeout The I2C transfer timed out.
 */
ERtcStatus_t rtc_ds3231_configure_sqw_1hz(void);

/******************************************************************************/
#endif // !RTC_DS3231_H_
