/*
 * common.c
 *
 *  Created on: Apr 12, 2012
 *      Author: xczou
 */
#include "common.h"

uint32_t single_set_bit_c(uint64_t value) {
    const uint64_t de_bruijn_multiplier = 0x022fdd63cc95386dull;
    const uint32_t de_bruijn_table[64] = {
        0,  1,  2,  53, 3,  7,  54, 27,
        4,  38, 41,  8, 34, 55, 48, 28,
        62, 5,  39, 46, 44, 42, 22, 9,
        24, 35, 59, 56, 49, 18, 29, 11,
        63, 52, 6,  26, 37, 40, 33, 47,
        61, 45, 43, 21, 23, 58, 17, 10,
        51, 25, 36, 32, 60, 20, 57, 16,
        50, 31, 19, 15, 30, 14, 13, 12,
    };    
    return de_bruijn_table[(value * de_bruijn_multiplier) >> 58];
}

uint32_t highest_set_bit_c(uint64_t value) {
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    
    value = value ^ (value >> 1);
    
    return single_set_bit_c(value);
}

void int_to_bin(uint32_t num) {
	char str[32] = { 0 };
	int i;
	for (i = 31; i >= 0; i--) {
		str[i] = (num & 1) ? '1' : '0';
		num >>= 1;
	}
	printf("%s\n", str);

}



/*void free_memory(void * p) {
	if (p != NULL)
		free(p);
}*/


uint8_t msb(uint32_t i) {
	/*uint8_t r = 0;
	while (i > 0) {
		i = i >> 1;
		r++;
	}
	return r;*/
	return highest_set_bit_c(i) + 1;
}

