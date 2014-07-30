/*
 * pfordelta.h
 *
 *  Created on: Apr 13, 2012
 *      Author: xczou
 */

#ifndef PFORDELTA_H_
#define PFORDELTA_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "common.h"


struct compressed_data {
	uint32_t min_bits_per_item; // min bits can encode most data
	// number of unpacked(input) data, the packed number bytes can be computed by
		//  (min_bits_per_item * unpackedNums) / BITS_PER_INT + (0/1)
	uint32_t unpacked_nums;
	uint32_t exception_nums; // total exception number
	//exception offset which indicates how far is next exception,
	// all the exceptions can be scanned by using exceptionNums
	uint32_t exception_offset; // first exception value offset
	uint32_t *packed; // encoded bit stream
	uint32_t *exceptions; // exception list
	// this is tricky flag for the 0 exception method
	// we have this flag to distinguish whether first value is 0 or not
	// 1 : yes, 0 : no
	uint8_t first_value ;

};

struct decompressed_data {
	uint32_t unpacked_nums; // number of data
	uint32_t *unpacked; // de-encoded data that is original input data
};


uint32_t get_compressed_size(struct compressed_data &cd);

/*
 * data: pointer to the input data
 * total_nums: total number of data
 * */
void pfordelta_compress(uint32_t* data, uint32_t total_nums, struct compressed_data *cd);

/*
 * input compressed_data get from compress step, decompression output is stored in
 * decompressed_data structure
 */
void pfordelta_decompress(struct compressed_data *cd, struct decompressed_data *dcd);

void print_compressed_data(struct compressed_data *cd);

void print_decompressed_data(struct decompressed_data *dcd) ;

/*
 * input : pass input data and its size
 * output : 1 means compression and decompression using pfordelta succeeds
 *          0 failed
 */
int test_pfordelta(uint32_t *data, uint32_t size) ;




#endif /* PFORDELTA_H_ */
