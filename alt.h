/** @file   alt.c
    @author Bailey Lissington, Dillon Pike, Joseph Ramirez
    @date   6 May 2021
    @brief  Functions related to altitude monitoring.
*/


#ifndef ALT_H
#define ALT_H


#define ADC_MAX 4095 // max raw value from the adc (2**12-1)
#define ADC_MAX_V 3.3 // Max voltage the ADC can handle
#define ALT_MAX_REDUCTION_V 1.0 // Voltage the altitude sensor reduces by at 100 % altitude
#define MAX_ALT (ADC_MAX / ADC_MAX_V * ALT_MAX_REDUCTION_V) // Maximum altitude expressed as 12-bit int
#define BUF_SIZE 10


// Global variables needed by alt.c and main.c
uint32_t initialAlt; // sets initial alt reading i.e. where 0% lies
circBuf_t circBufADC; // global since initCircBuf uses the same address of circBufADC


/** Gives reading for the raw ADC value of altitude.  */
uint32_t altRead(void);

/** Interrupt handler for when the ADC finishes conversion.
   Fills in the values for buffer in initial read.
   Waits for buffer to be filled before giving initialAltRead.  */
void ADCIntHandler(void);

/** Initialises the Analog to Digital Converter of the MCU.  */
void initADC(void);

/** Converts raw ADC to altitude percentage.
   @param raw ADC value.
   @return altitude percentage.  */
int16_t altitudeCalc(uint32_t rawADC);


#endif
