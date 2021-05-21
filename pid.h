/*
 * pid.h
 *
 *  Created on: 8/05/2021
 *      Author: lissi
 */

#ifndef PID_H_
#define PID_H_

#include <stdint.h>

#define MAIN_PID_KP 0.6
#define MAIN_PID_KI 0.4

#define TAIL_PID_KP 0.43
#define TAIL_PID_KI 0.25

#define PID_MAX 98
#define PID_MIN 2

#define FULL_ROTATION_DEG 360 // degrees of a full rotation

void initMainPid(void);
void initTailPid(void);

double mainPidCompute(uint8_t setPoint, int16_t input, double deltaT);
double tailPidCompute(double setPoint, double input, double deltaT);

/* Sets main and tail error integrals to 0.  */
void resetErrorIntegrals(void);

#endif /* PID_H_ */
