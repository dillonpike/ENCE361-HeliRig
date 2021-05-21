/** @file   pacer.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   21 May 2021
    @brief  Functions related to implementing a pacer loop.
*/

#include <stdbool.h>

#include "pacer.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"

static uint32_t pacerPeriod; // desired pacer period in timer increments

/** Initialises the pacerPeriod and initialises TIMER0.  */
void initPacer(uint16_t freq) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    pacerPeriod = SysCtlClockGet() / freq;

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));

    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT_UP);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

/** Waits until a time equal to pacerPeriod has elapsed since the last timer reset, then resets the timer.  */
void pacerWait(void) {
    while (TimerValueGet(TIMER0_BASE, TIMER_A) < pacerPeriod);

    HWREG(TIMER0_BASE + TIMER_O_TBV) = 0;
    HWREG(TIMER0_BASE + TIMER_O_TAV) = 0;
}
