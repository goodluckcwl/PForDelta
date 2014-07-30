/*
 * common.h
 *
 *  Created on: Apr 12, 2012
 *      Author: xczou
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define BITS_PER_INT (8*sizeof(uint32_t))

/*
 * print out binary string given by a number
 */
void int_to_bin(uint32_t num) ;

/*
 * free memory
 */
//void free_memory(void * p);

#define  free_memory(p) if ((p) != NULL) free(p); p = NULL;


/*calculate most significant bit position*/
//TODO : need a more efficient algorithm
uint8_t msb(uint32_t i);

#endif /* COMMON_H_ */
