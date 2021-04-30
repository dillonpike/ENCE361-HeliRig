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
#include "yaw.h"

// Constant definitions
#define BUF_SIZE 10 // buffer size of the circular buffer
#define SYSTICK_RATE_HZ 500 // rate of the systick clock
#define ADC_MAX_V 3.3 // Max voltage the ADC can handle
#define ALT_MAX_REDUCTION_V 1.0 // Voltage the altitude sensor reduces by at 100 % altitude
#define ADC_MAX 4095 // max raw value from the adc (2**12-1)
#define MAX_ALT (ADC_MAX / ADC_MAX_V * ALT_MAX_REDUCTION_V) // Maximum altitude expressed as 12-bit int
#define MAX_OLED_STR 17 // maximum allowable string for the OLED display
#define DEBUG_STR_LEN 20 // buffer size for uart debugging strings
#define BLANK_OLED_STR "                " // blank string for OLED display
#define INSTRUCTIONS_PER_CYCLE 3  // number of instructions that sysctldelay performs each cycle
#define DISPLAY_DELAY 100 // OLED display refresh time (ms)


// RUNNING MODES. UNCOMMENT TO ENABLE
#define DEBUG // Debug mode. Displays useful info via serial
#define TESTING // Enables built-in potentiometer to be used instead of the rig's output

enum altDispMode {ALT_MODE_PERCENTAGE, ALT_MODE_RAW_ADC, ALT_MODE_OFF}; // Display mode enumerator

// function prototypes
void initADC(void);
void initClock(void);
void ADCIntHandler(void);
void SysTickIntHandler(void);
uint32_t bufferMean(circBuf_t* circBuf);
int16_t altitudeCalc(uint32_t rawADC);
void ConfigureUART(void);

// Global variable declarations
static uint8_t curAltDispMode = ALT_MODE_PERCENTAGE;
static circBuf_t circBufADC;
static uint32_t clockRate;
static volatile bool initialAltRead = false; // Has the initial altitude been read?
static uint32_t initialAlt;

/** Main function of the MCU.  */
int main(void)
{
    // local variable declarations
#ifdef DEBUG
    char debugStr[DEBUG_STR_LEN];
    uint8_t debugStrI;
#endif
    char dispStr[MAX_OLED_STR];
    uint32_t averageADC;
    int16_t altitudePercentage;

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

    // Block until initial altitude reading
    while (!initialAltRead);
    initialAlt = bufferMean(&circBufADC);

    while (1) {
        averageADC = bufferMean(&circBufADC);
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
#ifdef DEBUG
        // Appends newline and /0 characters to dispStr for better formatting over Serial Terminal
        debugStrI = 0;
        while(dispStr[debugStrI] != 0) {
            debugStr[debugStrI] = dispStr[debugStrI];
            debugStrI++;
        }
        if(dispStr[debugStrI-1] == '%')
            debugStr[debugStrI++] = '%'; // Escapes the % sign for printf
        debugStr[debugStrI++] = '\n';
        debugStr[debugStrI] = 0;
        UARTprintf(debugStr);
#endif
        OLEDStringDraw(dispStr, 0, 0);
        usnprintf(dispStr, MAX_OLED_STR, "YAW: %4d", getYawDegrees());
        OLEDStringDraw(dispStr, 0, 1);
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

/* Initialises the Analog to Digital Converter of the MCU.  */
void initADC(void)
{
    // Enable ADC0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));
    // Configure sequence
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
#ifdef TESTING
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0);
#else
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH9);
#endif
    ADCSequenceEnable(ADC0_BASE, 0);
    ADCIntRegister(ADC0_BASE, 0, ADCIntHandler);
    ADCIntEnable(ADC0_BASE, 0);
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

/** Interrupt handler for when the ADC finishes conversion.  */
void ADCIntHandler(void)
{
    static uint8_t sampleCount = 0;
    uint32_t valADC;
    ADCSequenceDataGet(ADC0_BASE, 0, &valADC);
    writeCircBuf(&circBufADC, valADC);
    ADCIntClear(ADC0_BASE, 0);
    if(sampleCount < BUF_SIZE) {
        sampleCount++;
    } else {
        initialAltRead = true;
    }
}

/** Sets the initial altitude to the current circular buffer mean if the left button has been pushed.
    Cycles to the next altitude display mode if the up button has been pushed.  */
void SysTickIntHandler(void)
{
    ADCProcessorTrigger(ADC0_BASE, 0);
    updateButtons();
    if(initialAltRead && (checkButton(LEFT) == PUSHED))
        initialAlt = bufferMean(&circBufADC);
    if(checkButton(UP) == PUSHED)
        curAltDispMode = (curAltDispMode == ALT_MODE_OFF) ? ALT_MODE_PERCENTAGE : (curAltDispMode + 1);
}

/** Converts raw ADC to altitude percentage.
    @param raw ADC value.
    @return altitude percentage.  */
int16_t altitudeCalc(uint32_t rawADC)
{
    int16_t alt_percent = (int16_t)(initialAlt - rawADC) * 100 / MAX_ALT;
    return alt_percent;
}
