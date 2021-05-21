/** @file   yaw.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   21 May 2021
    @brief  Functions related to yaw monitoring.
*/

// standard library includes
#include <stdint.h>
#include <stdbool.h>

// library includes
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "yaw.h"

#define DISC_SLOTS 112 // number of slots on the encoder disc
#define EDGES_PER_SLOT 4 // total number of rising and falling edges per slot
#define DEGREES_PER_REV 360 // number of degrees in a full revolution

// global yaw counter variable that tracks how many disc slots the reader is away from the origin
static volatile int16_t yawCounter = 0;

// flag for refYawIntHandler to set so it can be handled in main loop
volatile bool refYawFlag = false;

// global variables that track the state of channel A and B
static volatile bool aState;
static volatile bool bState;

/** Enables GPIO port B and initialises YawIntHandler to run when the values on pins 0 or 1 change.  */
void initYawInt(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_BOTH_EDGES);
    GPIOIntRegister(GPIO_PORTB_BASE, YawIntHandler);
    GPIOIntEnable(GPIO_PORTB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1);
}

/** Enables GPIO port C and registers refYawIntHandler to run when the value on pin 4 is changes to low.  */
void initRefYawInt(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

    GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOIntTypeSet(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
    GPIOIntRegister(GPIO_PORTC_BASE, refYawIntHandler);
}

/** Assigns the initial states of channel A and B to aState and bState.  */
void initYawStates(void)
{
    aState = GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_0) & 1;
    bState = (GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_1) >> 1) & 1;
}

/** Interrupt handler for when the value on the pins monitoring yaw changes.
    Increments yawCounter if channel A leads (clockwise).
    Decrements yawCounter if channel B leads (counter-clockwise).  */
void YawIntHandler(void)
{
    uint32_t status = GPIOIntStatus(GPIO_PORTB_BASE, true);
    GPIOIntClear(GPIO_PORTB_BASE, status);

    if (status & GPIO_PIN_0) { // if channel A changes
        aState = !aState;
        if (aState != bState) { // if channel A leads
            yawCounter--;
        } else {
            yawCounter++;
        }
    } else {
        bState = !bState;
        if (aState != bState) { // if channel B leads
            yawCounter++;
        } else {
            yawCounter--;
        }
    }
    yawConstrain();
}

/** Sets the yawCounter to 0 so the reference yaw is at 0,
    sets a flag for the main loop, then disables the interrupt.  */
void refYawIntHandler(void)
{
    GPIOIntClear(GPIO_PORTC_BASE, GPIO_INT_PIN_4);
    yawCounter = 0;
    refYawFlag = true;
    GPIOIntDisable(GPIO_PORTC_BASE, GPIO_INT_PIN_4);
}

/** Constrains yawCounter between the negative and positive values of the counter at half a rotation.
    When decreasing below the limit, it changes to the maximum value, and vice versa.  */
void yawConstrain(void)
{
    if (yawCounter > DISC_SLOTS * EDGES_PER_SLOT / 2) {
        yawCounter -= DISC_SLOTS * EDGES_PER_SLOT;
    } else if (yawCounter <= -EDGES_PER_SLOT / 2 * DISC_SLOTS) {
        yawCounter += DISC_SLOTS * EDGES_PER_SLOT;
    }
}

/** Converts yawCounter to degrees and returns it.
    @return yawCounter converted to degrees.  */
int16_t getYawDegrees(void)
{
    return yawCounter * DEGREES_PER_REV / (EDGES_PER_SLOT * DISC_SLOTS);
}

/** Enables PC4 interrupts to be handled and clears any interrupts generated while disabled.  */
void enableRefYawInt(void)
{
    GPIOIntClear(GPIO_PORTC_BASE, GPIO_INT_PIN_4);
    GPIOIntEnable(GPIO_PORTC_BASE, GPIO_INT_PIN_4);
}
