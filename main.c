/** @file   main.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   22 April 2021
    @brief  Accepts an analogue input and displays an altitude based on it.
*/

// Macro function definition
#define MS_TO_CYCLES(ms, clockRate) ((clockRate) / 1000 * (ms)) // Converts milliseconds into clock cycles
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

// Constant definitions
#define SYSTICK_RATE_HZ 200 // rate of the systick clock
#define MAX_OLED_STR 17 // maximum allowable string for the OLED display
#define DEBUG_STR_LEN 30 // buffer size for uart debugging strings. Needs additional characters for newline, escape, zero
#define BLANK_OLED_STR "                " // blank string for OLED display
#define INSTRUCTIONS_PER_CYCLE 3  // number of instructions that sysctldelay performs each cycle
#define DISPLAY_DELAY 100 // OLED display refresh time (ms)

#define DESIRED_YAW_STEP 15 // increment/decrement step of yaw in degrees
#define DESIRED_ALT_STEP 10 // increment/decrement step of altitude in percentage

// RUNNING MODES. UNCOMMENT TO ENABLE
#define DEBUG // Debug mode. Displays useful info via serial

enum altDispMode {ALT_MODE_PERCENTAGE, ALT_MODE_RAW_ADC, ALT_MODE_OFF}; // Display mode enumerator
enum heliMode {LANDED = 0, LAUNCHING, FLYING, LANDING};
static const char* heliModeStr[] = {"LANDED", "LAUNCHING", "FLYING", "LANDING"};

// function prototypes
void initClock(void);
void SysTickIntHandler(void);
void ConfigureUART(void);
void checkFlags(void);
void displayInfoOLED(int16_t altitudePercentage, uint32_t averageADC, int16_t yawDegrees, uint8_t tailDuty, uint8_t mainDuty);
void displayInfoSerial(int16_t altitudePercentage, int16_t yawDegrees, uint8_t tailDuty, uint8_t mainDuty);

//main.c variable declarations
static uint8_t curAltDispMode = ALT_MODE_PERCENTAGE;
static uint8_t curHeliMode = LANDED;
static uint32_t clockRate;
static uint8_t desiredAltitude = 0;
static int16_t desiredYaw = 0;
static int16_t refYaw = 0;
static volatile uint8_t dTCounter = 0;

/** Main function of the MCU.  */
int main(void)
{
    // Initialization of peripherals
    initClock();
    initADC();
    initButtons();
    initCircBuf(&circBufADC, BUF_SIZE);
    OLEDInitialise();
    initGPIO();
    initYawStates();
    initRefGPIO();
    initPWMClock();
    initialisePWM();
    initialisePWMTail();
    IntMasterEnable();
    ConfigureUART();
    SysTickEnable();

    // local variable declarations
    uint32_t averageADC;
    int16_t altitudePercentage;

    initialAlt = altRead(); //takes first reading as initial alt (constant)
    uint8_t tailDuty;
    uint8_t mainDuty;

    while (1)
    {
        averageADC = altRead();
        altitudePercentage = altitudeCalc(averageADC);
        int16_t yawDegrees = getYawDegrees();

        if (curHeliMode == LAUNCHING) {
            mainDuty = 25;
            tailDuty = 35;
        } else if (curHeliMode == LANDING) {
            desiredYaw = refYaw;
            if (yawDegrees == refYaw) {
                desiredAltitude = 0;
            }
        }

        if(refYawFlag) {
            yawDegrees = 0;
            desiredYaw = 0;
            resetYawCounter();
            curHeliMode = FLYING;
            refYawFlag = false;
        }

        if (curHeliMode != LANDED && curHeliMode != LAUNCHING) {
            mainDuty = mainPidCompute(desiredAltitude, altitudePercentage, ((double)dTCounter)/SYSTICK_RATE_HZ);
            tailDuty = tailPidCompute(desiredYaw, yawDegrees, ((double)dTCounter)/SYSTICK_RATE_HZ);
        }
        dTCounter = 0; // Reset dTCounter
        setPWMDuty(mainDuty, MAIN);
        setPWMDuty(tailDuty, TAIL);

#ifdef DEBUG
        displayInfoSerial(altitudePercentage, yawDegrees, tailDuty, mainDuty);
#endif
        displayInfoOLED(altitudePercentage, averageADC, yawDegrees, tailDuty, mainDuty);
        SysCtlDelay(MS_TO_CYCLES(DISPLAY_DELAY, clockRate)/INSTRUCTIONS_PER_CYCLE);
    }
}

/* Displays altitude, yaw, main and tail duty cycles, and the mode of the helicopter to the Orbit OLED.  */
void displayInfoOLED(int16_t altitudePercentage, uint32_t averageADC, int16_t yawDegrees, uint8_t tailDuty, uint8_t mainDuty) {
    char dispStr[MAX_OLED_STR];

    // Sets different formatting of text depending on display mode of OLED
    switch(curAltDispMode) {
        case ALT_MODE_PERCENTAGE:
            usnprintf(dispStr, MAX_OLED_STR, "ALT: %4d [%3d]\n", altitudePercentage, desiredAltitude);
            break;
        case ALT_MODE_RAW_ADC:
            usnprintf(dispStr, MAX_OLED_STR, "ALTITUDE: %5d", averageADC);
            break;
        case ALT_MODE_OFF:
            usnprintf(dispStr, MAX_OLED_STR, BLANK_OLED_STR);
            break;
    }
    OLEDStringDraw(dispStr, 0, 0); //Position of altitude disp;

    usnprintf(dispStr, MAX_OLED_STR, "YAW: %4d [%4d]\n", yawDegrees, desiredYaw);
    OLEDStringDraw(dispStr, 0, 1); // Position of yaw display
    usnprintf(dispStr, DEBUG_STR_LEN, "M: %2d T: %2d", mainDuty, tailDuty);
    OLEDStringDraw(dispStr, 0, 2); // Duty values display
    usnprintf(dispStr, MAX_OLED_STR, "MODE: %9s", heliModeStr[curHeliMode]);
    OLEDStringDraw(dispStr, 0, 3); // Heli mode display
}

/* Displays altitude, yaw, main and tail duty cycles, and the mode of the helicopter to serial.  */
void displayInfoSerial(int16_t altitudePercentage, int16_t yawDegrees, uint8_t tailDuty, uint8_t mainDuty) {
    char debugStr[DEBUG_STR_LEN];
    usnprintf(debugStr, DEBUG_STR_LEN, "Alt: %4d [%3d]\n", altitudePercentage, desiredAltitude);
    UARTprintf(debugStr);
    usnprintf(debugStr, DEBUG_STR_LEN, "Yaw: %4d [%4d]\n", yawDegrees, desiredYaw);
    UARTprintf(debugStr);
    usnprintf(debugStr, DEBUG_STR_LEN, "Main: %3d Tail: %3d\n", mainDuty, tailDuty);
    UARTprintf(debugStr);
    usnprintf(debugStr, DEBUG_STR_LEN, "Mode: %s\n", heliModeStr[curHeliMode]);
    UARTprintf(debugStr);
}


/* Configures the UART0 for USB Serial Communication. Referenced from TivaWare Examples.  */
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

/* Initialisation of the clock and systick.  */
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
    updateButtons();
    if(checkButton(LEFT) == PUSHED)
        desiredYaw -= DESIRED_YAW_STEP;
        if (desiredYaw > 180) {
            desiredYaw -= 360;
        }
    if(checkButton(RIGHT) == PUSHED)
        desiredYaw += DESIRED_YAW_STEP;
    if (desiredYaw < -179) {
        desiredYaw += 360;
    }
    if(checkButton(UP) == PUSHED) {
        desiredAltitude = CONSTRAIN_PERCENT(desiredAltitude + DESIRED_ALT_STEP);
    }
    if(checkButton(DOWN) == PUSHED) {
        desiredAltitude = CONSTRAIN_PERCENT(desiredAltitude - DESIRED_ALT_STEP);
    }
    if (checkButton(SWITCH1) == PUSHED && curHeliMode == LANDED) {
        curHeliMode = LAUNCHING;
        enableRefYawInt();
    }
    if (checkButton(SWITCH1) == RELEASED && curHeliMode == FLYING) {
        curHeliMode = LANDING;
    }
    if (checkButton(RESET) == PUSHED) {
        SysCtlReset();
    }
    dTCounter++;
}
