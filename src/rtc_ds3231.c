/**
 * @file    rtc_ds3231.c
 * @version 0.1.0
 * @authors Anton Chernov
 * @date    2026-11-21
 * @date    @showdate "%Y-%m-%d"
 */

/******************************** Included files ******************************/
#include <stddef.h>
#include "bsp.h"
#include "rtc_ds3231.h"

/********************************* Definitions ********************************/

/**
 * @def DS3231_ADDRESS
 * @brief Unshifted 7-bit I2C address of the DS3231.
 */
#define DS3231_ADDRESS          0x68U

/**
 * @def DS3231_REG_SECONDS
 * @brief Address of the first calendar register.
 */
#define DS3231_REG_SECONDS      0x00U

/**
 * @def DS3231_CALENDAR_LENGTH
 * @brief Number of consecutive seconds-through-year registers.
 */
#define DS3231_CALENDAR_LENGTH  7U

/***************************** Private prototypes *****************************/

static uint8_t bcd_to_uint8(
    uint8_t bcd,
    uint8_t mask,
    uint8_t *value
);
static uint8_t decode_hours(uint8_t raw_hours, uint8_t *hours);

/****************************** Private functions *****************************/

static uint8_t bcd_to_uint8(
    uint8_t bcd,
    uint8_t mask,
    uint8_t *value
) {
    uint8_t tens;
    uint8_t units;
    uint8_t valid;

    bcd &= mask;
    tens = (uint8_t)(bcd >> 4U);
    units = (uint8_t)(bcd & 0x0FU);
    valid = 0U;
    if ((tens <= 9U) && (units <= 9U)) {
        *value = (uint8_t)((tens * 10U) + units);
        valid = 1U;
    }

    return valid;
}
/*----------------------------------------------------------------------------*/

static uint8_t decode_hours(uint8_t raw_hours, uint8_t *hours) {
    uint8_t is_pm;
    uint8_t valid;

    valid = 0U;
    if ((raw_hours & 0x40U) != 0U) {
        is_pm = (uint8_t)((raw_hours & 0x20U) != 0U);
        if (bcd_to_uint8(raw_hours, 0x1FU, hours) != 0U) {
            if ((*hours >= 1U) && (*hours <= 12U)) {
                if (*hours == 12U) {
                    *hours = 0U;
                }
                if (is_pm != 0U) {
                    *hours = (uint8_t)(*hours + 12U);
                }
                valid = 1U;
            }
        }
    }
    else if (bcd_to_uint8(raw_hours, 0x3FU, hours) != 0U) {
        valid = (uint8_t)(*hours <= 23U);
    }

    return valid;
}
/*----------------------------------------------------------------------------*/

/********************* Application Programming Interface *********************/

ERtcStatus_t rtc_ds3231_read_time(SRtcDateTime_t *date_time) {
    uint8_t register_address;
    uint8_t raw[DS3231_CALENDAR_LENGTH];
    SRtcDateTime_t decoded;
    EBspStatus_t bsp_status;
    ERtcStatus_t ret_val;

    if (date_time == NULL) {
        return eRtcInvalidParam;
    }

    register_address = DS3231_REG_SECONDS;
    bsp_status = bspI2cWriteRead(
        DS3231_ADDRESS,
        &register_address,
        1U,
        raw,
        DS3231_CALENDAR_LENGTH
    );

    ret_val = eRtcBusError;
    switch (bsp_status) {
        case eBspInvalidParam:
            ret_val = eRtcInvalidParam;
            break;
        case eBspNotInitialized:
            ret_val = eRtcNotInitialized;
            break;
        case eBspTimeout:
            ret_val = eRtcTimeout;
            break;
        case eBspNack:
        case eBspBusError:
            ret_val = eRtcBusError;
            break;
        case eBspOk:
            if (
                (bcd_to_uint8(raw[0], 0x7FU, &decoded.seconds) == 0U) ||
                (bcd_to_uint8(raw[1], 0x7FU, &decoded.minutes) == 0U) ||
                (decode_hours(raw[2], &decoded.hours) == 0U) ||
                (bcd_to_uint8(raw[3], 0x07U, &decoded.weekday) == 0U) ||
                (bcd_to_uint8(raw[4], 0x3FU, &decoded.date) == 0U) ||
                (bcd_to_uint8(raw[5], 0x1FU, &decoded.month) == 0U) ||
                (bcd_to_uint8(raw[6], 0xFFU, &decoded.year) == 0U) ||
                (decoded.seconds > 59U) ||
                (decoded.minutes > 59U) ||
                (decoded.weekday < 1U) || (decoded.weekday > 7U) ||
                (decoded.date < 1U) || (decoded.date > 31U) ||
                (decoded.month < 1U) || (decoded.month > 12U)
            ) {
                ret_val = eRtcInvalidData;
            }
            else {
                *date_time = decoded;
                ret_val = eRtcOk;
            }
            break;
        default:
            ret_val = eRtcBusError;
            break;
    }

    return ret_val;
}
/******************************************************************************/
