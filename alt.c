/** @file   alt.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   21 May 2021
    @brief  Functions related to altitude monitoring.
*/

// standard library includes
#include <stdint.h>
#include <stdbool.h>

// library includes
#include "circBufT.h" // Obtained from P.J. Bones
#include "inc/hw_memmap.h"
#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "alt.h"

//#define TESTING // Enables built-in potentiometer to be used instead of the rig's output

static volatile bool initialAltRead = false; // Has the initial altitude been read?
static volatile uint8_t sampleCount = 0; //Counter comparing to BUF_SIZE; interrupt to get the mean initial read

/** Calculates the raw ADC mean of the circular buffer and returns it.
    @return average raw ADC.  */
uint32_t altRead(void)
{
    while (!initialAltRead); // Block until initial altitude reading

    uint32_t readAlt = bufferMean(&circBufADC);
    return readAlt;
}

/** Interrupt handler for when the ADC finishes conversion.
    Fills in the values for buffer in initial read.
    Waits for buffer to be filled before giving initialAltRead.  */
void ADCIntHandler(void)
{
    uint32_t valADC;
    ADCSequenceDataGet(ADC0_BASE, 0, &valADC);
    writeCircBuf(&circBufADC, valADC);
    ADCIntClear(ADC0_BASE, 0);
    if (sampleCount < BUF_SIZE) {
        sampleCount++;
    } else {
        initialAltRead = true;
    }
}

/** Initialises the Analog to Digital Converter of the MCU.  */
void initADC(void)
{
    // Enable ADC0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));
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

/** Converts raw ADC to altitude percentage.
    @param raw ADC value.
    @return altitude percentage.  */
int16_t altitudeCalc(uint32_t rawADC)
{
    int16_t alt_percent = (int16_t)(initialAlt - rawADC) * 100 / MAX_ALT;
    return alt_percent;
}
