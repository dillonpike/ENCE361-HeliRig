/*
 * pid.c
 *
 *  Created on: 8/05/2021
 *      Author: lissi
 */

#include "pid.h"

static double mainPidErrorIntegral = 0;
static double tailPidErrorIntegral = 0;

double mainPidCompute(double setPoint, double input, double deltaT)
{
    double control;
    double error = setPoint - input;
    mainErrorIntegral += error * deltaT;

    control = error * MAIN_PID_KP + mainPidErrorIntegral * MAIN_PID_KI;
    return control;
}

double tailPidCompute(double setPoint, double input, double deltaT)
{
    double control;
    double error = setPoint - input;
    tailErrorIntegral += error * deltaT;

    control = error * MAIN_PID_KP + mainPidErrorIntegral * MAIN_PID_KI;
    return control;
}
