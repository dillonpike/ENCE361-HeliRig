/** @file   yaw.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   24 April 2021
    @brief  Functions related to yaw monitoring.
*/


#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "yaw.h"

// yaw counter that tracks how many disc slots the reader is away from the origin
volatile int16_t yawCounter = 0;

/* Enables GPIO B and initialises GPIOBIntHandler to run when the values on pins 0 or 1 change.  */
void initGPIO(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_BOTH_EDGES);
    GPIOIntEnable(GPIO_PORTB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1);
    GPIOIntRegister(GPIO_PORTB_BASE, GPIOBIntHandler);
}

/** Interrupt handler for when the value on the pins monitoring yaw changes.
    Increments yawCounter if channel A leads (clockwise).
    Decrements yawCounter if channel B leads (counter-clockwise).  */
void GPIOBIntHandler(void)
{
    static bool aState = false; // arbitrary starting states
    static bool bState = false;

    uint32_t status = GPIOIntStatus(GPIO_PORTB_BASE, true);
    GPIOIntClear(GPIO_PORTB_BASE, status);

    if(status & GPIO_PIN_0) { // if channel A changes
        aState = !aState;
        if(aState != bState) { // if channel A leads
            yawCounter--;
        } else {
            yawCounter++;
        }
    } else {
        bState = !bState;
        if(aState != bState) { // if channel B leads
            yawCounter++;
        } else {
            yawCounter--;
        }
    }
    yawConstrain();
}

/** Constrains yawCounter between -2 * DISC_SLOTS and 2 * DISC_SLOTS.  */
void yawConstrain(void)
{
    if(yawCounter > DISC_SLOTS * 2) {
        yawCounter -= DISC_SLOTS * 4;
    } else if(yawCounter <= -2 * DISC_SLOTS) {
        yawCounter += DISC_SLOTS * 4;
    }
}

/** Converts yawCounter to degrees and returns it.
    @return yawCounter converted to degrees.  */
int16_t getYawDegrees(void)
{
    return yawCounter * DEGREES_PER_REV / (4 * DISC_SLOTS);
}
