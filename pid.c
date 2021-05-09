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
    double deltaI = error * deltaT; // change in integral since last computation

    control = error * TAIL_PID_KP + (mainErrorIntegral + deltaI) * TAIL_PID_KI;

    if(control < PID_MIN) {
        control = PID_MIN;
    } else if (control > PID_MAX) {
        control = PID_MAX;
    } else {
        tailErrorIntegral += deltaI;
    }

    return control;
}
