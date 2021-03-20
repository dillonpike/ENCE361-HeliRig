#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "circBufT.h"
// TivaWare Library Includes:
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"


#define MS_TO_CYCLES(ms, clockRate) clockRate / 1000 * ms
#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3
#define BUF_SIZE 10
#define SAMPLE_RATE_HZ 10

void initADC(void);
void initClock (void);
void ADCIntHandler(void);
void SysTickIntHandler(void);

static circBuf_t circBufADC;
static uint32_t clockRate;

int main(void)
{
    uint32_t cumSum;
    uint16_t bufIndex;
    uint16_t val;
    initClock();
    initADC();
    initCircBuf(&circBufADC, BUF_SIZE);
    IntMasterEnable();

    while (1) {
        cumSum = 0;
        for (bufIndex = 0; bufIndex < BUF_SIZE; bufIndex++)
            cumSum += readCircBuf(&circBufADC);
        val = (2 * cumSum + BUF_SIZE) / 2 / BUF_SIZE;
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
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0); // CHANGE TO CH9 ONCE TESTING DONE
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
    uint32_t valADC;
    ADCSequenceDataGet(ADC0_BASE, 0, &valADC);
    writeCircBuf(&circBufADC, valADC);
    ADCIntClear(ADC0_BASE, 0);
}

void SysTickIntHandler(void)
{
    ADCProcessorTrigger(ADC0_BASE, 0);
}
