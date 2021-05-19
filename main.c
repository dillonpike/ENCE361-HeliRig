/** @file   main.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   22 April 2021
    @brief  Accepts an analogue input and displays an altitude based on it.
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
#define BACKGROUND_LOOP_FREQ_HZ 10

#define DESIRED_YAW_STEP 15 // increment/decrement step of yaw in degrees
#define DESIRED_ALT_STEP 10 // increment/decrement step of altitude in percentage

// RUNNING MODES. UNCOMMENT TO ENABLE
#define DEBUG // Debug mode. Displays useful info via serial

enum altDispMode {ALT_MODE_PERCENTAGE, ALT_MODE_RAW_ADC, ALT_MODE_OFF}; // Display mode enumerator
enum heliMode {LANDED, LAUNCHING, FLYING, LANDING};

// function prototypes
void initClock(void);
void SysTickIntHandler(void);
void ConfigureUART(void);
void uartDebugPrint(char* debugStr);

//main.c variable declarations
static uint8_t curHeliMode = LANDED;
static uint32_t clockRate;
static uint8_t desiredAltitude = 0;
static int16_t desiredYaw = 0;
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
    initPWMClock();
    initialisePWM();
    initialisePWMTail();
    IntMasterEnable();
    ConfigureUART();
    initPacer(BACKGROUND_LOOP_FREQ_HZ);

    // local variable declarations
    char dispStr[MAX_OLED_STR];
#ifdef DEBUG
    char debugStr[DEBUG_STR_LEN];
#endif
    uint32_t averageADC;
    int16_t altitudePercentage;

    initialAlt = altRead(); //takes first reading as initial alt (constant)
    uint8_t tailDuty;
    uint8_t mainDuty;

    int16_t refYaw;

    while (1)
    {
        averageADC = altRead();
        altitudePercentage = altitudeCalc(averageADC);
        int16_t yawDegrees = getYawDegrees();


        if (curHeliMode == LAUNCHING) {
            desiredYaw++;
            if (desiredYaw > 360) {
                desiredYaw = 0;
            }
        }
        if (refYawFlag) {
            curHeliMode = FLYING;
            refYaw = getRefYaw();
            desiredYaw = refYaw;
        }

        mainDuty = mainPidCompute(desiredAltitude, altitudePercentage, ((double)dTCounter)/SYSTICK_RATE_HZ);
        tailDuty = tailPidCompute(desiredYaw, yawDegrees, ((double)dTCounter)/SYSTICK_RATE_HZ);
        dTCounter = 0; // Reset dTCounter
        setPWMDuty(mainDuty, MAIN);
        setPWMDuty(tailDuty, TAIL);

#ifdef DEBUG
        usnprintf(debugStr, DEBUG_STR_LEN, "Alt: %4d [%3d]\n", altitudePercentage, desiredAltitude);
        UARTprintf(debugStr);
        usnprintf(debugStr, DEBUG_STR_LEN, "Yaw: %4d [%4d]\n", yawDegrees, desiredYaw);
        UARTprintf(debugStr);
        usnprintf(debugStr, DEBUG_STR_LEN, "Main: %3d Tail: %3d\n", mainDuty, tailDuty);
        UARTprintf(debugStr);
        usnprintf(debugStr, DEBUG_STR_LEN, "Mode: %1d\n", curHeliMode);
        UARTprintf(debugStr);
#endif
        usnprintf(dispStr, MAX_OLED_STR, "ALTITUDE: %4d%%", altitudePercentage);
        OLEDStringDraw(dispStr, 0, 0); //Position of altitude disp;
        usnprintf(dispStr, MAX_OLED_STR, "YAW: %4d", yawDegrees);
        OLEDStringDraw(dispStr, 0, 1); //Position of yaw display
        usnprintf(dispStr, DEBUG_STR_LEN, "Main: %2d", mainDuty);
        OLEDStringDraw(dispStr, 0, 2); //Position of main duty display
        usnprintf(dispStr, DEBUG_STR_LEN, "Tail: %2d", tailDuty);
        OLEDStringDraw(dispStr, 0, 3); //Position of tail duty display

        pacerWait();
    }
}




/* Configures the UART0 for USB Serial Communication. Referenced from TivaWare Examples. */
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

/* Initialisation of the clock and systick. */
void initClock(void)
{
    // Set the clock rate to 20 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    clockRate = SysCtlClockGet();
    SysTickPeriodSet(clockRate / SYSTICK_RATE_HZ);
    SysTickIntRegister(SysTickIntHandler);
    SysTickIntEnable();
    SysTickEnable();
}

/** Sets the initial altitude to the current circular buffer mean if the left button has been pushed.
    Cycles to the next altitude display mode if the up button has been pushed.  */
void SysTickIntHandler(void)
{
    ADCProcessorTrigger(ADC0_BASE, 0);
    updateButtons();
    if(checkButton(LEFT) == PUSHED)
        desiredYaw -= DESIRED_YAW_STEP;
    if(checkButton(RIGHT) == PUSHED)
        desiredYaw += DESIRED_YAW_STEP;
    if(checkButton(UP) == PUSHED)
        desiredAltitude = CONSTRAIN_PERCENT(desiredAltitude + DESIRED_ALT_STEP);
    if(checkButton(DOWN) == PUSHED)
        desiredAltitude = CONSTRAIN_PERCENT(desiredAltitude - DESIRED_ALT_STEP);
    if (checkButton(SWITCH1) == PUSHED && curHeliMode == LANDED) {
        curHeliMode = LAUNCHING;
        enableRefYawInt();
    }
    dTCounter++;
}
