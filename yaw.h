/** @file   yaw.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   26 April 2021
    @brief  Functions related to yaw monitoring.
*/

#ifndef YAW_H
#define YAW_H


#define DISC_SLOTS 112 // number of slots on the encoder disc
#define DEGREES_PER_REV 360 // number of degrees in a full revolution

extern volatile bool refYawFlag;

/** Enables GPIO port B and initialises GPIOBIntHandler to run when the values on pins 0 or 1 change.  */
void initGPIO(void);

/** Enables GPIO port C and registers refYawIntHandler to run when the value on pin 4 is changes to low.  */
void initRefGPIO(void);

/** Assigns the initial states of channel A and B to aState and bState.  */
void initYawStates(void);

/** Interrupt handler for when the value on the pins monitoring yaw changes.
    Increments yawCounter if channel A leads (clockwise).
    Decrements yawCounter if channel B leads (counter-clockwise).  */
void GPIOBIntHandler(void);

/** Sets the current yaw to the reference yaw.  */
void refYawIntHandler(void);

/** Constrains yawCounter between -2 * DISC_SLOTS and 2 * DISC_SLOTS.  */
void yawConstrain(void);

/** Converts yawCounter to degrees and returns it.
    @return yawCounter converted to degrees.  */
int16_t getYawDegrees(void);

/** Returns the reference yaw.
    @return reference yaw.  */
int16_t getRefYaw(void);

/** Enables PC4 to generate interrupts.  */
void enableRefYawInt(void);

void resetYawCounter(void);

#endif
