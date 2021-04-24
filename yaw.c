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

/** Constrains yawCount between -2 * DISC_SLOTS and 2 * DISC_SLOTS.
    @param yawCount a counter for yaw.
    @return constrained yawCount.  */
int16_t yawConstrain(int16_t yawCount)
{
    if(yawCount > DISC_SLOTS * 2) {
        yawCount -= DISC_SLOTS * 4;
    } else if(yawCount <= -2 * DISC_SLOTS) {
        yawCount += DISC_SLOTS * 4;
    }
    return yawCount;
}

/** Converts yawCount to degrees.
    @param yawCount a counter for yaw.
    @return yawCount converted to degrees.  */
int16_t yawDegrees(int16_t yawCount)
{
    return yawCount * DEGREES_PER_REV / (4 * DISC_SLOTS);
}

