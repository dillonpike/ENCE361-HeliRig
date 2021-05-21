/** @file   yaw.h
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   21 May 2021
    @brief  Functions related to yaw monitoring.
*/

#ifndef YAW_H
#define YAW_H

#define DISC_SLOTS 112 // number of slots on the encoder disc
#define DEGREES_PER_REV 360 // number of degrees in a full revolution

extern volatile bool refYawFlag; // flag for refYawIntHandler to set so it can be handled in main loop

/** Enables GPIO port B and initialises YawIntHandler to run when the values on pins 0 or 1 change.  */
void initYawInt(void);

/** Enables GPIO port C and registers refYawIntHandler to run when the value on pin 4 is changes to low.  */
void initRefYawInt(void);

/** Assigns the initial states of channel A and B to aState and bState.  */
void initYawStates(void);

/** Interrupt handler for when the value on the pins monitoring yaw changes.
    Increments yawCounter if channel A leads (clockwise).
    Decrements yawCounter if channel B leads (counter-clockwise).  */
void YawIntHandler(void);

/** Sets the yawCounter to 0 so the reference yaw is at 0,
    sets a flag for the main loop, then disables the interrupt.  */
void refYawIntHandler(void);

/** Constrains yawCounter between -2 * DISC_SLOTS and 2 * DISC_SLOTS.  */
void yawConstrain(void);

/** Converts yawCounter to degrees and returns it.
    @return yawCounter converted to degrees.  */
int16_t getYawDegrees(void);

/** Enables PC4 interrupts to be handled and clears any interrupts generated while disabled.  */
void enableRefYawInt(void);

#endif /* YAW_H_ */
