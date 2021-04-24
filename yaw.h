/** @file   yaw.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   24 April 2021
    @brief  Functions related to yaw monitoring.
*/


#ifndef YAW_H
#define YAW_H

#define DISC_SLOTS 112 // number of slots on the encoder disc
#define DEGREES_PER_REV 360 // number of degrees in a full revolution

// global yaw counter that tracks how many disc slots it is away from the origin
static volatile int16_t yawCounter = 0;

/* Enables GPIO B and initialises GPIOBIntHandler to run when the values on pins 0 or 1 change.  */
void initGPIO(void);

/** Interrupt handler for when the value on the pins monitoring yaw changes.
    Increments yawCounter if channel A leads (clockwise).
    Decrements yawCounter if channel B leads (counter-clockwise).  */
void GPIOBIntHandler(void);

/** Constrains yawCount between -2 * DISC_SLOTS and 2 * DISC_SLOTS.
    @param yawCount a counter for yaw.
    @return constrained yawCount.  */
int16_t yawConstrain(int16_t yawCount);

/** Converts yawCount to degrees.
    @param yawCount a counter for yaw.
    @return yawCount converted to degrees.  */
int16_t yawDegrees(int16_t yawCount);


#endif
