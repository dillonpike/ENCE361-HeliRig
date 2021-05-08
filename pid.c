/*
 * pid.c
 *
 *  Created on: 8/05/2021
 *      Author: lissi
 */

#include "pid.h"

static double mainErrorIntegral = 0;
static double tailErrorIntegral = 0;

double mainPidCompute(double setPoint, double input, double deltaT)
{
    double control;
    double error = setPoint - input;
    mainErrorIntegral += error * deltaT;

    control = error * MAIN_PID_KP + mainErrorIntegral * MAIN_PID_KI;
    return control;
}

double tailPidCompute(double setPoint, double input, double deltaT)
{
    double control;
    double error = setPoint - input;
    tailErrorIntegral += error * deltaT;

    control = error * MAIN_PID_KP + tailErrorIntegral * MAIN_PID_KI;
    return control;
}
