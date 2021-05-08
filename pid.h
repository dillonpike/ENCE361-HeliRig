/*
 * pid.h
 *
 *  Created on: 8/05/2021
 *      Author: lissi
 */

#ifndef PID_H_
#define PID_H_

#define MAIN_PID_KP 1
#define MAIN_PID_KI 1

#define TAIL_PID_KP 1
#define TAIL_PID_KI 1

void initMainPid(void);
void initTailPid(void);

float mainPidCompute(float setPoint, float input);
float tailPidCompute(float setPoint, float input);

#endif /* PID_H_ */
