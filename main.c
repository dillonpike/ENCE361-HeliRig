/** @file   main.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   22 April 2021
    @brief  Accepts an analogue input and displays an altitude based on it.
*/

// Macro function definition
#define MS_TO_CYCLES(ms, clockRate) ((clockRate) / 1000 * (ms)) // Converts milliseconds into clock cycles

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

// Constant definitions
#define SYSTICK_RATE_HZ 500 // rate of the systick clock
#define MAX_OLED_STR 17 // maximum allowable string for the OLED display
#define DEBUG_STR_LEN 20 // buffer size for uart debugging strings. Needs additional characters for newline, escape, zero
#define BLANK_OLED_STR "                " // blank string for OLED display
#define INSTRUCTIONS_PER_CYCLE 3  // number of instructions that sysctldelay performs each cycle
#define DISPLAY_DELAY 100 // OLED display refresh time (ms)

// RUNNING MODES. UNCOMMENT TO ENABLE
#define DEBUG // Debug mode. Displays useful info via serial

enum altDispMode {ALT_MODE_PERCENTAGE, ALT_MODE_RAW_ADC, ALT_MODE_OFF}; // Display mode enumerator

// function prototypes
void initClock(void);
void SysTickIntHandler(void);
void ConfigureUART(void);
void uartDebugPrint(char* debugStr);

//main.c variable declarations
static uint8_t curAltDispMode = ALT_MODE_PERCENTAGE;
static uint32_t clockRate;

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
    IntMasterEnable();
    ConfigureUART();

    // local variable declarations
    char dispStr[MAX_OLED_STR];
    uint32_t averageADC;
    int16_t altitudePercentage;

    initialAlt = altRead(); //takes first reading as initial alt (constant)

    while (1)
    {
        averageADC = altRead();
        altitudePercentage = altitudeCalc(averageADC);
        // Sets different formatting of text depending on display mode of OLED
        switch(curAltDispMode) {
            case ALT_MODE_PERCENTAGE:
                usnprintf(dispStr, MAX_OLED_STR, "ALTITUDE: %4d%%", altitudePercentage);
                break;
            case ALT_MODE_RAW_ADC:
                usnprintf(dispStr, MAX_OLED_STR, "ALTITUDE: %5d", averageADC);
                break;
            case ALT_MODE_OFF:
                usnprintf(dispStr, MAX_OLED_STR, BLANK_OLED_STR);
                break;
        }
        OLEDStringDraw(dispStr, 0, 0); //Position of altitude disp;


#ifdef DEBUG
        uartDebugPrint(dispStr);
#endif

        usnprintf(dispStr, MAX_OLED_STR, "YAW: %4d", getYawDegrees());

#ifdef DEBUG
        uartDebugPrint(dispStr);
#endif

        OLEDStringDraw(dispStr, 0, 1); //Position of yaw disp;
        SysCtlDelay(MS_TO_CYCLES(DISPLAY_DELAY, clockRate)/INSTRUCTIONS_PER_CYCLE);
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
    UARTStdioConfig(0, 115200, 16000000);
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
    if((checkButton(LEFT) == PUSHED))
        initialAlt = altRead(); // takes a new value for initialAlt
    if(checkButton(UP) == PUSHED)
        curAltDispMode = (curAltDispMode == ALT_MODE_OFF) ? ALT_MODE_PERCENTAGE : (curAltDispMode + 1);
}

void uartDebugPrint(char* debugStr)
{
    char uartStr[DEBUG_STR_LEN];
    uint8_t uartStrI = 0;
    while(debugStr[uartStrI] != 0) { // check if uartStrI is end of str
        uartStr[uartStrI] = debugStr[uartStrI];
        uartStrI++; // appends if uartStrI isn't end of str
    }
    if(debugStr[uartStrI-1] == '%')
        uartStr[uartStrI++] = '%'; // Escapes the % sign for printf
    uartStr[uartStrI++] = '\n';
    uartStr[uartStrI] = 0;
    UARTprintf(uartStr);
}
