/**
 * @file    i2c.c
 * @version 0.1.0
 * @authors Anton Chernov
 * @date    2026-11-21
 * @date    @showdate "%Y-%m-%d"
 */

/******************************** Included files ******************************/
#include <stddef.h>
#include "RTE_Components.h"
#include CMSIS_device_header
#include "bsp.h"

/********************************* Definitions ********************************/

/**
 * @def I2C_APB1_CLOCK_HZ
 * @brief APB1 clock frequency used by the STM32F103 I2C1 peripheral.
 */
#define I2C_APB1_CLOCK_HZ       36000000U

/**
 * @def I2C_STANDARD_MODE_HZ
 * @brief I2C standard-mode bus speed.
 */
#define I2C_STANDARD_MODE_HZ    100000U

/**
 * @def I2C_TIMEOUT_POLLS
 * @brief Maximum peripheral-status polls before a transfer times out.
 */
#define I2C_TIMEOUT_POLLS       100000U

/**
 * @def I2C_ADDRESS_MAX
 * @brief Largest legal unshifted 7-bit I2C address.
 */
#define I2C_ADDRESS_MAX         0x7FU

/**
 * @def I2C_ERROR_FLAGS
 * @brief I2C1 status flags treated as transfer errors.
 */
#define I2C_ERROR_FLAGS         (I2C_SR1_BERR | I2C_SR1_ARLO | \
                                 I2C_SR1_AF | I2C_SR1_OVR)

/****************************** Module variables ******************************/

static uint8_t i2c_initialized;

/***************************** Private prototypes *****************************/

static EBspStatus_t i2c_check_error(void);
static EBspStatus_t i2c_wait_sr1(uint32_t flag);
static EBspStatus_t i2c_wait_idle(void);
static EBspStatus_t i2c_begin_address(uint8_t address7, uint8_t read);
static void i2c_clear_address(void);
static EBspStatus_t i2c_write_bytes(
    uint8_t const *data,
    uint8_t length
);
static EBspStatus_t i2c_read_bytes(uint8_t *data, uint8_t length);

/****************************** Private functions *****************************/

static EBspStatus_t i2c_check_error(void) {
    uint32_t errors;
    EBspStatus_t ret_val;

    errors = I2C1->SR1 & I2C_ERROR_FLAGS;
    ret_val = eBspOk;
    if (errors != 0U) {
        I2C1->SR1 &= ~errors;
        if ((errors & I2C_SR1_AF) != 0U) {
            ret_val = eBspNack;
        }
        else {
            ret_val = eBspBusError;
        }
    }

    return ret_val;
}
/*----------------------------------------------------------------------------*/

static EBspStatus_t i2c_wait_sr1(uint32_t flag) {
    uint32_t timeout;
    EBspStatus_t ret_val;

    timeout = I2C_TIMEOUT_POLLS;
    ret_val = eBspTimeout;
    while (timeout > 0U) {
        ret_val = i2c_check_error();
        if (ret_val != eBspOk) {
            break;
        }
        if ((I2C1->SR1 & flag) != 0U) {
            ret_val = eBspOk;
            break;
        }
        timeout--;
    }

    return ret_val;
}
/*----------------------------------------------------------------------------*/

static EBspStatus_t i2c_wait_idle(void) {
    uint32_t timeout;
    EBspStatus_t ret_val;

    timeout = I2C_TIMEOUT_POLLS;
    ret_val = eBspTimeout;
    while (timeout > 0U) {
        ret_val = i2c_check_error();
        if (ret_val != eBspOk) {
            break;
        }
        if ((I2C1->SR2 & I2C_SR2_BUSY) == 0U) {
            ret_val = eBspOk;
            break;
        }
        timeout--;
    }

    return ret_val;
}
/*----------------------------------------------------------------------------*/

static EBspStatus_t i2c_begin_address(uint8_t address7, uint8_t read) {
    EBspStatus_t ret_val;
    uint32_t address_byte;

    I2C1->CR1 |= I2C_CR1_START;
    ret_val = i2c_wait_sr1(I2C_SR1_SB);
    if (ret_val == eBspOk) {
        (void)I2C1->SR1;
        address_byte = ((uint32_t)address7 << 1U) | (uint32_t)read;
        I2C1->DR = address_byte;
        ret_val = i2c_wait_sr1(I2C_SR1_ADDR);
    }

    return ret_val;
}
/*----------------------------------------------------------------------------*/

static void i2c_clear_address(void) {
    (void)I2C1->SR1;
    (void)I2C1->SR2;
}
/*----------------------------------------------------------------------------*/

static EBspStatus_t i2c_write_bytes(
    uint8_t const *data,
    uint8_t length
) {
    uint8_t index;
    EBspStatus_t ret_val;

    ret_val = eBspOk;
    for (index = 0U; index < length; index++) {
        ret_val = i2c_wait_sr1(I2C_SR1_TXE);
        if (ret_val != eBspOk) {
            break;
        }
        I2C1->DR = data[index];
    }

    if (ret_val == eBspOk) {
        ret_val = i2c_wait_sr1(I2C_SR1_BTF);
    }

    return ret_val;
}
/*----------------------------------------------------------------------------*/

static EBspStatus_t i2c_read_bytes(uint8_t *data, uint8_t length) {
    uint8_t index;
    uint8_t remaining;
    EBspStatus_t ret_val;

    ret_val = eBspOk;
    switch (length) {
        case 1U:
            I2C1->CR1 &= ~I2C_CR1_ACK;
            i2c_clear_address();
            I2C1->CR1 |= I2C_CR1_STOP;
            ret_val = i2c_wait_sr1(I2C_SR1_RXNE);
            if (ret_val == eBspOk) {
                data[0] = (uint8_t)I2C1->DR;
            }
            break;
        case 2U:
            /* NACK the final byte before clearing ADDR on a two-byte read. */
            I2C1->CR1 &= ~I2C_CR1_ACK;
            i2c_clear_address();
            ret_val = i2c_wait_sr1(I2C_SR1_BTF);
            if (ret_val == eBspOk) {
                I2C1->CR1 |= I2C_CR1_STOP;
                data[0] = (uint8_t)I2C1->DR;
                data[1] = (uint8_t)I2C1->DR;
            }
            break;
        default:
            I2C1->CR1 |= I2C_CR1_ACK;
            i2c_clear_address();
            remaining = length;
            index = 0U;
            while ((remaining > 3U) && (ret_val == eBspOk)) {
                ret_val = i2c_wait_sr1(I2C_SR1_RXNE);
                if (ret_val == eBspOk) {
                    data[index] = (uint8_t)I2C1->DR;
                    index++;
                    remaining--;
                }
            }
            if (ret_val == eBspOk) {
                ret_val = i2c_wait_sr1(I2C_SR1_BTF);
            }
            if (ret_val == eBspOk) {
                I2C1->CR1 &= ~I2C_CR1_ACK;
                data[index] = (uint8_t)I2C1->DR;
                index++;
                I2C1->CR1 |= I2C_CR1_STOP;
                data[index] = (uint8_t)I2C1->DR;
                index++;
                ret_val = i2c_wait_sr1(I2C_SR1_RXNE);
                if (ret_val == eBspOk) {
                    data[index] = (uint8_t)I2C1->DR;
                }
            }
            break;
    }

    return ret_val;
}
/*----------------------------------------------------------------------------*/

/********************* Application Programming Interface *********************/

EBspStatus_t bspI2cInit(void) {
    uint32_t ccr;

    i2c_initialized = 0U;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* PB6 (SCL) and PB7 (SDA): alternate-function open-drain, 50 MHz. */
    GPIOB->CRL = (GPIOB->CRL & ~(0xFFUL << 24U))
               | (0xFFUL << 24U);

    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0U;
    I2C1->CR2 = (I2C1->CR2 & ~I2C_CR2_FREQ)
              | (I2C_APB1_CLOCK_HZ / 1000000U);
    ccr = I2C_APB1_CLOCK_HZ / (2U * I2C_STANDARD_MODE_HZ);
    I2C1->CCR = ccr;
    I2C1->TRISE = (I2C_APB1_CLOCK_HZ / 1000000U) + 1U;
    I2C1->OAR1 = (1U << 14U);
    I2C1->CR1 = I2C_CR1_PE;

    i2c_initialized = 1U;
    return eBspOk;
}
/*----------------------------------------------------------------------------*/

EBspStatus_t bspI2cWriteRead(
    uint8_t address7,
    uint8_t const *tx_data,
    uint8_t tx_len,
    uint8_t *rx_data,
    uint8_t rx_len
) {
    EBspStatus_t ret_val;

    if ((address7 > I2C_ADDRESS_MAX) ||
        ((tx_len > 0U) && (tx_data == NULL)) ||
        ((rx_len > 0U) && (rx_data == NULL)) ||
        ((tx_len == 0U) && (rx_len == 0U))) {
        return eBspInvalidParam;
    }
    if (i2c_initialized == 0U) {
        return eBspNotInitialized;
    }

    ret_val = i2c_wait_idle();
    if (ret_val != eBspOk) {
        return ret_val;
    }
    I2C1->CR1 |= I2C_CR1_ACK;

    if (tx_len > 0U) {
        ret_val = i2c_begin_address(address7, 0U);
        if (ret_val == eBspOk) {
            i2c_clear_address();
            ret_val = i2c_write_bytes(tx_data, tx_len);
        }
        if ((ret_val == eBspOk) && (rx_len > 0U)) {
            ret_val = i2c_begin_address(address7, 1U);
        }
    }
    else {
        ret_val = i2c_begin_address(address7, 1U);
    }

    if (ret_val == eBspOk) {
        if (rx_len > 0U) {
            ret_val = i2c_read_bytes(rx_data, rx_len);
        }
        else {
            I2C1->CR1 |= I2C_CR1_STOP;
        }
    }

    if (ret_val != eBspOk) {
        I2C1->CR1 |= I2C_CR1_STOP;
    }
    I2C1->CR1 |= I2C_CR1_ACK;

    return ret_val;
}
/******************************************************************************/
