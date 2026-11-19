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

/********************************* Definitions ********************************/

/**
 * @def CHECK_STATUS
 * @brief Checks the return value and, if unsuccessful, halts in error_handler.
 */
#define CHECK_STATUS(x) do { \
    if ((x) != eAcroOk) { \
        error_handler(); \
    } \
} while(0)

/****************************** Module variables ******************************/

static volatile AcroTick_t system_tick = 0U; /* incremented in a timer ISR */

/***************************** Private prototypes *****************************/

static void error_handler(void);

/****************************** Private functions *****************************/

static void error_handler(void) {
    /* handle error (e.g. log, reset, etc.) */
    while(1); /* halt */
}
/*----------------------------------------------------------------------------*/

static void myTask(AcroParam_t p) {
    (void)p;
    /* task body */
}

/********************** Application Programming Interface *********************/

void acroWatchdogRefresh(void) {
    /* Strong override: kick the hardware watchdog once per dispatcher
     * iteration. The kernel calls this only when the loop completes. */
    bspWatchdogKick();
}
/*----------------------------------------------------------------------------*/

int main(void) {
    CHECK_STATUS(acroInit(&system_tick));
    CHECK_STATUS(acroAddTask(1U, myTask, NULL, ePeriodic, 100U, 0U));
    IGNORE_RETURN(acroRun()); /* should never return */
    error_handler(); /* error handling if acroRun() returns */
}
/******************************************************************************/
