/**********************************************************
 * Functions for generating a PWM output on a Tiva board
 * J4-05 (M0PWM7).
 * Author: P.J. Bones   UCECE
 *
 * Modified to generate a second PWM output (M1PWM5)
 * by Bailey Lissington, Dillon Pike, and Joseph Ramirez.
 *
 * Last modified:  30 April 2021
 **********************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h" //Needed for pin configure
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "buttons4.h"
#include "OrbitOLED/OrbitOLEDInterface.h" // Obtained from mdp46

/**********************************************************
 * Constants
 **********************************************************/
// Systick configuration
#define SYSTICK_RATE_HZ    100

// PWM configuration
#define PWM_START_RATE_HZ  250
#define PWM_RATE_STEP_HZ   50
#define PWM_RATE_MIN_HZ    50
#define PWM_RATE_MAX_HZ    400
#define PWM_START_DUTY     60
#define PWM_DUTY_STEP      5
#define PWM_DUTY_MIN       5
#define PWM_DUTY_MAX       95
#define PWM_DIVIDER_CODE   SYSCTL_PWMDIV_4
#define PWM_DIVIDER        4
#define PWM_TAIL_RATE_HZ   200
#define PWM_TAIL_DUTY      10

//  PWM Hardware Details M0PWM7 (gen 3)
//  ---Main Rotor PWM: PC5, J4-05
#define PWM_MAIN_BASE        PWM0_BASE
#define PWM_MAIN_GEN         PWM_GEN_3
#define PWM_MAIN_OUTNUM      PWM_OUT_7
#define PWM_MAIN_OUTBIT      PWM_OUT_7_BIT
#define PWM_MAIN_PERIPH_PWM  SYSCTL_PERIPH_PWM0
#define PWM_MAIN_PERIPH_GPIO SYSCTL_PERIPH_GPIOC
#define PWM_MAIN_GPIO_BASE   GPIO_PORTC_BASE
#define PWM_MAIN_GPIO_CONFIG GPIO_PC5_M0PWM7
#define PWM_MAIN_GPIO_PIN    GPIO_PIN_5

//  PWM Hardware Details M1PWM5
//  ---Tail Rotor PWM: PF1,
#define PWM_TAIL_BASE        PWM1_BASE
#define PWM_TAIL_GEN         PWM_GEN_2
#define PWM_TAIL_OUTNUM      PWM_OUT_5
#define PWM_TAIL_OUTBIT      PWM_OUT_5_BIT
#define PWM_TAIL_PERIPH_PWM  SYSCTL_PERIPH_PWM1
#define PWM_TAIL_PERIPH_GPIO SYSCTL_PERIPH_GPIOF
#define PWM_TAIL_GPIO_BASE   GPIO_PORTF_BASE
#define PWM_TAIL_GPIO_CONFIG GPIO_PF1_M1PWM5
#define PWM_TAIL_GPIO_PIN    GPIO_PIN_1

#define MAX_OLED_STR 17 // maximum allowable string for the OLED display

typedef enum {MAIN, TAIL} rotor; // rotor enumerator

/*******************************************
 *      Local prototypes
 *******************************************/
void SysTickIntHandler (void);
void initClocks (void);
void initSysTick (void);
void initialisePWM (void);
void setPWM (uint32_t u32Freq, uint32_t u32Duty, rotor chosenRotor);

/***********************************************************
 * Initialisation function for PWM Clock.
 ***********************************************************/
void
initPWMClock (void)
{
    // Set the PWM clock rate (using the prescaler)
    SysCtlPWMClockSet(PWM_DIVIDER_CODE);
}

/*********************************************************
 * initialisePWMMain
 * M0PWM7 (J4-05, PC5) is used for the main rotor motor
 *********************************************************/
void
initialisePWM (void)
{
    SysCtlPeripheralEnable(PWM_MAIN_PERIPH_PWM);
    SysCtlPeripheralEnable(PWM_MAIN_PERIPH_GPIO);

    GPIOPinConfigure(PWM_MAIN_GPIO_CONFIG);
    GPIOPinTypePWM(PWM_MAIN_GPIO_BASE, PWM_MAIN_GPIO_PIN);

    PWMGenConfigure(PWM_MAIN_BASE, PWM_MAIN_GEN,
                    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
    // Set the initial PWM parameters
    setPWM (PWM_START_RATE_HZ, PWM_START_DUTY, MAIN);

    PWMGenEnable(PWM_MAIN_BASE, PWM_MAIN_GEN);

    // Disable the output.  Repeat this call with 'true' to turn O/P on.
    PWMOutputState(PWM_MAIN_BASE, PWM_MAIN_OUTBIT, false);
}

/*********************************************************
 * initialisePWMTail
 * M1PWM5 (PF1) is used for the tail rotor motor.
 * Modified version of initialisePWMMain.
 *********************************************************/
void
initialisePWMTail (void)
{
    SysCtlPeripheralEnable(PWM_TAIL_PERIPH_PWM);
    SysCtlPeripheralEnable(PWM_TAIL_PERIPH_GPIO);

    GPIOPinConfigure(PWM_TAIL_GPIO_CONFIG);
    GPIOPinTypePWM(PWM_TAIL_GPIO_BASE, PWM_TAIL_GPIO_PIN);

    PWMGenConfigure(PWM_TAIL_BASE, PWM_TAIL_GEN,
                    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
    // Set the initial PWM parameters
    setPWM (PWM_TAIL_RATE_HZ, PWM_TAIL_DUTY, TAIL);

    PWMGenEnable(PWM_TAIL_BASE, PWM_TAIL_GEN);

    // Disable the output.  Repeat this call with 'true' to turn O/P on.
    PWMOutputState(PWM_TAIL_BASE, PWM_TAIL_OUTBIT, false);
}

/********************************************************
 * Function to set the freq and duty cycle of M0PWM7.
 * Modified to also set freq and duty cycle of M1PWM5.
 ********************************************************/
void
setPWM (uint32_t ui32Freq, uint32_t ui32Duty, rotor chosenRotor)
{
    // Calculate the PWM period corresponding to the freq.
    uint32_t ui32Period =
        SysCtlClockGet() / PWM_DIVIDER / ui32Freq;

    if (chosenRotor == MAIN) {
    PWMGenPeriodSet(PWM_MAIN_BASE, PWM_MAIN_GEN, ui32Period);
    PWMPulseWidthSet(PWM_MAIN_BASE, PWM_MAIN_OUTNUM,
        ui32Period * ui32Duty / 100);
    } else {
        PWMGenPeriodSet(PWM_TAIL_BASE, PWM_TAIL_GEN, ui32Period);
        PWMPulseWidthSet(PWM_TAIL_BASE, PWM_TAIL_OUTNUM,
            ui32Period * ui32Duty / 100);
    }
}