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

void initGPIO(void);
void GPIOBIntHandler(void);
int16_t yawConstrain(int16_t yawCount);
int16_t yawDegrees(int16_t yawCount);


#endif
