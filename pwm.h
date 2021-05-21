/**********************************************************
 * Functions for generating a PWM output on a Tiva board
 * J4-05 (M0PWM7).
 * Author: P.J. Bones   UCECE
 *
 * Modified to generate a second PWM output (M1PWM5)
 * by Bailey Lissington, Dillon Pike, and Joseph Ramirez.
 *
 * Last modified: 21 May 2021
 **********************************************************/

#ifndef PWM_H_
#define PWM_H_

typedef enum {MAIN = 0, TAIL} rotor; // rotor enumerator

/***********************************************************
 * Initialisation function for PWM Clock.
 ***********************************************************/
void initPWMClock (void);

/*********************************************************
 * initialisePWMMain
 * M0PWM7 (J4-05, PC5) is used for the main rotor motor
 *********************************************************/
void initialisePWM (void);

/*********************************************************
 * initialisePWMTail
 * M1PWM5 (PF1) is used for the tail rotor motor.
 * Modified version of initialisePWM.
 *********************************************************/
void initialisePWMTail (void);

/********************************************************
 * Function to set the duty cycle of M0PWM7.
 * Modified to also set duty cycle of M1PWM5.
 ********************************************************/
void setPWMDuty (double duty, rotor chosenRotor);

#endif /* PWM_H_ */
