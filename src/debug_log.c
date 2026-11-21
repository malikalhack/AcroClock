/**
 * @file    debug_log.c
 * @version 0.1.0
 * @authors Anton Chernov
 * @date    2026-11-21
 * @date    @showdate "%Y-%m-%d"
 */

/******************************** Included files ******************************/
#include "bsp.h"
#include "debug_log.h"

/***************************** Private prototypes *****************************/

static void debug_log_message(char const *level, char const *message);

/****************************** Private functions *****************************/

static void debug_log_message(char const *level, char const *message) {
    uartSendStr(level);
    uartSendStr(message);
    uartSendChar('\r');
    uartSendChar('\n');
}

/********************* Application Programming Interface *********************/

void debug_log_info(char const *message) {
    debug_log_message("[INFO] ", message);
}
/*----------------------------------------------------------------------------*/

void debug_log_error(char const *message) {
    debug_log_message("[ERROR] ", message);
}
/******************************************************************************/
