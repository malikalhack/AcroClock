/**
 * @file    debug_log.h
 * @version 0.1.1
 * @authors Anton Chernov
 * @date    2026-11-21
 * @date    @showdate "%Y-%m-%d"
 */

#ifndef DEBUG_LOG_H_
#define DEBUG_LOG_H_

/******************************** Included files ******************************/

/********************* Application Programming Interface *********************/

/**
 * @brief Writes an informational message to the debug UART.
 * @param[in] message - null-terminated message.
 */
void debug_log_info(char const *message);

/**
 * @brief Writes an error message to the debug UART.
 * @param[in] message - null-terminated message.
 */
void debug_log_error(char const *message);

/******************************************************************************/
#endif // !DEBUG_LOG_H_
