/*
 * pfordelta.c
 *
 *  Created on: Mar 29, 2012
 *      Author: xczou
 */

#include "pfordelta.h"
#include <string.h>

uint32_t COMPRESSWIDTH = 8;

float THRESHOLD = 0.9;

/*for efficiency, we enumerate all the possible maximum value */
uint32_t maxValue[33] = { 0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047,
		4095, 8191, 16383, 32767, 65535, 131071, 262143, 524287, 1048575,
		2097151, 4194303, 8388607, 16777215, 33554431, 67108863, 134217727,
		268435455, 536870911, 1073741823, 2147483647, 4294967295 };


//WARNING :  All the value should fits certain bits
// Otherwise will cause problem
void write_to_bitstream(uint32_t total_nums, uint32_t min_bits_per_item,
		uint32_t *unpacked, uint32_t *packed, uint32_t *offset) {
	uint32_t bit_offset = BITS_PER_INT; // use 32,16,or 8 bits as packed bit window
	int diff;
	uint32_t i;
	packed[*offset] = 0;
	for (i = 0; i < total_nums; i++) {
		diff = bit_offset - min_bits_per_item;
		if (diff > 0) {
			packed[*offset] |= (unpacked[i] << diff);
			bit_offset -= min_bits_per_item;
		} else if (diff == 0) {
			packed[(*offset)++] |= unpacked[i];
			packed[(*offset)] = 0;
			bit_offset = BITS_PER_INT;
		} else {
			packed[(*offset)++] |= (unpacked[i] >> (-diff));
			packed[*offset] = (unpacked[i] << (BITS_PER_INT + diff));
			bit_offset = BITS_PER_INT + diff;
		}
	}
	if (bit_offset != BITS_PER_INT) {
		(*offset)++;
	}
}

//retrieve integer values from bitfield buffer, put into result,
// return the offset of the next int in the buffer stream
void read_from_bitstream(uint32_t nitems, uint32_t min_bits_per_item,
		uint32_t *unpacked_buffer, uint32_t *packed_buffer, uint32_t *offset) {
	//construct bit_mask
	uint32_t mask = 0;
	uint32_t i;
	for (i = 0; i < min_bits_per_item; i++) {
		mask |= 1 << i;
	}

	int bit_offset = BITS_PER_INT;
	int diff;
	uint32_t value = packed_buffer[*offset];

	for (i = 0; i < nitems; i++) {
		diff = bit_offset - min_bits_per_item;
		if (diff > 0) {
			unpacked_buffer[i] = (value >> diff) & mask;
			bit_offset -= min_bits_per_item;
		}
		//perfectly aligned
		else if (diff == 0) {
			unpacked_buffer[i] = value & mask;
			if (i < nitems - 1) {
				value = packed_buffer[++(*offset)];
				bit_offset = BITS_PER_INT;
			}
		}
		//partly not in this int
		else if (diff < 0) {
			unpacked_buffer[i] = (value << (-diff)) & mask;
			value = packed_buffer[++(*offset)];
			unpacked_buffer[i] |= (value >> (BITS_PER_INT + diff)) & mask;
			bit_offset = BITS_PER_INT + diff;
		}
	}

	if (bit_offset != BITS_PER_INT) {
		(*offset)++;
	}

}

/*
 * calculate delta between two adjacent data
 */
uint32_t *calculate_delta(uint32_t total_nums, uint32_t *delta, uint32_t *data) {
	uint32_t i;
	uint32_t predecessor = 0;
//	printf("calculating  delta data : \n ");
//	printf(" data : \n ");
	for (i = 0; i < total_nums; i++) {
		uint32_t v = data[i];
//		printf("%d , ", v);
		delta[i] = v - predecessor;
//		printf("%d , ", delta[i]);
		predecessor = v;
	}
//	printf("\n");
	return delta;
}

uint32_t get_compressed_size(struct compressed_data &cd) {
	return ceil(1.0 * cd.min_bits_per_item * cd.unpacked_nums / 8) + (cd.exception_nums * sizeof(uint32_t));
}
/*
 * data: pointer to the input data
 * total_nums: total number of data
 * implicitly requirement is num/size must be a multiple of 32
 * */
void pfordelta_compress(uint32_t* data, uint32_t total_nums,
		struct compressed_data *cd) {
	/*In memory, treat all data in 4 bytes way * */
	if (total_nums <= 0) {
		printf("total nums is not positive ! \n");
		return;
	}
	data[0] == 0 ? (cd->first_value = 1) : (cd->first_value = 0);
//	printf("Starting to compress by using threshold value %f \n", THRESHOLD);
	uint32_t i; // loop variable
	uint32_t *delta = (uint32_t*) malloc(sizeof(uint32_t) * total_nums);

	calculate_delta(total_nums, delta, data);

	uint32_t min_bits_per_item = 0;
	int exceptionNums = 0;
	uint8_t *least_bits = (uint8_t *) malloc(sizeof(uint8_t) * total_nums);
	uint8_t maxbits = 0;
	/*the longest bit length will not exceed 32, uint8_t type is enough*/
//	printf("determine the min. bits to decode most of values: \n");
	for (i = 0; i < total_nums; i++) {
		least_bits[i] = msb(delta[i]); //TODO : need to optimize the msb function
		if (least_bits[i] > maxbits)
			maxbits = least_bits[i];
//		printf("%d , ", least_bits[i]);
	}
//	printf("max bit : %d", maxbits);

	if (maxbits <= 0) { //no need to goes down
		printf("error: maximum bits is less and equals to 0 \n");
		return;
	}
	uint32_t blen = sizeof(uint32_t) * (maxbits + 1);
	uint32_t * buckets = (uint32_t *) malloc(blen);
	memset(buckets, 0, blen);

	/*bits usage stat.*/
	for (i = 0; i < total_nums; i++) {
		buckets[least_bits[i]]++;
	}
//	printf("\n bit buckets (number of value encoded by certain bits): ");
//	for (i = 0; i < (maxbits + 1); i++) {
//		printf("%d,", buckets[i]);
//	}

//	printf("\n accumulated buckets : 0, ");
	/*bits usage accumulated , start from 2nd position*/
	for (i = 1; i < (maxbits + 1); i++) {
		buckets[i] = buckets[i] + buckets[i - 1];
//		printf("%d,", buckets[i]);
	}

	uint8_t found = 0;
	/* estimate the threshold */
	for (i = 1; i < (maxbits + 1) && !found; i++) {
		if ((float) buckets[i] / (float) total_nums >= THRESHOLD) {
			min_bits_per_item = i;
			exceptionNums = total_nums - buckets[i];
			found = 1;
//			printf("\n and exception number is %d \n", exceptionNums);
		}
	}

	if (min_bits_per_item == 0) { // not found the best bit width
		printf("Not found the min. bits to decode the non-exception values!\n");
		return;
	}

	printf("decide to using %u bits for encoding non-exception values \n ",
			min_bits_per_item);

	/*printf(
	 "potential maximum number of exceptions values are [%u]\n",
	 maxNumOfEnforcedEpt + exceptionNums);*/

	uint32_t actualEptNums = 0;
	uint32_t preException = 0;
	if (exceptionNums == 0) { // no exception
		cd->exception_offset = 0;
		cd->exceptions = 0;
	} else {
		uint32_t * exceptions = (uint32_t *) malloc(
				sizeof(uint32_t) * (exceptionNums ));
		for (i = 0; i < total_nums; i++) {
			if (least_bits[i] > min_bits_per_item) { // exception
				exceptions[actualEptNums++] = delta[i];
				delta[i] = 0; // all exceptions are substituted by 0
			}
		}
		//in all 0 exception version, the offset is useless,
		// setting first exception offset to 0 is acceptable
		cd->exception_offset = 0;

	/*	printf("after forced exception, delta \n: ");
		for (i = 0; i < total_nums; i++) {
			printf("%d , ", delta[i]);
		}
		printf("\n");*/
		cd->exceptions = exceptions;
	}

	/* each delta value can be expressed via 'usingbits' bits */
	/* there is 'length' of delta values */
	uint32_t packedNums = (min_bits_per_item * total_nums) / BITS_PER_INT;
	// BugFix: allocate one extra memory space no matter the total number of input data
	// is divisible by 32. If the total input number is divisible by 32, we still need extra 1 memory
	// because the offset will access this extra space at the end.
	uint32_t *packed = (uint32_t *) malloc(sizeof(uint32_t) * (packedNums + 1));
	uint32_t offset = 0;
	write_to_bitstream(total_nums, min_bits_per_item, delta, packed, &offset);

	cd->packed = packed;
	cd->unpacked_nums = total_nums;
	cd->exception_nums = actualEptNums;
	cd->min_bits_per_item = min_bits_per_item;
	free_memory(buckets);
	free_memory(least_bits);
	free_memory(delta);
}

void pfordelta_decompress(struct compressed_data *cd,
		struct decompressed_data *dcd) {
	uint32_t a = 0;
	uint32_t total_nums = cd->unpacked_nums;
//	printf("unpacking values with size of [%u]: ", total_nums);
	dcd->unpacked = (uint32_t *) malloc(total_nums * sizeof(uint32_t));
	dcd->unpacked_nums = total_nums;
//	dcd->unpacked[total_nums-1]=0;
	read_from_bitstream(total_nums, cd->min_bits_per_item, dcd->unpacked,
			cd->packed, &a);
//	printf("a[%u],\n",a);
	/*patch approach to the exception list*/

	uint32_t i ;
	uint32_t j = 0;
	// scan entire unpacked list, substitute 0 value with corresponding
	// exception value
	for ( i = 0 ; i < total_nums ; i ++){
		if (dcd->unpacked[i] == 0){
			// first value of raw data is 0,we dont treat it as an exception
			if ( i ==0 && cd->first_value == 1) {
				continue;
			}
			dcd->unpacked[i] = cd->exceptions[j++];
		}
	}

	// Have to start from 1, instead of 0, because 0 position value is the original value
	// we accumulate the delta value to recover to input data
	for (i = 1; i < total_nums; i++) {
		dcd->unpacked[i] = dcd->unpacked[i] + dcd->unpacked[i - 1];
	}
// read_from_bitstream
}

void print_compressed_data(struct compressed_data *cd) {
	uint32_t i;
	uint32_t packedNums = (cd->min_bits_per_item * cd->unpacked_nums)
			/ BITS_PER_INT;
	uint32_t divisible = (cd->min_bits_per_item * cd->unpacked_nums)
			% BITS_PER_INT;
	packedNums = packedNums + (divisible > 0 ? 1 : 0);
	printf("\n compressed data with size [%u] : ", packedNums);
	for (i = 0; i < packedNums; i++) {
		printf("%u , ", cd->packed[i]);
	}

	printf("\n exception data with size [%u]: ", cd->exception_nums);
	for (i = 0; i < cd->exception_nums; i++) {
		printf("%u ,", cd->exceptions[i]);
	}
}

void print_decompressed_data(struct decompressed_data *dcd) {
	printf("\n decompressed data with size [%u] : \n", dcd->unpacked_nums);
	uint32_t i;
	for (i = 0; i < dcd->unpacked_nums; i++) {
		printf("%u , ", dcd->unpacked[i]);
	}
}

/*test two array with same size have  equal elements*/
int test_array_equality(uint32_t *data1, uint32_t **data2, uint32_t size) {
	uint32_t i;
	for (i = 0; i < size; i++) {
		if (data1[i] != (*data2)[i]) //
			return 0;
	}
	return 1;
}

int test_pfordelta(uint32_t *data, uint32_t size) {
	struct compressed_data cd;
	pfordelta_compress(data, size, &cd);

	struct decompressed_data dcd;
	pfordelta_decompress(&cd, &dcd);
	int r = 0;
	r = test_array_equality(data, &(dcd.unpacked), size);
	free_memory(cd.exceptions);
	free_memory(cd.packed);
	free_memory(dcd.unpacked);

	return r;

}

void test_suite() {
	printf("1st Test Begins \n ");
	uint32_t noException[32] = { 4, 8, 9, 12, 16, 19, 21, 24, 28, 31, 33, 36,
			40, 43, 45, 48, 52, 55, 57, 60, 64, 67, 69, 72, 76, 79, 81, 84, 88,
			91, 93, 96 };
	if (test_pfordelta(noException, 32)) {
		printf(
				"\n Test pfordelta SUCCESSFULLY with no exception data [32 size]\n");
	} else {
		printf("\n Test pfordelta FAILED with no exception data [32 size]\n");
	}

	printf("\n 2nd Test Begins \n ");

	uint32_t exception3[32] = { 4, 7, 15, 16, 19, 21, 24, 28, 31, 33, 36, 40,
			43, 45, 48, 52, 55, 57, 64, 68, 69, 72, 76, 79, 81, 84, 88, 91, 93,
			96, 109, 112 };

	if (test_pfordelta(exception3, 32)) {
		printf(
				"\n Test pfordelta SUCCESSFULLY with 3 exception data [32 size]\n");
	} else {
		printf("\n Test pfordelta FAILED with 3 exception data [32 size]\n");
	}

	printf("\n 3th Test Begins \n ");
	uint32_t forcedException[32] = { 1, 9, 18, 19, 21, 22, 23, 24, 25, 28, 31,
			33, 36, 40, 43, 45, 48, 52, 55, 57, 60, 64, 67, 69, 72, 76, 79, 81,
			84, 88, 391, 393 };

	if (test_pfordelta(forcedException, 32)) {
		printf(
				"\n Test pfordelta SUCCESSFULLY with force exception [32 size]\n");
	} else {
		printf("\n Test pfordelta FAILED with force exception [32 size]\n");
	}

	printf("\n 4th Test Begins \n ");
	uint32_t forcedException2[32] = { 1, 9, 18, 19, 21, 22, 23, 24, 25, 28, 31,
			33, 36, 40, 43, 45, 48, 52, 55, 57, 60, 64, 67, 69, 372, 376, 379,
			381, 384, 388, 391, 393 };
	if (test_pfordelta(forcedException2, 32)) {
		printf(
				"\n Test pfordelta SUCCESSFULLY with force exception2 [32 size]\n");
	} else {
		printf("\n Test pfordelta FAILED with force exception2 [32 size]\n");
	}

	printf("\n  5th Test Begins \n ");
	uint32_t lessThan32[12] = { 1, 9, 18, 19, 21, 22, 23, 24, 25, 28, 31, 33 };
	if (test_pfordelta(lessThan32, 12)) {
		printf("\n Test pfordelta SUCCESSFULLY with lessThan32 [12 size]\n");
	} else {
		printf("\n Test pfordelta FAILED with lessThan32 [12 size]\n");
	}

	printf("\n  6th Test Begins \n ");
	uint32_t one[1] = { 18 };
	if (test_pfordelta(one, 1)) {
		printf("\n Test pfordelta SUCCESSFULLY with one [1 size]\n");
	} else {
		printf("\n Test pfordelta FAILED with one [1 size]\n");
	}

	printf("\n 7th Test Begins \n ");
	uint32_t forcedException3[35] = { 1, 9, 18, 19, 21, 22, 23, 24, 25, 28, 31,
			33, 36, 40, 43, 45, 48, 52, 55, 57, 60, 64, 67, 69, 372, 376, 379,
			381, 384, 388, 391, 393, 395, 400, 405 };
	if (test_pfordelta(forcedException3, 35)) {
		printf(
				"\n Test pfordelta SUCCESSFULLY with force exception3 [35 size]\n");
	} else {
		printf("\n Test pfordelta FAILED with force exception3 [35 size]\n");
	}

	printf("\n 8th Test Begins \n ");
	uint32_t noexcetionWithNonDivisible32[35] = { 4, 8, 9, 12, 16, 19, 21, 24,
			28, 31, 33, 36, 40, 43, 45, 48, 52, 55, 57, 60, 64, 67, 69, 72, 76,
			79, 81, 84, 88, 91, 93, 96, 98, 100, 103 };
	if (test_pfordelta(noexcetionWithNonDivisible32, 35)) {
		printf(
				"\n Test pfordelta SUCCESSFULLY with no excetion With NonDivisible32 [35 size]\n");
	} else {
		printf(
				"\n Test pfordelta FAILED with force no excetion With NonDivisible32 [35 size]\n");
	}

	printf("\n 9th Test Begins \n ");
	uint32_t exception1[13] = { 20004717, 20175886, 20176828, 20176829,
			20177783, 20177784, 20178752, 20347069, 20347998, 20348071,
			20348940, 20349895, 20350864 };

	if (test_pfordelta(exception1, 13)) {
		printf(
				"\n Test pfordelta SUCCESSFULLY for real bin values with  1 exception data [13 size]\n");
	} else {
		printf(
				"\n Test pfordelta FAILED for real bin values with  1 exception data [13 size]\n");
	}
}
