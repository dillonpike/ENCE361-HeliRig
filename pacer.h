/** @file   pacer.h
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   21 May 2021
    @brief  Functions related to implementing a pacer loop.
*/

#ifndef PACER_H_
#define PACER_H_

#include <stdint.h>

/** Initialises the pacerPeriod and initialises TIMER0.  */
void initPacer(uint16_t freq);

/** Waits until a time equal to pacerPeriod has elapsed since the last timer reset, then resets the timer.  */
void pacerWait(void);

#endif /* PACER_H_ */
