/*
 * pacer.c
 *
 *  Created on: 19/05/2021
 *      Author: lissi
 */

#include <stdbool.h>

#include "pacer.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"

uint32_t pacerPeriod; // desired pacer period in timer increments


void initPacer(uint16_t freq) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    pacerPeriod = SysCtlClockGet() / freq;
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT_UP);
    TimerEnable(TIMER0_BASE, TIMER_A);
}
void pacerWait(void) {
    uint32_t incrementsElapsed = TimerValueGet(TIMER0_BASE, TIMER_A);
    while(TimerValueGet(TIMER0_BASE, TIMER_A) < pacerPeriod);
    HWREG(TIMER0_BASE + TIMER_O_TBV) = 0;
    HWREG(TIMER0_BASE + TIMER_O_TAV) = 0;
}
