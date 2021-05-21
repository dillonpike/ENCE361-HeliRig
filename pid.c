/** @file   pid.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   21 May 2021
    @brief  Functions related to PI control for the main and tail rotors.
*/

#include "pid.h"

static double mainErrorIntegral = 0;
static double tailErrorIntegral = 0;

/** Calculates a PI control duty cycle to drive the main rotor based on a set and input altitude.  */
double mainPidCompute(uint8_t setAltitude, int16_t inputAltitude, double deltaT)
{
    double control;
    double error = setAltitude - inputAltitude;
    double deltaI = error * deltaT;

    control = error * MAIN_PID_KP + (mainErrorIntegral + deltaI) * MAIN_PID_KI;

    // Constrains control between PID_MIN and PID_MAX
    if(control < PID_MIN) {
        control = PID_MIN;
    } else if (control > PID_MAX) {
        control = PID_MAX;
    } else {
        mainErrorIntegral += deltaI; // adds to error integral if not constrained
    }

    return control;
}

/** Calculates a PI control duty cycle to drive the tail rotor based on a set and input yaw.  */
double tailPidCompute(double setPoint, double input, double deltaT)
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

    control = error * TAIL_PID_KP + (tailErrorIntegral + deltaI) * TAIL_PID_KI;

    // Constrains control between PID_MIN and PID_MAX
    if(control < PID_MIN) {
        control = PID_MIN;
    } else if (control > PID_MAX) {
        control = PID_MAX;
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
