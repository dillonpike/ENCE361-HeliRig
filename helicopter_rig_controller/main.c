// ************************************************************
// week2_blink
// Sample code for Week 2 ENCE361 Lab
// Author:  Phil Bones
// Last edited:   3.2.2018
// Based on Texas example code, and additions by Steve Weddell
//
// Purpose: This program will cause the LED connected to Pin 3
// of Port F to blink.
// ************************************************************
//

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// TivaWare Library Includes:
#include "inc/hw_memmap.h"        // defines GPIO_PORTF_BASE
#include "inc/tm4c123gh6pm.h"     // defines interrupt vectors
                                  //   and register addresses
#include "driverlib/gpio.h"       // defines for GPIO peripheral
#include "driverlib/sysctl.h"     // system control functions

#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3


int main(void) {
    uint32_t clock_rate;
    // Set up the system clock (refer to pp.220 of the TM4C123 datasheet for an overview).
    // Many options exist here but basically the 16MHz crystal oscillator signal is
    // boosted to 400Mz via a phase-locked loop (PLL), divided by 2 and then again by 10
    // (via the SYSCTL_SYSDIV_10 setting) to make the system clock rate 20 MHz.
    SysCtlClockSet(SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ | SYSCTL_SYSDIV_10);

    SysCtlDelay(100);  // Allow time for the oscillator to settle down.
    // SysCtlDelay() is an API function which executes a 3-instruction loop the number of
    //   times specified by the argument).

    clock_rate = SysCtlClockGet();  // Get the clock rate in pulses/s.

    // Enable GPIO Port F
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Set up the specific port pin as medium strength current & pull-down config.
    // Refer to TivaWare peripheral lib user manual for set up for configuration options
    GPIOPadConfigSet(GPIO_PORTF_BASE, GREEN_LED, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);

    // Set data direction register as output
    GPIODirModeSet(GPIO_PORTF_BASE, GREEN_LED, GPIO_DIR_MODE_OUT);

    // Write a zero to the output pin 3 on port F
    GPIOPinWrite(GPIO_PORTF_BASE, GREEN_LED, 0x00);

    // Enter a gadfly loop (kernel) to make the LED blink
    while (1)
    {
        //
        // Delay (passing the argument value clock_rate / 3 gives a delay of 1 sec)
        //
        SysCtlDelay(clock_rate / 12);

        //
        // Turn on the LED
        //
        GPIOPinWrite(GPIO_PORTF_BASE,  GREEN_LED, GREEN_LED);

        //
        // Delay
        //
        SysCtlDelay(clock_rate / 12);

        //
        // Turn off the LED
        //
        GPIOPinWrite(GPIO_PORTF_BASE,  GREEN_LED, 0x00);

    }
}
