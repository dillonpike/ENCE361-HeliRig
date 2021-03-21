// Macro function definitions
#define MS_TO_CYCLES(ms, clockRate) ((clockRate) / 1000 * (ms)) // Converts milliseconds into clock cycles
#define AVERAGE_OF_SUM(sum, n) ((2 * (sum) + (n)) / 2 / (n)) // Averages the sum
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "circBufT.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "OrbitOLED/OrbitOLEDInterface.h"
#include "utils/ustdlib.h"

#define RED_LED GPIO_PIN_1
#define BLUE_LED GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3
#define BUF_SIZE 10
#define SAMPLE_RATE_HZ 500
#define ADC_MAX_V 3.3
#define ALT_MAX_REDUCTION_V 0.8 // Voltage the altitude sensor reduces by at 100 % altitude
#define ADC_MAX 4095
#define MAX_ALT (ADC_MAX / ADC_MAX_V * ALT_MAX_REDUCTION_V) // Maximum altitude expressed as 12-bit int
#define TESTING
#define MAX_OLED_STR 17

void initADC(void);
void initClock (void);
void ADCIntHandler(void);
void SysTickIntHandler(void);
uint32_t bufferMean(circBuf_t* circBuf);
uint8_t altitudeCalc(uint32_t rawADC);

static circBuf_t circBufADC;
static uint32_t clockRate;
static volatile bool initialAltRead = false; // Has the initial altitude been read?
static uint32_t initialAlt;

int main(void)
{
    char dispStr[MAX_OLED_STR];
    uint32_t averageADC;
    uint8_t altitudePercentage;
    initClock();
    initADC();
    initCircBuf(&circBufADC, BUF_SIZE);
    OLEDInitialise();
    IntMasterEnable();

    // Block until initial altitude reading
    while(!initialAltRead);
    initialAlt = bufferMean(&circBufADC);

    while (1) {
        averageADC = bufferMean(&circBufADC);
        altitudePercentage = altitudeCalc(averageADC);
        // Constrain altitudePercentage to a valid range
        //altitudePercentage = altitudePercentage < 0 ? MIN(altitudePercentage, 0) : MAX(altitudePercentage, 100);
        OLEDStringDraw("Hello", 0, 0);
        usnprintf(dispStr, MAX_OLED_STR, "ALTITUDE: %3d", altitudePercentage);
        OLEDStringDraw(dispStr, 0, 0);
        SysCtlDelay(MS_TO_CYCLES(500, clockRate));
    }
}

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
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH09);
#endif
    ADCSequenceEnable(ADC0_BASE, 0);
    ADCIntRegister (ADC0_BASE, 0, ADCIntHandler);
    ADCIntEnable(ADC0_BASE, 0);
}

void initClock (void)
{
    // Set the clock rate to 20 MHz
    SysCtlClockSet (SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    clockRate = SysCtlClockGet();
    SysTickPeriodSet(clockRate / SAMPLE_RATE_HZ);
    SysTickIntRegister(SysTickIntHandler);
    SysTickIntEnable();
    SysTickEnable();
}

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

void SysTickIntHandler(void)
{
    ADCProcessorTrigger(ADC0_BASE, 0);
}

uint32_t bufferMean(circBuf_t* circBuf)
{
    // sum the buffer contents
    uint32_t cumSum = 0;
    uint16_t bufIndex;
    for (bufIndex = 0; bufIndex < BUF_SIZE; bufIndex++)
        cumSum += readCircBuf(circBuf);
    return AVERAGE_OF_SUM(cumSum, BUF_SIZE);
}

// Converts raw ADC to altitude in percentage
uint8_t altitudeCalc(uint32_t rawADC)
{
    int16_t alt = (int16_t)(initialAlt - rawADC) * 100 / MAX_ALT;
    return alt < 0 ? 0 : MIN(alt, 100);

}
