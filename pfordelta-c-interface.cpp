/*
 * c-interface.c
 *
 *  Created on: Sep 12, 2012
 *      Author: David A. Boyuka II
 */

#include <iostream>
#include <stdint.h>
#include <cstring>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <assert.h>
#include "pfordelta-c-interface.h"
#include "patchedframeofreference.h"
using namespace std;
using namespace pfor;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t decode_rids_to_selbox(bool isPGContained /*1: PG space is fully contained in the selection box, 0: intersected*/
, const char *input, uint64_t inputLength, uint64_t *srcstart //PG region dimension
		, uint64_t *srccount, uint64_t *deststart //region dimension of Selection box
		, uint64_t *destcount, int dim, void **bitmap) {
	bmap_t ** bmap = (bmap_t **) bitmap;
	uint32_t remaining_length = inputLength;
	uint32_t current_length = 0;

	uint32_t decodedDataElm = 0;

	if (isPGContained == true) {

		while (remaining_length > 0) {
			const char *current_buffer = (input + current_length);

			uint32_t data_size = *(const uint32_t *) current_buffer;

			current_buffer += sizeof(uint32_t);
			current_length += sizeof(uint32_t);
			remaining_length -= sizeof(uint32_t);

			uint32_t remaining_data = data_size;
			// When PG is within the selection box,
			// it does not filter out RID during decoding
			decodedDataElm += remaining_data;

			while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
				uint32_t actual_data_size;
				uint32_t actual_significant_data_size;
				uint32_t actual_buffer_size;

				if (!PatchedFrameOfReference::batch_decode_within_selbox_without_checking(
						current_buffer, remaining_length, actual_data_size,
						actual_significant_data_size, actual_buffer_size,
						 srcstart, srccount, deststart,
						destcount, dim, bmap)) {
					return 0;
				}
				if (actual_data_size != PatchedFrameOfReference::kBatchSize
						|| actual_significant_data_size
								!= PatchedFrameOfReference::kBatchSize) {
					return 0;
				}

				current_buffer += actual_buffer_size;
				current_length += actual_buffer_size;
				remaining_length -= actual_buffer_size;

				remaining_data -= PatchedFrameOfReference::kBatchSize;
			}
			if (remaining_data > 0) {
				uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };

				uint32_t actual_data_size;
				uint32_t actual_significant_data_size;
				uint32_t actual_buffer_size;

				if (!PatchedFrameOfReference::batch_decode_within_selbox_without_checking(
						current_buffer, remaining_length, actual_data_size,
						actual_significant_data_size, actual_buffer_size,
						 srcstart, srccount, deststart,
						destcount, dim, bmap)) {
					return 0;
				}

				if (actual_data_size != PatchedFrameOfReference::kBatchSize
						|| actual_significant_data_size != remaining_data) {
				}

				current_buffer += actual_buffer_size;
				current_length += actual_buffer_size;
				remaining_length -= actual_buffer_size;
				remaining_data -= remaining_data;
			}
		}
		if (remaining_length != 0) {
			return 0;
		}
	} else {
		const char *current_buffer = (input + current_length);

		uint32_t data_size = *(const uint32_t *) current_buffer;

		current_buffer += sizeof(uint32_t);
		current_length += sizeof(uint32_t);
		remaining_length -= sizeof(uint32_t);

		uint32_t remaining_data = data_size;
		//cout << "reached here" << endl;
		uint32_t actual_significant_data_size;
		while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
			uint32_t actual_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::batch_decode_within_selbox_with_checking(
					current_buffer, remaining_length, actual_data_size,
					actual_significant_data_size, actual_buffer_size,  srcstart, srccount, deststart, destcount, dim, bmap)) {
				return 0;
			}

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			remaining_data -= PatchedFrameOfReference::kBatchSize;
			decodedDataElm += actual_significant_data_size;
		}
		if (remaining_data > 0) {
			uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };

			uint32_t actual_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::batch_decode_within_selbox_with_checking(
					current_buffer, remaining_length, actual_data_size,
					actual_significant_data_size, actual_buffer_size, srcstart, srccount, deststart, destcount, dim, bmap)) {
				return 0;
			}

			decodedDataElm += actual_significant_data_size;
			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;
			remaining_data -= remaining_data;
		}
		if (remaining_length != 0) {
			return 0;
		}
	}

	return decodedDataElm;
}

/*
 * expansion + runlength encode
 */

int exapnd_runlength_encode_rids(const uint32_t *input, uint32_t inputCount,
		char *output, uint64_t *outputLength) {
	*((uint32_t *) output) = inputCount;
	*outputLength = sizeof(uint32_t);
	output += sizeof(uint32_t);

	const uint32_t *current_data = input;
	uint32_t remaining_data = inputCount;

	char *current_buffer = output;
	char temp_buffer[PatchedFrameOfReference::kSufficientBufferCapacity];
	uint32_t temp_buffer_size;

	while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
		if (!PatchedFrameOfReference::expand_runlength_encode(current_data,
				PatchedFrameOfReference::kBatchSize,
				PatchedFrameOfReference::kBatchSize, temp_buffer,
				sizeof(temp_buffer), temp_buffer_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, temp_buffer_size);
		*outputLength += temp_buffer_size;

		current_buffer += temp_buffer_size;
		current_data += PatchedFrameOfReference::kBatchSize;
		remaining_data -= PatchedFrameOfReference::kBatchSize;

	}

	if (remaining_data > 0) {
		uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };
		memcpy(temp_data, current_data, remaining_data * sizeof(uint32_t));

		if (!PatchedFrameOfReference::expand_runlength_encode(temp_data,
				PatchedFrameOfReference::kBatchSize, remaining_data,
				temp_buffer, sizeof(temp_buffer), temp_buffer_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, temp_buffer_size);
		*outputLength += temp_buffer_size;

		current_buffer += temp_buffer_size;
		current_data += remaining_data;
		remaining_data -= remaining_data;
	}
	return 1;
}

/*
 *
 * 	THE decoding of expansion + runlength is same as the decoding of runlength
 */
int expand_runlength_decode_rids(const char *input, uint64_t inputLength,
		void **bitmap) {
	rle_decode_rids(input, inputLength, bitmap);
}

/*
 * adaptive encoding: NOT USING NOW
 */
int adaptive_encode_rids(const uint32_t *input, uint32_t inputCount,
		char *output, uint64_t *outputLength) {
	*((uint32_t *) output) = inputCount;
	*outputLength = sizeof(uint32_t);
	output += sizeof(uint32_t);

	const uint32_t *current_data = input;
	uint32_t remaining_data = inputCount;

	char *current_buffer = output;
	char temp_buffer[PatchedFrameOfReference::kSufficientBufferCapacity];
	uint32_t temp_buffer_size;

	while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
		if (!PatchedFrameOfReference::expand_encode(current_data,
				PatchedFrameOfReference::kBatchSize,
				PatchedFrameOfReference::kBatchSize, temp_buffer,
				sizeof(temp_buffer), temp_buffer_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, temp_buffer_size);
		*outputLength += temp_buffer_size;

		current_buffer += temp_buffer_size;
		current_data += PatchedFrameOfReference::kBatchSize;
		remaining_data -= PatchedFrameOfReference::kBatchSize;

	}

	if (remaining_data > 0) {
		uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };
		memcpy(temp_data, current_data, remaining_data * sizeof(uint32_t));

		if (!PatchedFrameOfReference::expand_encode(temp_data,
				PatchedFrameOfReference::kBatchSize, remaining_data,
				temp_buffer, sizeof(temp_buffer), temp_buffer_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, temp_buffer_size);
		*outputLength += temp_buffer_size;

		current_buffer += temp_buffer_size;
		current_data += remaining_data;
		remaining_data -= remaining_data;
	}
	return 1;
}

/*
 * expansion encode
 */

int expand_encode_rids(const uint32_t *input, uint32_t inputCount,
		char *output, uint64_t *outputLength) {
	*((uint32_t *) output) = inputCount;
	*outputLength = sizeof(uint32_t);
	output += sizeof(uint32_t);

	const uint32_t *current_data = input;
	uint32_t remaining_data = inputCount;

	char *current_buffer = output;
	char temp_buffer[PatchedFrameOfReference::kSufficientBufferCapacity];
	uint32_t temp_buffer_size;

	while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
		if (!PatchedFrameOfReference::expand_encode(current_data,
				PatchedFrameOfReference::kBatchSize,
				PatchedFrameOfReference::kBatchSize, temp_buffer,
				sizeof(temp_buffer), temp_buffer_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, temp_buffer_size);
		*outputLength += temp_buffer_size;

		current_buffer += temp_buffer_size;
		current_data += PatchedFrameOfReference::kBatchSize;
		remaining_data -= PatchedFrameOfReference::kBatchSize;

	}

	if (remaining_data > 0) {
		uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };
		memcpy(temp_data, current_data, remaining_data * sizeof(uint32_t));

		if (!PatchedFrameOfReference::expand_encode(temp_data,
				PatchedFrameOfReference::kBatchSize, remaining_data,
				temp_buffer, sizeof(temp_buffer), temp_buffer_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, temp_buffer_size);
		*outputLength += temp_buffer_size;

		current_buffer += temp_buffer_size;
		current_data += remaining_data;
		remaining_data -= remaining_data;
	}
	return 1;
}

int expand_decode_rids(const char *input, uint64_t inputLength, void **bitmap) {

	bmap_t ** bmap = (bmap_t **) bitmap;
	uint32_t remaining_length = inputLength;
	uint32_t current_length = 0;
	//	uint32_t *current_data = output;
	//	*outputCount = 0;
	while (remaining_length > 0) {
		const char *current_buffer = (input + current_length);

		uint32_t data_size = *(const uint32_t *) current_buffer;

		current_buffer += sizeof(uint32_t);
		current_length += sizeof(uint32_t);
		remaining_length -= sizeof(uint32_t);

		uint32_t remaining_data = data_size;
		while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			PatchedFrameOfReference::expand_decode_every_batch(current_buffer,
					remaining_length, actual_data_size,
					actual_significant_data_size, actual_buffer_size, bmap);

			/*	if (!PatchedFrameOfReference::expand_decode_every_batch(current_buffer,
			 remaining_length, actual_data_size,
			 actual_significant_data_size, actual_buffer_size, bmap)) {
			 return PRINT_ERROR_RETURN_FAIL("EPD decode failed \n ");
			 }
			 if (actual_data_size != PatchedFrameOfReference::kBatchSize
			 || actual_significant_data_size
			 != PatchedFrameOfReference::kBatchSize) {
			 return 0;
			 }*/

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			/**outputCount += actual_significant_data_size;
			 current_data += PatchedFrameOfReference::kBatchSize;*/
			remaining_data -= PatchedFrameOfReference::kBatchSize;
		}
		if (remaining_data > 0) {
			uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };

			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			PatchedFrameOfReference::expand_decode_every_batch(current_buffer,
					remaining_length, actual_data_size,
					actual_significant_data_size, actual_buffer_size, bmap);

			/*if (!PatchedFrameOfReference::expand_decode_every_batch(current_buffer,
			 remaining_length,  actual_data_size,
			 actual_significant_data_size, actual_buffer_size, bmap)) {
			 return 0;
			 }

			 if (actual_data_size != PatchedFrameOfReference::kBatchSize
			 || actual_significant_data_size != remaining_data) {
			 }
			 *//*memcpy(current_data, temp_data,
			 actual_significant_data_size * sizeof(uint32_t));*/

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			/*	*outputCount += actual_significant_data_size;
			 current_data += remaining_data;*/
			remaining_data -= remaining_data;
		}
	}
	if (remaining_length != 0) {
		return 0;
	}
	return 1;
}

_Bool binary_search_for_position(const raw_rid_t *set, int64_t low,
		int64_t high, raw_rid_t key, int64_t *key_pos) {

	// key is not in this interval
	if (key < set[low] || key > set[high])
		return false;

	if ((high == low) || (high - low == 1)) { // find this interval
		*key_pos = low;
	} else {
		int64_t middle = (low + high) / 2;
		_Bool lrt = binary_search_for_position(set, low, middle, key, key_pos);
		_Bool rrt = binary_search_for_position(set, middle, high, key, key_pos);
		if (!lrt && !rrt) {
			return false;
		}

	}
	return true;
}

uint32_t pfordelta_block_size() {
	return PatchedFrameOfReference::kBatchSize;
}

int decode_skipping_partially_search_block(const char *input,
		const uint64_t inputLength, const uint32_t rid, int64_t *block_idx) {

	//compressed block offset starts from beginning
	// keep this pointer so that we can directly jump to desired compressed block
	const char * saved_ptr = input;
	// skip total # of element
	input += sizeof(total_elm_t);

	//total # of blocks + 1 = index_size
	block_num_t index_size = *((block_num_t *) input);
	input += sizeof(block_num_t);

	raw_rid_t * itv = (raw_rid_t *) input;

	/*
	 * ONLY FOR debug
	 */
	/*printf("first level index : ");
	 for (block_num_t var = 0; var < index_size; ++var) {
	 printf("%u,", itv[var]);
	 }
	 printf("\n");*/

	int64_t key_pos;
	_Bool found = binary_search_for_position(itv, 0, index_size - 1, rid,
			&key_pos);

	// not found
	if (found == false) {
		//		printf("rid %u is not found in first-level indexing \n", rid);
		return 0;
	}
	assert( key_pos >= 0 && key_pos < index_size -1); // -1 is required
	*block_idx = key_pos;
	return 1;
}

int decode_skipping_partially_decompress_block(const char *input,
		const uint64_t inputLength, const int64_t block_idx, uint32_t *output,
		uint32_t *outputCount) {

	//compressed block offset starts from beginning
	// keep this pointer so that we can directly jump to desired compressed block
	const char * saved_ptr = input;
	// skip total # of element
	input += sizeof(total_elm_t);

	//total # of blocks + 1 = index_size
	block_num_t index_size = *((block_num_t *) input);
	input += sizeof(block_num_t);

	int64_t key_pos = block_idx;
	// key_pos indicates which block should be decompressed
	// skip first-level index
	input += index_size * sizeof(raw_rid_t);
	// beginning of block offset
	block_offset_t * blk_offset = (block_offset_t *) input;
	// the block needs to be decompressed
	block_offset_t ofst_blk = *(blk_offset + key_pos);
	// the length of compressed blk can be calculated by next block offset - current blk offset
	uint32_t compressed_blk_len = *(blk_offset + key_pos + 1) - ofst_blk;
	// jump to compressed block
	const char* blk_compressed_ptr = saved_ptr + ofst_blk;

	uint32_t actual_data_size; //decompressed data number
	uint32_t actual_significant_data_size; // decompressed data number
	uint32_t actual_buffer_size; // compressed size
	// now we can call decompression
	// last block to be decompressed, this is special case because its encoded element number may less than kbatch size
	// normal block which encoded by kbatch size elements
	if (!PatchedFrameOfReference::decode_new(blk_compressed_ptr,
			compressed_blk_len, output, PatchedFrameOfReference::kBatchSize,
			actual_data_size, actual_significant_data_size, actual_buffer_size)) {
		printf("pfordetla decode error \n");
		return 0;
	}

	assert(actual_data_size <= PatchedFrameOfReference::kBatchSize);
	assert(actual_significant_data_size <= actual_data_size );
	*outputCount = actual_significant_data_size;
	return 1;
}

int decode_skipping_partially(const char *input, const uint64_t inputLength,
		const uint32_t rid, uint32_t *output, uint32_t *outputCount) {
	//compressed block offset starts from beginning
	// keep this pointer so that we can directly jump to desired compressed block
	const char * saved_ptr = input;
	// skip total # of element
	input += sizeof(total_elm_t);

	//total # of blocks + 1 = index_size
	block_num_t index_size = *((block_num_t *) input);
	input += sizeof(block_num_t);

	raw_rid_t * itv = (raw_rid_t *) input;

	/*
	 * ONLY FOR debug
	 */
	/*printf("first level index : ");
	 for (block_num_t var = 0; var < index_size; ++var) {
	 printf("%u,", itv[var]);
	 }
	 printf("\n");*/

	int64_t key_pos;
	_Bool found = binary_search_for_position(itv, 0, index_size - 1, rid,
			&key_pos);

	// not found
	if (found == false) {
		//		printf("rid %u is not found in first-level indexing \n", rid);
		return 2;
	}

	assert( key_pos >= 0 && key_pos < index_size -1); // -1 is required

	// find it
	// key_pos indicates which block should be decompressed
	// skip first-level index
	input += index_size * sizeof(raw_rid_t);
	// beginning of block offset
	block_offset_t * blk_offset = (block_offset_t *) input;
	// the block needs to be decompressed
	block_offset_t ofst_blk = *(blk_offset + key_pos);
	// the lenght of compressed blk can be calculated by next block offset - current blk offset
	uint32_t compressed_blk_len = *(blk_offset + key_pos + 1) - ofst_blk;
	// jump to compressed block
	const char* blk_compressed_ptr = saved_ptr + ofst_blk;

	uint32_t actual_data_size; //decompressed data number
	uint32_t actual_significant_data_size; // decompressed data number
	uint32_t actual_buffer_size; // compressed size
	// now we can call decompression
	// last block to be decompressed, this is special case because its encoded element number may less than kbatch size
	// normal block which encoded by kbatch size elements
	if (!PatchedFrameOfReference::decode_new(blk_compressed_ptr,
			compressed_blk_len, output, PatchedFrameOfReference::kBatchSize,
			actual_data_size, actual_significant_data_size, actual_buffer_size)) {
		printf("pfordetla decode error \n");
		return 0;
	}

	assert(actual_data_size <= PatchedFrameOfReference::kBatchSize);
	assert(actual_significant_data_size <= actual_data_size );
	*outputCount = actual_significant_data_size;
	return 1;

	// binary search is carried by outside

}

int decode_skipping_fully(const char *input, const uint64_t inputLength,
		uint32_t *output, uint32_t *outputCount) {
	// total # of element
	uint64_t compressed_len = inputLength;
	uint32_t expect_output_count = *((total_elm_t *) input);
	input += sizeof(total_elm_t);
	compressed_len -= sizeof(total_elm_t);
	//	printf("total elem : %u \n", expect_output_count);

	//total # of blocks
	block_num_t index_size = *((block_num_t *) input);
	input += sizeof(block_num_t);
	compressed_len -= sizeof(block_num_t);
	//	printf("total index size # : %u\n", index_size);

	/*************ONLY FOR DEBUG -- PRINT FIRST-LEVEL INDEX*********************/
	/*const char *tmp = input;
	 for(int i = 0; i < index_size; i ++){
	 printf("first level index %u \n", *((raw_rid_t*)tmp));
	 tmp += sizeof(raw_rid_t);
	 }*/
	/******************END *******************************/

	// since it is fully decompression, we can skip the first-level indexing & block offsets
	// skip first level index
	input += index_size * sizeof(raw_rid_t);
	compressed_len -= index_size * sizeof(raw_rid_t);

	/************ONLY FOR DEBUG -- PRINT BLOCK OFFSETS*********************/
	/*tmp = input;
	 for(int i = 0; i < index_size; i ++){
	 printf("block offset %u \n", *((block_offset_t*)tmp));
	 tmp += sizeof(block_offset_t);
	 }*/
	/*****************END (ONLY FOR DEBUG) *******************************/

	// skip block offsets as well
	input += (index_size) * sizeof(block_offset_t);
	compressed_len -= (index_size) * sizeof(block_offset_t);

	uint32_t remaining_length = compressed_len;
	uint32_t current_length = 0;
	uint32_t *current_data = output;
	*outputCount = 0;
	uint32_t remaining_data = expect_output_count;

	while (remaining_length > 0) {
		const char *current_buffer = (input + current_length);

		while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::decode_new(current_buffer,
					remaining_length, current_data,
					PatchedFrameOfReference::kBatchSize, actual_data_size,
					actual_significant_data_size, actual_buffer_size)) {
				return 0;
			}
			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size
							!= PatchedFrameOfReference::kBatchSize) {
				//cout << "here2" << endl;
				return 0;
			}

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			*outputCount += actual_significant_data_size;
			current_data += PatchedFrameOfReference::kBatchSize;
			remaining_data -= PatchedFrameOfReference::kBatchSize;
		}
		//cout << "reached here too too" << endl;
		if (remaining_data > 0) {
			uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };

			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::decode_new(current_buffer,
					remaining_length, temp_data,
					PatchedFrameOfReference::kBatchSize, actual_data_size,
					actual_significant_data_size, actual_buffer_size)) {
				//cout << "here3" << endl;
				return 0;
			}

			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size != remaining_data) {
			}
			//cout << "before memcpy" << endl;
			memcpy(current_data, temp_data,
					actual_significant_data_size * sizeof(uint32_t));

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			*outputCount += actual_significant_data_size;
			current_data += remaining_data;
			remaining_data -= remaining_data;
		}
		//cout << "current length = " << current_length << ", remaining_length = " << remaining_length << endl;
	}

	assert(expect_output_count == (*outputCount));

	if (remaining_length != 0) {
		//cout << "here4 : remaining_buffer = " << remaining_buffer << endl;
		return 0;
	}
	//cout << "decode function returning" << endl;
	return 1;

}

int encode_skipping(const uint32_t *input, uint32_t inputCount, char *output,
		uint64_t *outputLength) {

	*outputLength = 0; // in terms of bytes
	uint32_t block_size = PatchedFrameOfReference::kBatchSize;

	// total # of element
	uint64_t size_of_total_elem = sizeof(total_elm_t);
	*((total_elm_t *) output) = inputCount;
	*outputLength += size_of_total_elem;
	output += size_of_total_elem; // move forward

	// total # of blocks + 1 = number of values in first-level indexing
	uint32_t total_num_block = inputCount / block_size;
	_Bool remainder = inputCount % block_size > 0;
	total_num_block += (remainder ? 1 : 0);
	uint32_t index_size = total_num_block + 1; // additional 1 is added in order to easily perform binary search
	*((block_num_t *) output) = index_size;
	*outputLength += sizeof(block_num_t);
	//	printf("first level size  #: %u  \n", index_size);
	output += sizeof(block_num_t);
	; // move forward

	// build first level index: consists a number of minimum value of every block,
	// In order to perform binary search easily on this first-level index, I added
	// last element into this first-level index
	// |  block 1    | block 2       | block 3      | block 4 	       |
	// | rid0 ...... | rid128 ...... | rid256 ..... | rid512 ... rid758|
	// first-level index : rid0, rid128, rid256, rid512, and rid758
	//	printf("first level index: ");
	for (int32_t i = 0; i < inputCount; i += block_size) {
		*((raw_rid_t *) output) = input[i];
		//		printf("%u ,", input[i] );
		*outputLength += sizeof(raw_rid_t); // every minimum value is uint32_t type
		output += sizeof(raw_rid_t); // move forward
	}

	// add last element, this is additional value in first-level index
	// ATTENSION: even the last value in first-level index is same as the value before last value
	// it is okay. Our binary search algorithm still can find the right interval
	// The equal last two values occur when the total rid element is multiple of bash size (128) + 1
	*((raw_rid_t *) output) = input[inputCount - 1];
	//	printf("%u ,", input[inputCount -1 ] );
	*outputLength += sizeof(raw_rid_t);
	output += sizeof(raw_rid_t); // move forward

	//	printf("\n");


	char *compressed_block_ptr = output; // mark for the compressed block beginning address
	//skip compressed block pointers, the pointer will be filled after block is compressed
	// extra block pointer, it is because of shifting offset
	// additional 1 block offset is needed because the total compressed size is added to this block offset
	output += sizeof(block_offset_t) * (total_num_block + 1);
	*outputLength += sizeof(block_offset_t) * (total_num_block + 1);
	*((block_offset_t *) compressed_block_ptr) = *outputLength; // we know the beginning address (bytes offset) of first block
	//	printf("first block offset %u \n",*((block_offset_t *) compressed_block_ptr) );

	compressed_block_ptr += sizeof(block_offset_t); // this pointer always points to next block


	const uint32_t *current_data = input;
	uint32_t remaining_data = inputCount;

	char *current_buffer = output;
	char temp_buffer[PatchedFrameOfReference::kSufficientBufferCapacity];
	uint32_t block_compressed_size;

	while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
		/*if (!PatchedFrameOfReference::hybrid_encode(current_data,
		 PatchedFrameOfReference::kBatchSize,
		 PatchedFrameOfReference::kBatchSize, temp_buffer,
		 sizeof(temp_buffer), block_compressed_size)) {
		 return 0;
		 }*/

		/*only compress it by a normal p4d encoding*/
		if (!PatchedFrameOfReference::encode(current_data,
				PatchedFrameOfReference::kBatchSize,
				PatchedFrameOfReference::kBatchSize, temp_buffer,
				sizeof(temp_buffer), block_compressed_size)) {
			return 0;
		}

		memcpy(current_buffer, temp_buffer, block_compressed_size);
		*outputLength += block_compressed_size;

		current_buffer += block_compressed_size;
		current_data += PatchedFrameOfReference::kBatchSize;
		remaining_data -= PatchedFrameOfReference::kBatchSize;

		// update block offset
		*((block_offset_t *) compressed_block_ptr) = *outputLength;
		//		printf("block offset %u \n", *((block_offset_t *) compressed_block_ptr));
		compressed_block_ptr += sizeof(block_offset_t);
	}

	if (remaining_data > 0) {

		uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };
		memcpy(temp_data, current_data, remaining_data * sizeof(uint32_t));

		/*if (!PatchedFrameOfReference::hybrid_encode(temp_data,
		 PatchedFrameOfReference::kBatchSize, remaining_data,
		 temp_buffer, sizeof(temp_buffer), block_compressed_size)) {
		 return 0;
		 }*/

		if (!PatchedFrameOfReference::encode(temp_data,
				PatchedFrameOfReference::kBatchSize, remaining_data,
				temp_buffer, sizeof(temp_buffer), block_compressed_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, block_compressed_size);
		*outputLength += block_compressed_size;

		current_buffer += block_compressed_size;
		current_data += remaining_data;
		remaining_data -= remaining_data;

		// update block offset
		*((block_offset_t *) compressed_block_ptr) = *outputLength;
		//		printf("block offset %u \n", *((block_offset_t *) compressed_block_ptr));
		compressed_block_ptr += sizeof(block_offset_t);
	}

	return 1;
}

int rle_decode_rids(const char *input, uint64_t inputLength, void **bitmap) {

	bmap_t ** bmap = (bmap_t **) bitmap;
	uint32_t remaining_length = inputLength;
	uint32_t current_length = 0;
	//	uint32_t *current_data = output;
	//	*outputCount = 0;
	while (remaining_length > 0) {
		const char *current_buffer = (input + current_length);
		uint32_t data_size = *(const uint32_t *) current_buffer;

		current_buffer += sizeof(uint32_t);
		current_length += sizeof(uint32_t);
		remaining_length -= sizeof(uint32_t);

		uint32_t remaining_data = data_size;
		while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::rle_decode_every_batch(
					current_buffer, remaining_length, actual_data_size,
					actual_significant_data_size, actual_buffer_size, bmap)) {
				return PRINT_ERROR_RETURN_FAIL("RLE decode failed \n ")
				;
			}
			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size
							!= PatchedFrameOfReference::kBatchSize) {
				return PRINT_ERROR_RETURN_FAIL("RLE decode size not match")
				;
			}

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			//			*outputCount += actual_significant_data_size;
			//			current_data += PatchedFrameOfReference::kBatchSize;
			remaining_data -= PatchedFrameOfReference::kBatchSize;
		}
		if (remaining_data > 0) {

			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::rle_decode_every_batch(
					current_buffer, remaining_length, actual_data_size,
					actual_significant_data_size, actual_buffer_size, bmap)) {
				return 0;
			}

			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size != remaining_data) {
			}

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			//			*outputCount += actual_significant_data_size;
			//			current_data += remaining_data;
			remaining_data -= remaining_data;
		}
	}
	if (remaining_length != 0) {
		return 0;
	}
	return 1;
}

int hybrid_encode_rids(const uint32_t *input, uint32_t inputCount,
		char *output, uint64_t *outputLength) {
	*((uint32_t *) output) = inputCount;
	*outputLength = sizeof(uint32_t);
	output += sizeof(uint32_t);

	const uint32_t *current_data = input;
	uint32_t remaining_data = inputCount;

	char *current_buffer = output;
	char temp_buffer[PatchedFrameOfReference::kSufficientBufferCapacity];
	uint32_t temp_buffer_size;

	while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
		if (!PatchedFrameOfReference::hybrid_encode(current_data,
				PatchedFrameOfReference::kBatchSize,
				PatchedFrameOfReference::kBatchSize, temp_buffer,
				sizeof(temp_buffer), temp_buffer_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, temp_buffer_size);
		*outputLength += temp_buffer_size;

		current_buffer += temp_buffer_size;
		current_data += PatchedFrameOfReference::kBatchSize;
		remaining_data -= PatchedFrameOfReference::kBatchSize;

	}

	if (remaining_data > 0) {
		uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };
		memcpy(temp_data, current_data, remaining_data * sizeof(uint32_t));

		if (!PatchedFrameOfReference::hybrid_encode(temp_data,
				PatchedFrameOfReference::kBatchSize, remaining_data,
				temp_buffer, sizeof(temp_buffer), temp_buffer_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, temp_buffer_size);
		*outputLength += temp_buffer_size;

		current_buffer += temp_buffer_size;
		current_data += remaining_data;
		remaining_data -= remaining_data;
	}
	return 1;
}

int print_consecutive_binary_stat(const char *input, uint64_t inputLength,
		uint32_t *output, uint32_t *outputCount, uint64_t b_stat[],
		uint64_t exp_stat[]) {

	uint32_t remaining_length = inputLength;
	uint32_t current_length = 0;
	uint32_t *current_data = output;
	*outputCount = 0;
	while (remaining_length > 0) {
		const char *current_buffer = (input + current_length);

		uint32_t data_size = *(const uint32_t *) current_buffer;

		current_buffer += sizeof(uint32_t);
		current_length += sizeof(uint32_t);
		remaining_length -= sizeof(uint32_t);

		uint32_t remaining_data = data_size;
		while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::inspect_binary_stat_while_query(
					current_buffer, remaining_length, current_data,
					PatchedFrameOfReference::kBatchSize, actual_data_size,
					actual_significant_data_size, actual_buffer_size)) {
				return 0;
			}
			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size
							!= PatchedFrameOfReference::kBatchSize) {
				return 0;
			}

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			*outputCount += actual_significant_data_size;
			current_data += PatchedFrameOfReference::kBatchSize;
			remaining_data -= PatchedFrameOfReference::kBatchSize;
		}
		if (remaining_data > 0) {
			uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };

			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::inspect_binary_stat_while_query(
					current_buffer, remaining_length, temp_data,
					PatchedFrameOfReference::kBatchSize, actual_data_size,
					actual_significant_data_size, actual_buffer_size)) {
				return 0;
			}

			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size != remaining_data) {
			}
			memcpy(current_data, temp_data,
					actual_significant_data_size * sizeof(uint32_t));

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			*outputCount += actual_significant_data_size;
			current_data += remaining_data;
			remaining_data -= remaining_data;
		}
	}
	if (remaining_length != 0) {
		return 0;
	}

	return 1;
}

int print_decode_stat(const char *input, uint64_t inputLength,
		uint32_t *output, uint32_t *outputCount, void **bitmap,
		uint64_t b_stat[], uint64_t exp_stat[]) {
	bmap_t ** bmap = (bmap_t **) bitmap;
	uint32_t remaining_length = inputLength;
	uint32_t current_length = 0;
	uint32_t *current_data = output;
	*outputCount = 0;
	while (remaining_length > 0) {
		const char *current_buffer = (input + current_length);

		uint32_t data_size = *(const uint32_t *) current_buffer;

		current_buffer += sizeof(uint32_t);
		current_length += sizeof(uint32_t);
		remaining_length -= sizeof(uint32_t);

		uint32_t remaining_data = data_size;
		//cout << "reached here" << endl;
		while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::inspect_decode_every_batch(
					current_buffer, remaining_length, current_data,
					PatchedFrameOfReference::kBatchSize, actual_data_size,
					actual_significant_data_size, actual_buffer_size, bmap,
					b_stat, exp_stat)) {
				return 0;
			}
			//cout << "reached here too" << endl;
			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size
							!= PatchedFrameOfReference::kBatchSize) {
				//cout << "here2" << endl;
				return 0;
			}

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			*outputCount += actual_significant_data_size;
			current_data += PatchedFrameOfReference::kBatchSize;
			remaining_data -= PatchedFrameOfReference::kBatchSize;
		}
		if (remaining_data > 0) {
			uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };

			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::inspect_decode_every_batch(
					current_buffer, remaining_length, temp_data,
					PatchedFrameOfReference::kBatchSize, actual_data_size,
					actual_significant_data_size, actual_buffer_size, bmap,
					b_stat, exp_stat)) {
				return 0;
			}

			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size != remaining_data) {
			}
			memcpy(current_data, temp_data,
					actual_significant_data_size * sizeof(uint32_t));

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			*outputCount += actual_significant_data_size;
			current_data += remaining_data;
			remaining_data -= remaining_data;
		}
	}
	if (remaining_length != 0) {
		return 0;
	}
	return 1;
}

int print_encode_stat(const char *input, uint64_t inputLength,
		uint32_t *output, uint32_t *outputCount, uint64_t b_stat[],
		uint64_t exp_stat[]) {
	memset(PatchedFrameOfReference::B_TIMES, 0, sizeof(uint64_t) * 33);
	memset(PatchedFrameOfReference::EXP_SIZE, 0, sizeof(uint64_t) * 129);

	uint32_t remaining_length = inputLength;
	uint32_t current_length = 0;
	uint32_t *current_data = output;
	*outputCount = 0;
	while (remaining_length > 0) {
		const char *current_buffer = (input + current_length);

		/*if (remaining_buffer < sizeof(uint32_t)) {
		 return 0;
		 }*/

		uint32_t data_size = *(const uint32_t *) current_buffer;

		current_buffer += sizeof(uint32_t);
		current_length += sizeof(uint32_t);
		remaining_length -= sizeof(uint32_t);

		uint32_t remaining_data = data_size;
		//cout << "reached here" << endl;
		while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::inspect_encode_stat(current_buffer,
					remaining_length, current_data,
					PatchedFrameOfReference::kBatchSize, actual_data_size,
					actual_significant_data_size, actual_buffer_size)) {
				//cout << "here1" << endl;
				return 0;
			}
			//cout << "reached here too" << endl;
			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size
							!= PatchedFrameOfReference::kBatchSize) {
				//cout << "here2" << endl;
				return 0;
			}

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			*outputCount += actual_significant_data_size;
			current_data += PatchedFrameOfReference::kBatchSize;
			remaining_data -= PatchedFrameOfReference::kBatchSize;
		}
		//cout << "reached here too too" << endl;
		if (remaining_data > 0) {
			uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };

			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::inspect_encode_stat(current_buffer,
					remaining_length, temp_data,
					PatchedFrameOfReference::kBatchSize, actual_data_size,
					actual_significant_data_size, actual_buffer_size)) {
				//cout << "here3" << endl;
				return 0;
			}

			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size != remaining_data) {
			}
			//cout << "before memcpy" << endl;
			memcpy(current_data, temp_data,
					actual_significant_data_size * sizeof(uint32_t));

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			*outputCount += actual_significant_data_size;
			current_data += remaining_data;
			remaining_data -= remaining_data;
		}
		//cout << "current length = " << current_length << ", remaining_length = " << remaining_length << endl;
	}
	if (remaining_length != 0) {
		//cout << "here4 : remaining_buffer = " << remaining_buffer << endl;
		return 0;
	}
	for (int i = 1; i <= 32; i++) {
		b_stat[i] += PatchedFrameOfReference::B_TIMES[i];
	}

	for (int i = 1; i <= 128; i++) {
		exp_stat[i] += PatchedFrameOfReference::EXP_SIZE[i];
	}

	return 1;
}

int encode_rids(const uint32_t *input, uint32_t inputCount, char *output,
		uint64_t *outputLength) {
	*((uint32_t *) output) = inputCount;
	*outputLength = sizeof(uint32_t);
	output += sizeof(uint32_t);

	const uint32_t *current_data = input;
	uint32_t remaining_data = inputCount;

	char *current_buffer = output;
	char temp_buffer[PatchedFrameOfReference::kSufficientBufferCapacity];
	uint32_t temp_buffer_size;

	while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
		if (!PatchedFrameOfReference::encode(current_data,
				PatchedFrameOfReference::kBatchSize,
				PatchedFrameOfReference::kBatchSize, temp_buffer,
				sizeof(temp_buffer), temp_buffer_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, temp_buffer_size);
		*outputLength += temp_buffer_size;

		current_buffer += temp_buffer_size;
		current_data += PatchedFrameOfReference::kBatchSize;
		remaining_data -= PatchedFrameOfReference::kBatchSize;

	}

	if (remaining_data > 0) {
		uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };
		memcpy(temp_data, current_data, remaining_data * sizeof(uint32_t));

		if (!PatchedFrameOfReference::encode(temp_data,
				PatchedFrameOfReference::kBatchSize, remaining_data,
				temp_buffer, sizeof(temp_buffer), temp_buffer_size)) {
			return 0;
		}
		memcpy(current_buffer, temp_buffer, temp_buffer_size);
		*outputLength += temp_buffer_size;

		current_buffer += temp_buffer_size;
		current_data += remaining_data;
		remaining_data -= remaining_data;
	}
	return 1;
}
int update_rids(char *input, uint64_t inputLength, int32_t rid_offset) {
	char *current_buffer = input;
	uint32_t remaining_buffer = inputLength;

	if (remaining_buffer < sizeof(uint32_t)) {
		return 0;
	}
	uint32_t data_size = *(const uint32_t *) current_buffer;

	current_buffer += sizeof(uint32_t);
	remaining_buffer -= sizeof(uint32_t);

	uint32_t remaining_data = data_size;
	while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
		PatchedFrameOfReference::Header header;
		if (!header.read(current_buffer)) {
			//cout << "here 3" << endl;
			LOG_WARNING_RETURN_FAIL("invalid buffer header")
			;
		}
		header.frame_of_reference_ += rid_offset;

		if (!header.write(current_buffer)) {
			LOG_WARNING_RETURN_FAIL("failed to write header")
			;
		}

		current_buffer += header.encoded_size_;
		remaining_buffer -= header.encoded_size_;
		remaining_data -= PatchedFrameOfReference::kBatchSize;
	}
	if (remaining_data > 0) {
		PatchedFrameOfReference::Header header;
		if (!header.read(current_buffer)) {
			LOG_WARNING_RETURN_FAIL("invalid buffer header")
			;
		}
		header.frame_of_reference_ += rid_offset;

		if (!header.write(current_buffer)) {
			LOG_WARNING_RETURN_FAIL("failed to write header")
			;
		}

		current_buffer += header.encoded_size_;
		remaining_buffer -= header.encoded_size_;
		remaining_data -= remaining_data;
	}
	if (remaining_buffer != 0) {
		//cout << "here4 : remaining_buffer = " << remaining_buffer << endl;
		return 0;
	}
	return 1;
}

int decode_rids_set_bmap(const char *input, uint64_t inputLength, void **bitmap) {
	bmap_t ** bmap = (bmap_t **) bitmap;
	uint32_t remaining_length = inputLength;
	uint32_t current_length = 0;
	/*uint32_t *current_data = output;
	 *outputCount = 0;*/
	while (remaining_length > 0) {
		const char *current_buffer = (input + current_length);

		uint32_t data_size = *(const uint32_t *) current_buffer;

		current_buffer += sizeof(uint32_t);
		current_length += sizeof(uint32_t);
		remaining_length -= sizeof(uint32_t);

		uint32_t remaining_data = data_size;
		//cout << "reached here" << endl;
		while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::decode_every_batch(current_buffer,
					remaining_length, actual_data_size,
					actual_significant_data_size, actual_buffer_size, bmap)) {
				return 0;
			}
			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size
							!= PatchedFrameOfReference::kBatchSize) {
				return 0;
			}

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			/*	*outputCount += actual_significant_data_size;
			 current_data += PatchedFrameOfReference::kBatchSize;*/
			remaining_data -= PatchedFrameOfReference::kBatchSize;
		}
		if (remaining_data > 0) {
			uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };

			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			if (!PatchedFrameOfReference::decode_every_batch(current_buffer,
					remaining_length, actual_data_size,
					actual_significant_data_size, actual_buffer_size, bmap)) {
				return 0;
			}

			if (actual_data_size != PatchedFrameOfReference::kBatchSize
					|| actual_significant_data_size != remaining_data) {
			}
			//cout << "before memcpy" << endl;
			/*	memcpy(current_data, temp_data,
			 actual_significant_data_size * sizeof(uint32_t));*/

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			/*	*outputCount += actual_significant_data_size;
			 current_data += remaining_data;*/
			remaining_data -= remaining_data;
		}
	}
	if (remaining_length != 0) {
		return 0;
	}
	return 1;
}
int decode_rids(const char *input, uint64_t inputLength, uint32_t *output,
		uint32_t *outputCount) {

	uint32_t remaining_length = inputLength;
	uint32_t current_length = 0;
	uint32_t *current_data = output;
	*outputCount = 0;
	while (remaining_length > 0) {
		const char *current_buffer = (input + current_length);

		/*if (remaining_buffer < sizeof(uint32_t)) {
		 return 0;
		 }*/

		uint32_t data_size = *(const uint32_t *) current_buffer;

		current_buffer += sizeof(uint32_t);
		current_length += sizeof(uint32_t);
		remaining_length -= sizeof(uint32_t);

		uint32_t remaining_data = data_size;
		//cout << "reached here" << endl;
		while (remaining_data >= PatchedFrameOfReference::kBatchSize) {
			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			PatchedFrameOfReference::decode_new(current_buffer,
					remaining_length, current_data,
					PatchedFrameOfReference::kBatchSize, actual_data_size,
					actual_significant_data_size, actual_buffer_size);
			/*	if (!PatchedFrameOfReference::decode_new(current_buffer,
			 remaining_length, current_data,
			 PatchedFrameOfReference::kBatchSize, actual_data_size,
			 actual_significant_data_size, actual_buffer_size)) {
			 return 0;
			 }
			 if (actual_data_size != PatchedFrameOfReference::kBatchSize
			 || actual_significant_data_size
			 != PatchedFrameOfReference::kBatchSize) {
			 return 0;
			 }*/

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			*outputCount += actual_significant_data_size;
			current_data += PatchedFrameOfReference::kBatchSize;
			remaining_data -= PatchedFrameOfReference::kBatchSize;
		}
		//cout << "reached here too too" << endl;
		if (remaining_data > 0) {
			uint32_t temp_data[PatchedFrameOfReference::kBatchSize] = { 0 };

			uint32_t actual_data_size;
			uint32_t actual_significant_data_size;
			uint32_t actual_buffer_size;

			PatchedFrameOfReference::decode_new(current_buffer,
					remaining_length, temp_data,
					PatchedFrameOfReference::kBatchSize, actual_data_size,
					actual_significant_data_size, actual_buffer_size);
			/*
			 if (!PatchedFrameOfReference::decode_new(current_buffer,
			 remaining_length, temp_data,
			 PatchedFrameOfReference::kBatchSize, actual_data_size,
			 actual_significant_data_size, actual_buffer_size)) {
			 //cout << "here3" << endl;
			 return 0;
			 }

			 if (actual_data_size != PatchedFrameOfReference::kBatchSize
			 || actual_significant_data_size != remaining_data) {
			 }
			 */
			//cout << "before memcpy" << endl;
			memcpy(current_data, temp_data,
					actual_significant_data_size * sizeof(uint32_t));

			current_buffer += actual_buffer_size;
			current_length += actual_buffer_size;
			remaining_length -= actual_buffer_size;

			*outputCount += actual_significant_data_size;
			current_data += remaining_data;
			remaining_data -= remaining_data;
		}
	}

	return 1;
}

uint32_t get_sufficient_buffer_capacity(uint32_t bin_length) {
	//return 0;
	return PatchedFrameOfReference::get_sufficient_buffer_capacity(bin_length);
}

uint64_t get_sufficient_skipping_buffer_size(uint32_t bin_length) {
	uint32_t num_block = bin_length / PatchedFrameOfReference::kBatchSize;
	num_block += (bin_length % PatchedFrameOfReference::kBatchSize > 0 ? 1 : 0);

	return sizeof(total_elm_t) // total element field
			+ sizeof(block_num_t) // total block number
			+ sizeof(raw_rid_t) * (num_block + 1) // first-level index
			+ sizeof(block_offset_t) * (num_block + 1) // block offsets
			+ num_block * (sizeof(uint32_t) // block data size
					+ sizeof(uint64_t) // header size of one P4D block
					+ PatchedFrameOfReference::kBatchSize * sizeof(uint32_t) // maximum compressed data size
					);
	//	PatchedFrameOfReference::kBatchSize * sizeof(uint32_t);
}

#ifdef __cplusplus
}
#endif
