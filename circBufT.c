// *******************************************************
// 
// circBufT.c
//
// Support for a circular buffer of uint32_t values on the 
// Tiva processor.
// P.J. Bones UCECE
// Last modified:  8.3.2017
// 
// bufferMean function and AVERAGE_OF_SUM macro added by
// Bailey Lissington, Dillon Pike, and Joseph Ramirez.
//
// Last modified: 21 May 2021
// *******************************************************

#include <stdint.h>
#include "stdlib.h"
#include "circBufT.h"

// Macro function definition
#define AVERAGE_OF_SUM(sum, n) ((2 * (sum) + (n)) / 2 / (n)) // Averages the sum

// *******************************************************
// initCircBuf: Initialise the circBuf instance. Reset both indices to
// the start of the buffer.  Dynamically allocate and clear the the 
// memory and return a pointer for the data.  Return NULL if 
// allocation fails.
uint32_t *
initCircBuf (circBuf_t *buffer, uint32_t size)
{
	buffer->windex = 0;
	buffer->rindex = 0;
	buffer->size = size;
	buffer->data = 
        (uint32_t *) calloc (size, sizeof(uint32_t));
	return buffer->data;
}
   // Note use of calloc() to clear contents.

// *******************************************************
// writeCircBuf: insert entry at the current windex location,
// advance windex, modulo (buffer size).
void
writeCircBuf (circBuf_t *buffer, uint32_t entry)
{
	buffer->data[buffer->windex] = entry;
	buffer->windex++;
	if (buffer->windex >= buffer->size)
	   buffer->windex = 0;
}

// *******************************************************
// readCircBuf: return entry at the current rindex location,
// advance rindex, modulo (buffer size). The function deos not check
// if reading has advanced ahead of writing.
uint32_t
readCircBuf (circBuf_t *buffer)
{
	uint32_t entry;
	
	entry = buffer->data[buffer->rindex];
	buffer->rindex++;
	if (buffer->rindex >= buffer->size)
	   buffer->rindex = 0;
    return entry;
}

// *******************************************************
// freeCircBuf: Releases the memory allocated to the buffer data,
// sets pointer to NULL and ohter fields to 0. The buffer can
// re-initialised by another call to initCircBuf().
void
freeCircBuf (circBuf_t * buffer)
{
	buffer->windex = 0;
	buffer->rindex = 0;
	buffer->size = 0;
	free (buffer->data);
	buffer->data = NULL;
}

/** Calculates the mean of the values stored in a circular buffer.
    @param address of circular buffer.
    @return mean of buffer values.  */
uint32_t
bufferMean(circBuf_t* circBuf)
{
    uint32_t cumSum = 0;
    uint16_t bufIndex;
    for (bufIndex = 0; bufIndex < circBuf->size; bufIndex++)
        cumSum += readCircBuf(circBuf);
    return AVERAGE_OF_SUM(cumSum, circBuf->size);
}

