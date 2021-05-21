/** @file   main.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   21 May 2021
    @brief  Monitors and controls the altitude and yaw of a helirig.
*/

// Macro function definition
#define MIN(a,b) (((a)<(b))?(a):(b)) // min of two numbers
#define MAX(a,b) (((a)>(b))?(a):(b)) // max of two numbers
#define CONSTRAIN_PERCENT(x) (MIN(MAX(0, (x)), 100)) // Constrains x to a valid percentage range  */

// standard library includes
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// library includes
#include "circBufT.h" // Obtained from P.J. Bones
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "OrbitOLED/OrbitOLEDInterface.h" // Obtained from mdp46
#include "utils/ustdlib.h"
#include "buttons4.h" // Obtained from P.J. Bones.
#include "utils/uartstdio.h"
#include "alt.h"
#include "yaw.h"
#include "pid.h"
#include "pwm.h"
#include "pacer.h"

// Constant definitions
#define SYSTICK_RATE_HZ 500 // rate of the systick clock
#define MAX_OLED_STR 17 // maximum allowable string for the OLED display
#define DEBUG_STR_LEN 30 // buffer size for uart debugging strings. Needs additional characters for newline, escape, zero
#define BACKGROUND_LOOP_FREQ_HZ 10  // frequency of background loop in main

#define HOVER_DESIRED_ALT 10 // desired altitude when finding hover point
#define DESIRED_YAW_STEP 15 // increment/decrement step of yaw in degrees
#define DESIRED_ALT_STEP 10 // increment/decrement step of altitude in percentage
#define LANDING_ALT_STEP 5 // decrement step of altitude when landing

#define TAIL_DUTY_REF 45 // tail rotor duty cycle for finding reference point

#define BUTTON_POLLING_RATE_HZ 100 // rate of button polling in Hz

// RUNNING MODES. UNCOMMENT TO ENABLE
#define DEBUG // Debug mode. Displays useful info via serial

// Heli mode enumerator and matching strings for output
enum heliMode {LANDED = 0, LAUNCHING, FLYING, LANDING};
static const char* heliModeStr[] = {"LANDED", "LAUNCHING", "FLYING", "LANDING"};

// function prototypes
void initClock(void);
void SysTickIntHandler(void);
void ConfigureUART(void);
void initProgram(void);
void displayInfoOLED(int16_t altitudePercentage, int16_t yawDegrees, uint8_t tailDuty, uint8_t mainDuty);
void displayInfoSerial(int16_t altitudePercentage, int16_t yawDegrees, uint8_t tailDuty, uint8_t mainDuty);

//main.c variable declarations
static uint8_t curHeliMode = LANDED;
static uint32_t clockRate;
static uint8_t desiredAltitude = 0;
static int16_t desiredYaw = 0;
static volatile uint8_t dTCounter = 0;
static bool canLaunch = false;
static uint8_t sysTickButtonCounter = 0;

/** Main function of the MCU.  */
int main(void)
{
    initProgram();

    // local variable declarations
    uint32_t averageADC = 0;
    int16_t altitudePercentage = 0;
    int16_t yawDegrees = 0;
    uint8_t tailDuty = 0;
    uint8_t mainDuty = 0;
    bool isHovering = false;

    initialAlt = altRead(); // Takes first reading as initial altitutde (constant)

    while (1)
    {
        averageADC = altRead();
        altitudePercentage = altitudeCalc(averageADC);
        yawDegrees = getYawDegrees();

        if ((curHeliMode == LAUNCHING)) {
            // Starts searching for reference yaw once heli is hovering
            if ((!isHovering) && (altitudePercentage) > 0) {
                isHovering = true;
                tailDuty = TAIL_DUTY_REF;
                enableRefYawInt();
            }
        } else if (curHeliMode == LANDING) {
            if (yawDegrees == desiredYaw) {
                // Gradually lowers altitude when heli is facing reference point
                desiredAltitude = CONSTRAIN_PERCENT(desiredAltitude - LANDING_ALT_STEP);
                if (altitudePercentage == 0) {
                    curHeliMode = LANDED;
                    mainDuty = 0;
                    tailDuty = 0; // turns off the motors
                    isHovering = false;
                    resetErrorIntegrals(); // resets error integrals so they don't affect next flight
                }
            }
        }

        // Sets current and desired yaw to 0 if flag set (heli at reference yaw)
        // and sets heli to flying mode
        if (refYawFlag) {
            yawDegrees = 0;
            desiredYaw = 0;
            curHeliMode = FLYING;
            refYawFlag = false;
        }

        if (curHeliMode != LANDED) {
            if(curHeliMode != LAUNCHING) {
                mainDuty = mainPidCompute(desiredAltitude, altitudePercentage, ((double)dTCounter)/SYSTICK_RATE_HZ);
                tailDuty = tailPidCompute(desiredYaw, yawDegrees, ((double)dTCounter)/SYSTICK_RATE_HZ);
            } else if (!isHovering) {
                // Sets a desired altitude so heli can find a main duty that allows it to hover
                mainDuty = mainPidCompute(HOVER_DESIRED_ALT, altitudePercentage, ((double)dTCounter)/SYSTICK_RATE_HZ);
                tailDuty = tailPidCompute(desiredYaw, yawDegrees, ((double)dTCounter)/SYSTICK_RATE_HZ);
            } else {
                // Adjusts main duty to keep altitude at desired point while searching for reference yaw
                mainDuty = mainPidCompute(desiredAltitude, altitudePercentage, ((double)dTCounter)/SYSTICK_RATE_HZ);
            }
        }
        dTCounter = 0; // Reset dTCounter
        setPWMDuty(mainDuty, MAIN);
        setPWMDuty(tailDuty, TAIL);

        #ifdef DEBUG
        displayInfoSerial(altitudePercentage, yawDegrees, tailDuty, mainDuty);
        #endif
        displayInfoOLED(altitudePercentage, yawDegrees, tailDuty, mainDuty);

        pacerWait();
    }
}

/** Initialises the peripherals, interrupts, serial output, circular buffer, and yaw channel states.  */
void initProgram(void) {
    initClock();
    initADC();
    initButtons();
    initCircBuf(&circBufADC, BUF_SIZE);
    OLEDInitialise();
    initYawInt();
    initYawStates();
    initRefYawInt();
    initPWMClock();
    initialisePWM();
    initialisePWMTail();
    IntMasterEnable();
    ConfigureUART();
    SysTickEnable();
    initPacer(BACKGROUND_LOOP_FREQ_HZ);
}

/** Displays altitude, yaw, main and tail duty cycles, and the mode of the helicopter to the Orbit OLED.  */
void displayInfoOLED(int16_t altitudePercentage, int16_t yawDegrees, uint8_t tailDuty, uint8_t mainDuty) {
    char dispStr[MAX_OLED_STR];

    usnprintf(dispStr, MAX_OLED_STR, "ALT: %4d [%4d]\n", altitudePercentage, desiredAltitude);
    OLEDStringDraw(dispStr, 0, 0); // Display current altitude and desired altitude on line 0

    usnprintf(dispStr, MAX_OLED_STR, "YAW: %4d [%4d]\n", yawDegrees, desiredYaw);
    OLEDStringDraw(dispStr, 0, 1); // Display current yaw and desired yaw on line 1

    usnprintf(dispStr, DEBUG_STR_LEN, "M: %2d T: %2d", mainDuty, tailDuty);
    OLEDStringDraw(dispStr, 0, 2); // Display main and tail duty cycles on line 2

    usnprintf(dispStr, MAX_OLED_STR, "MODE: %9s", heliModeStr[curHeliMode]);
    OLEDStringDraw(dispStr, 0, 3); // Display heli mode on line 3
}

/** Prints altitude, yaw, main and tail duty cycles, and the mode of the helicopter to serial.  */
void displayInfoSerial(int16_t altitudePercentage, int16_t yawDegrees, uint8_t tailDuty, uint8_t mainDuty) {
    char debugStr[DEBUG_STR_LEN];

    usnprintf(debugStr, DEBUG_STR_LEN, "Alt: %4d [%4d]\n", altitudePercentage, desiredAltitude);
    UARTprintf(debugStr); // Display current altitude and desired altitude

    usnprintf(debugStr, DEBUG_STR_LEN, "Yaw: %4d [%4d]\n", yawDegrees, desiredYaw);
    UARTprintf(debugStr); // Display current yaw and desired yaw

    usnprintf(debugStr, DEBUG_STR_LEN, "Main: %3d Tail: %3d\n", mainDuty, tailDuty);
    UARTprintf(debugStr); // Display main and tail duty cycles

    usnprintf(debugStr, DEBUG_STR_LEN, "Mode: %s\n", heliModeStr[curHeliMode]);
    UARTprintf(debugStr); // Display heli mode
}


/** Configures the UART0 for USB Serial Communication. Referenced from TivaWare Examples.  */
void ConfigureUART(void)
{
    // Enable the GPIO Peripheral used by the UART.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Enable UART0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Configure GPIO Pins for UART mode.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Use the internal 16MHz oscillator as the UART clock source.
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    // Initialize the UART for console I/O.
    UARTStdioConfig(0, 9600, 16000000);
}

/** Initialisation of the clock and systick.  */
void initClock(void)
{
    // Set the clock rate to 20 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    clockRate = SysCtlClockGet();
    SysTickPeriodSet(clockRate / SYSTICK_RATE_HZ);
    SysTickIntRegister(SysTickIntHandler);
    SysTickIntEnable();
}

/** Sets the initial altitude to the current circular buffer mean if the left button has been pushed.
    Cycles to the next altitude display mode if the up button has been pushed.  */
void SysTickIntHandler(void)
{
    ADCProcessorTrigger(ADC0_BASE, 0);
    // Checks buttons at a desired frequency
    if(sysTickButtonCounter >= (SYSTICK_RATE_HZ/BUTTON_POLLING_RATE_HZ)) {
        sysTickButtonCounter = 0;
        updateButtons();
        if (checkButton(LEFT) == PUSHED) {
            desiredYaw -= DESIRED_YAW_STEP;
            // Constrains yaw between half a rotation and negative half a rotation
            if (desiredYaw > (FULL_ROTATION_DEG / 2)) {
                desiredYaw -= FULL_ROTATION_DEG;
            }
        }
        if (checkButton(RIGHT) == PUSHED) {
            desiredYaw += DESIRED_YAW_STEP;
            // Constrains yaw between half a rotation and negative half a rotation
            if (desiredYaw < (-(FULL_ROTATION_DEG / 2) - 1)) {
                desiredYaw += FULL_ROTATION_DEG;
            }
        }
        if (checkButton(UP) == PUSHED) {
            desiredAltitude = CONSTRAIN_PERCENT(desiredAltitude + DESIRED_ALT_STEP);
        }
        if (checkButton(DOWN) == PUSHED) {
            desiredAltitude = CONSTRAIN_PERCENT(desiredAltitude - DESIRED_ALT_STEP);
        }
        bool sw1State = getState(SWITCH1);
        if ((sw1State) && (curHeliMode == LANDED)) {
            if (canLaunch) {
                curHeliMode = LAUNCHING;
            }
        } else if (!sw1State) {
            canLaunch = true;
            if(curHeliMode == FLYING) {
                curHeliMode = LANDING;
                desiredYaw = 0;
            }
        }
        if (getState(RESET)) {
            SysCtlReset();
        }
    }
    sysTickButtonCounter++;
    dTCounter++; // increments timer to use as deltaT in PI control
}
