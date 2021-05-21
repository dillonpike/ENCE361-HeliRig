/** @file   pi.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   21 May 2021
    @brief  Functions related to PI control for the main and tail rotors.
*/

#include "pi.h"

static double mainErrorIntegral = 0;
static double tailErrorIntegral = 0;

/** Calculates a PI control duty cycle to drive the main rotor based on a set and input altitude.  */
double mainPiCompute(uint8_t setAltitude, int16_t inputAltitude, double deltaT)
{
    double control;
    double error = setAltitude - inputAltitude;
    double deltaI = error * deltaT;

    control = error * MAIN_PI_KP + (mainErrorIntegral + deltaI) * MAIN_PI_KI;

    // Constrains control between PI_MIN and PI_MAX
    if (control < PI_MIN) {
        control = PI_MIN;
    } else if (control > PI_MAX) {
        control = PI_MAX;
    } else {
        mainErrorIntegral += deltaI; // adds to error integral if not constrained
    }

    return control;
}

/** Calculates a PI control duty cycle to drive the tail rotor based on a set and input yaw.  */
double tailPiCompute(double setPoint, double input, double deltaT)
{
    double control;
    double error = setPoint - input;

    // Calibrates error to the shortest signed difference between input and setPoint
    // since input and setPoint are constrained between -179 and 180
    // and transitions from -179 to 180 when decreasing, and vice versa
    if (error < -(FULL_ROTATION_DEG / 2)) {
        error += FULL_ROTATION_DEG;
    } else if (error > (FULL_ROTATION_DEG / 2)) {
        error -= FULL_ROTATION_DEG;
    }

    double deltaI = error * deltaT; // change in integral since last computation

    control = error * TAIL_PI_KP + (tailErrorIntegral + deltaI) * TAIL_PI_KI;

    // Constrains control between PI_MIN and PI_MAX
    if (control < PI_MIN) {
        control = PI_MIN;
    } else if (control > PI_MAX) {
        control = PI_MAX;
    } else {
        tailErrorIntegral += deltaI; // adds to error integral if not constrained
    }

    return control;
}

/** Sets main and tail error integrals to 0.  */
void resetErrorIntegrals(void)
{
    mainErrorIntegral = 0;
    tailErrorIntegral = 0;
}
