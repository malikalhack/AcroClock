/**
 * @file    bsp.h
 * @version 0.1.0
 * @authors Anton Chernov
 * @date    2026-08-15
 * @date    @showdate "%Y-%m-%d"
 */

#ifndef BSP_H_
#define BSP_H_
/******************************** Included files ******************************/
#include <stdint.h>
/********************************* Definitions *******************************/

/**
 * @def BSP_TICKS_PER_SEC
 * @brief SysTick timer frequency in Hz (scheduler time base).
 */
#define BSP_TICKS_PER_SEC   1000U

/**
 * @enum EResetCause
 * @brief Cause of the most recent MCU reset, as reported by @ref reset_cause.
 */
enum EResetCause {
    eSoftware = 0, /**< Software reset (NVIC_SystemReset).       */
    eLowPower = 1, /**< Low-power / standby exit reset.          */
    eWatchDog = 2, /**< Independent or window watchdog reset.    */
    eBrownOut = 3, /**< Power-on / brown-out reset.              */
    ePin      = 4, /**< External NRST pin reset.                 */
    eUnknown  = 5  /**< Cause could not be determined.           */
};

/**
 * @enum EBspStatus
 * @brief Result of a board-support operation.
 */
typedef enum EBspStatus {
    eBspOk             = 0, /**< Operation completed successfully.     */
    eBspInvalidParam   = 1, /**< An argument is invalid.               */
    eBspNotInitialized = 2, /**< The peripheral is not initialized.    */
    eBspTimeout        = 3, /**< The operation timed out.              */
    eBspNack           = 4, /**< The I2C target did not acknowledge.   */
    eBspBusError       = 5  /**< The I2C peripheral reported an error. */
} EBspStatus_t;

/********************* Application Programming Interface *********************/

/**
 * @brief Initializes board peripherals and enables global interrupts.
 * @details Call from main() after startup has run SystemInit() (clocks, GPIO
 * and UART are already configured by then). Initializes I2C1 in standard mode
 * at 100 kHz before enabling interrupts.
 * @param[in] tick_source - application-owned 1 kHz counter used by SysTick.
 * @returns Initialization status.
 */
EBspStatus_t bspInit(volatile uint32_t *tick_source);

/**
 * @brief Get the system core clock frequency.
 * @returns System core clock frequency in Hz.
 */
uint32_t get_system_core_clock(void);

/** @brief Reset the MCU. */
void reset_mcu(void);

/**
 * @brief Reads and clears the cause of the last reset.
 * @returns One of @ref EResetCause.
 */
unsigned short reset_cause(void);

/**
 * @brief Arms the independent watchdog (IWDG) with a fixed timeout.
 * @details Once started the watchdog cannot be stopped except by a reset; the
 * application must call bspWatchdogKick() more often than the timeout or
 * the MCU is reset. Drives a nominal ~2 s timeout from the LSI.
 */
void bspWatchdogStart(void);

/** @brief Reloads (kicks) the independent watchdog to postpone its reset. */
void bspWatchdogKick(void);

/**
 * @brief Toggles the on-board LED connected to PC13.
 * @details The LED is active-low on the STM32F103C8 Blue Pill board.
 */
void bspLedToggle(void);

/**
 * @brief Initializes the I2C1 peripheral for synchronous master transfers.
 * @details Uses PB6/PB7, standard-mode 100 kHz and assumes external pull-ups.
 * @returns Initialization status.
 */
EBspStatus_t bspI2cInit(void);

/**
 * @brief Writes bytes and optionally reads bytes from a 7-bit I2C address.
 * @details When both lengths are non-zero, the read follows the write using a
 * repeated START. This blocking API is for task/main context, not an ISR.
 * @param[in] address7 - unshifted 7-bit I2C target address.
 * @param[in] tx_data - bytes to transmit, or NULL when tx_len is zero.
 * @param[in] tx_len - number of bytes to transmit.
 * @param[out] rx_data - receive buffer, or NULL when rx_len is zero.
 * @param[in] rx_len - number of bytes to receive.
 * @returns Transfer status.
 */
EBspStatus_t bspI2cWriteRead(
    uint8_t address7,
    uint8_t const *tx_data,
    uint8_t tx_len,
    uint8_t *rx_data,
    uint8_t rx_len
);

/**
 * @brief Transmits a single character over the debug UART (blocking).
 * @param[in] c - character to transmit.
 */
void uartSendChar(char c);

/**
 * @brief Receives a single character from the debug UART if available (non-blocking).
 * @details If no data is available, returns 0 and sets *c to 0.
 * @param[out] c - pointer to store the received character (not modified if no data).
 * @returns 1 if a character was received, 0 if no data available.
 */
uint8_t uartRecvChar(char *c);

/**
 * @brief Transmits a null-terminated string over the debug UART (blocking).
 * @param[in] str - pointer to the string to transmit (must not be NULL).
 */
void uartSendStr(const char *str);

/**
 * @brief Transmits an unsigned 8-bit integer as decimal digits over UART.
 * @param[in] n - value to transmit (0..255).
 */
void uartSendUint8(uint8_t n);

/**
 * @brief Transmits an unsigned 16-bit integer as decimal digits over UART.
 * @details Leading zeros are suppressed; the value 0 is printed as "0".
 * @param[in] n - value to transmit (0..65535).
 */
void uartSendUint16(uint16_t n);

/*****************************************************************************/
#endif //! BSP_H_
