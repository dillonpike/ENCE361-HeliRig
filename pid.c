/*
 * pid.c
 *
 *  Created on: 8/05/2021
 *      Author: lissi
 */

#include "pid.h"


static double mainErrorIntegral = 0;
static double tailErrorIntegral = 0;

double mainPidCompute(uint8_t setAltitude, int16_t inputAltitude, double deltaT)
{
    double control;
    double error = setAltitude - inputAltitude;
    double deltaI = error * deltaT;

    control = error * MAIN_PID_KP + (mainErrorIntegral + deltaI) * MAIN_PID_KI;

    if(control < PID_MIN) {
        control = PID_MIN;
    } else if (control > PID_MAX) {
        control = PID_MAX;
    } else {
        mainErrorIntegral += deltaI;
    }

    return control;
}

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

    if(control < PID_MIN) {
        control = PID_MIN;
    } else if (control > PID_MAX) {
        control = PID_MAX;
    } else {
        tailErrorIntegral += deltaI;
    }

    return control;
}

/* Sets main and tail error integrals to 0.  */
void resetErrorIntegrals(void)
{
    mainErrorIntegral = 0;
    tailErrorIntegral = 0;
}
