/*
 * pacer.h
 *
 *  Created on: 19/05/2021
 *      Author: lissi
 */

#ifndef PACER_H_
#define PACER_H_

#include <stdint.h>

void initPacer(uint16_t freq);
void pacerWait(void);

#endif /* PACER_H_ */
