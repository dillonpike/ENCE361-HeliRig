/** @file   pi.h
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   21 May 2021
    @brief  Functions related to PI control for the main and tail rotors.
*/

#ifndef PI_H_
#define PI_H_

#include <stdint.h>

// Proportional and integral coefficients for main and tail rotor PI control
#define MAIN_PI_KP 0.6
#define MAIN_PI_KI 0.4

#define TAIL_PI_KP 0.43
#define TAIL_PI_KI 0.25

// Max and min duty cycles
#define PI_MAX 98
#define PI_MIN 2

#define FULL_ROTATION_DEG 360 // degrees of a full rotation

/** Calculates a PI control duty cycle to drive the main rotor based on a set and input altitude.  */
double mainPiCompute(uint8_t setPoint, int16_t input, double deltaT);

/** Calculates a PI control duty cycle to drive the tail rotor based on a set and input yaw.  */
double tailPiCompute(double setPoint, double input, double deltaT);

/** Sets main and tail error integrals to 0.  */
void resetErrorIntegrals(void);

#endif /* PI_H_ */
