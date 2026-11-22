/**
 * @file    rtc_ds3231.c
 * @version 0.1.1
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
#define DS3231_ADDRESS              0x68U

/**
 * @def DS3231_REG_SECONDS
 * @brief Address of the first calendar register.
 */
#define DS3231_REG_SECONDS          0x00U

/**
 * @def DS3231_CALENDAR_LENGTH
 * @brief Number of consecutive seconds-through-year registers.
 */
#define DS3231_CALENDAR_LENGTH      7U

/**
 * @def DS3231_REG_CONTROL
 * @brief Address of the DS3231 control register.
 */
#define DS3231_REG_CONTROL          0x0EU

/**
 * @def DS3231_REG_TEMP_MSB
 * @brief Address of the DS3231 temperature MSB register.
 */
#define DS3231_REG_TEMP_MSB         0x11U

/**
 * @def DS3231_TEMP_REGISTER_COUNT
 * @brief Number of consecutive temperature registers.
 */
#define DS3231_TEMP_REGISTER_COUNT  2U

/**
 * @def DS3231_TEMP_SIGN_MASK
 * @brief Sign bit in the integer temperature register.
 */
#define DS3231_TEMP_SIGN_MASK       0x80U

/**
 * @def DS3231_TEMP_SIGN_OFFSET
 * @brief Two's-complement offset for the 8-bit integer temperature.
 */
#define DS3231_TEMP_SIGN_OFFSET     256

/**
 * @def DS3231_TEMP_FRACTION_MASK
 * @brief Mask for the quarter-degree fraction in the temperature LSB.
 */
#define DS3231_TEMP_FRACTION_MASK   0xC0U

/**
 * @def DS3231_TEMP_FRACTION_SHIFT
 * @brief Bit shift of the fractional temperature field.
 */
#define DS3231_TEMP_FRACTION_SHIFT  6U

/**
 * @def DS3231_TEMP_QUARTERS_PER_DEGREE
 * @brief Number of quarter-degrees in one Celsius degree.
 */
#define DS3231_TEMP_QUARTERS_PER_DEGREE  4

/**
 * @def DS3231_CTRL_SQW_CONFIG_MASK
 * @brief Control bits configured for 1 Hz square-wave operation.
 * @details Clears alarm enables, rate selection, INTCN and BBSQW.
 */
#define DS3231_CTRL_SQW_CONFIG_MASK 0x5FU

/***************************** Private prototypes *****************************/

/**
 * @brief Maps a BSP I2C status to the corresponding RTC status.
 * @param[in] status - result returned by the BSP I2C operation.
 * @returns Corresponding RTC status.
 */
static ERtcStatus_t rtc_map_bsp_status(EBspStatus_t status);

/**
 * @brief Decodes a masked packed-BCD value.
 * @param[in] bcd - packed-BCD input value.
 * @param[in] mask - bits to retain before decoding.
 * @param[out] value - decoded decimal value, unchanged if invalid.
 * @returns 1 if the value is valid BCD; otherwise 0.
 */
static uint8_t bcd_to_uint8(uint8_t bcd, uint8_t mask, uint8_t *value);

/**
 * @brief Decodes a DS3231 hours register into 24-hour format.
 * @param[in] raw_hours - raw DS3231 hours register.
 * @param[out] hours - decoded hour in the range 0..23, if valid.
 * @returns 1 if the hour is valid; otherwise 0.
 */
static uint8_t decode_hours(uint8_t raw_hours, uint8_t *hours);

/****************************** Private functions *****************************/

static ERtcStatus_t rtc_map_bsp_status(EBspStatus_t status) {
    ERtcStatus_t ret_val;

    switch (status) {
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
            ret_val = eRtcOk;
            break;
        default:
            ret_val = eRtcBusError;
            break;
    }

    return ret_val;
}
/*----------------------------------------------------------------------------*/

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

    ret_val = rtc_map_bsp_status(bsp_status);
    if (ret_val == eRtcOk) {
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
        }
    }

    return ret_val;
}
/******************************************************************************/

ERtcStatus_t rtc_ds3231_read_temperature(int16_t *temperature_quarters_c) {
    uint8_t register_address;
    uint8_t raw[DS3231_TEMP_REGISTER_COUNT];
    int16_t whole_degrees;
    int16_t fractional_quarters;
    EBspStatus_t bsp_status;
    ERtcStatus_t ret_val;

    if (temperature_quarters_c == NULL) {
        return eRtcInvalidParam;
    }

    register_address = DS3231_REG_TEMP_MSB;
    bsp_status = bspI2cWriteRead(
        DS3231_ADDRESS,
        &register_address,
        1U,
        raw,
        DS3231_TEMP_REGISTER_COUNT
    );
    ret_val = rtc_map_bsp_status(bsp_status);
    if (ret_val == eRtcOk) {
        if ((raw[0] & DS3231_TEMP_SIGN_MASK) != 0U) {
            whole_degrees = (int16_t)(
                (int16_t)raw[0] - DS3231_TEMP_SIGN_OFFSET
            );
        }
        else {
            whole_degrees = (int16_t)raw[0];
        }
        fractional_quarters = (int16_t)(
            (raw[1] & DS3231_TEMP_FRACTION_MASK) >>
            DS3231_TEMP_FRACTION_SHIFT
        );
        *temperature_quarters_c = (int16_t)(
            (whole_degrees * DS3231_TEMP_QUARTERS_PER_DEGREE) +
            fractional_quarters
        );
    }

    return ret_val;
}
/******************************************************************************/

ERtcStatus_t rtc_ds3231_configure_sqw_1hz(void) {
    uint8_t register_address;
    uint8_t control;
    uint8_t write_data[2];
    EBspStatus_t bsp_status;
    ERtcStatus_t ret_val;

    register_address = DS3231_REG_CONTROL;
    bsp_status = bspI2cWriteRead(
        DS3231_ADDRESS,
        &register_address,
        1U,
        &control,
        1U
    );
    ret_val = rtc_map_bsp_status(bsp_status);
    if (ret_val == eRtcOk) {
        control &= (uint8_t)(~DS3231_CTRL_SQW_CONFIG_MASK);
        write_data[0] = DS3231_REG_CONTROL;
        write_data[1] = control;
        bsp_status = bspI2cWriteRead(
            DS3231_ADDRESS,
            write_data,
            2U,
            NULL,
            0U
        );
        ret_val = rtc_map_bsp_status(bsp_status);
    }

    return ret_val;
}
/******************************************************************************/
