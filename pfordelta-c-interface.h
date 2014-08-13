/*
 * c-interface.h
 *
 *  Created on: Sep 12, 2012
 *      Author: David A. Boyuka II
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef C_INTERFACE_H_
#define C_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PRINT_ERROR_RETURN_FAIL(...) \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, "\n"); \
    return false

#define PRINT_WARNING_RETURN_FAIL(...) \
	fprintf(stdout, __VA_ARGS__); \
	fprintf(stdout, "\n"); \
    return false


/*
 *
 * */
uint32_t decode_rids_to_selbox(
						bool isPGContained /*1: PG space is fully contained in the selection box, 0: intersected*/
						, const char *input
						, uint64_t inputLength
						, uint64_t *srcstart //PG region dimension
						, uint64_t *srccount
						, uint64_t *deststart //region dimension of Selection box
						, uint64_t *destcount
						, int dim
						,void **bmap);



/*
 * PFD
 */
int encode_rids(const uint32_t *input, uint32_t inputCount, char *output, uint64_t *outputLength);

int decode_rids(const char *input, uint64_t inputLength, uint32_t *output, uint32_t *outputCount);

int update_rids(char *input, uint64_t inputLength, int32_t rid_offset);

uint32_t get_sufficient_buffer_capacity(uint32_t bin_length);

/*void point for bmap,translate to bmap_t */
int decode_rids_set_bmap(const char *input, uint64_t inputLength,void **bmap);

/*
 * inspect `b` distribution for query
 */
int print_decode_stat(const char *input, uint64_t inputLength, uint32_t *output, uint32_t *outputCount, void **bmap
		,uint64_t b_stat[], uint64_t exp_stat[]);


/*
 * print out the compressed header
 * purpose of this is to inspect the # of exception and the # of bits used for encoding
 * Since exception will be print, it has to decode the data again to get the exception
 *
 */
int print_encode_stat(const char *input, uint64_t inputLength, uint32_t *output, uint32_t *outputCount,
		uint64_t b_stat[], uint64_t exp_stat[] );

/*
 *
 * collect the consecutive 1s or 0s
 * b_stat & exp_stat are not used for current being.
 */
int print_consecutive_binary_stat(const char *input, uint64_t inputLength,
		uint32_t *output, uint32_t *outputCount, uint64_t b_stat[],
		uint64_t exp_stat[]);

/*
 * Special encoding for b=1, & pfordelta encode for b!=1
 * b= 1 case encoded the as RLE ((0,1) Run Length Encoding)
 */
int hybrid_encode_rids(const uint32_t *input, uint32_t inputCount, char *output, uint64_t *outputLength);

/*
 * Special encoding for b=1, & pfordelta decode for b!=1
 * for the efficient memory usage purpose, we hold data in bitmap, therefore,
 * there is no need to keep decompressed data in memory.
 */
int rle_decode_rids(const char *input, uint64_t inputLength,void **bmap);

/***************FOLLOWING IS EXPANSION + RUN Length PFD METHOD *************************/

/*
 * Now, we have combine the expansion and the run length, which deals the b=1 and b ~= 1 cases, respectively
 * The codes is just two branches from the expansion and run length
 */
int exapnd_runlength_encode_rids(const uint32_t *input, uint32_t inputCount, char *output, uint64_t *outputLength);

int expand_runlength_decode_rids(const char *input, uint64_t inputLength, void **bmap);

/***************END of EXPANSION + RUN Length PFD METHOD *************************/


/***************FOLLOWING IS EXPANSION PFD METHOD *************************/
/*
 * Basic idea is instead of using b bits, we are using b+n bits which eliminates exception value
 * `n` value is wisely computed
 * at this moment, we look at the case b = 9, and expand 9 to 11
 * which will drop around 60% and 25% of rounds without exception for variable (vvel, wvel) and (temp, uvel) respectively
 */
int expand_encode_rids(const uint32_t *input, uint32_t inputCount, char *output, uint64_t *outputLength);

/*
 * signature changed:
 * for memory efficiency it does not need recover data in a uint32_t array format
 * instead, we have bitmap to represent the recovered data
 */
int expand_decode_rids(const char *input, uint64_t inputLength, void **bmap);


/************** FOLLOWING IS SKIPPING METHOD************************** */
/*
 * we consider the 2GB = 2^31 is biggest partition size, it has 2^28 total element
 * each block has 128=2^7 elements, so, it has 2^21 blocks
 */
typedef uint32_t total_elm_t;
typedef uint32_t block_num_t;
typedef uint32_t raw_rid_t; // rid_t
typedef uint32_t block_offset_t;

/*
 * Sufficient buffer allocated for skipping encoding
 */

uint64_t get_sufficient_skipping_buffer_size(uint32_t bin_length);

/*
 * TODO: allocate enough buffer size for output
 * Format of output buffer
 * [total # of input element] | [# of blocks] | [first level index] | [compress block pointer (bytes offset)] | [ every compressed block]
 *  uint32_t                  |  uint32_t     | rid_t == uint32_t   |  uint64_t                               |
 *  every compressed block length is computed in a such way, comp_blk_length[i] = comp_blk_ptr[i+1] - comp_blk_ptr[i] // shift one block
 *  we have extra block pointer here
 */
int encode_skipping (const uint32_t *input, uint32_t inputCount, char *output, uint64_t *outputLength);

/*
 * decode part of blocks and meanwhile setting the bitmap given by a raw RID value
 */
int decode_skipping_partially_set_bmap(const char *input, const uint64_t inputLength, const uint32_t rid, void **bmap );

/*
 * decode entire skipping structure, this is used by smallest RID list
 */
int decode_skipping_fully(const char *input, const uint64_t inputLength, uint32_t *output, uint32_t *outputCount);

/*
 * original decoding method: given by a raw rid value, decompress block
 * return:
 *   2 : rid is not found
 *   1 : rid is found, desired block is decompressed
 *   0 : decompression error
 */
int decode_skipping_partially(const char *input, const uint64_t inputLength, const uint32_t rid, uint32_t *output, uint32_t *outputCount);

/*
 * decode_skipping_partially function is divided into two functions (xx_search_block & xx_decompress_block)
 * in order to allow caller to implement caching
 * the caller will call this xx_search_block function, and will cache this returned block number.
 * the caller will also call xx_decompress_block function, and cache the decompressed block.
 * return: 0 means not found / decompress unsuccessfully
 *         1 means we are good
 */
int decode_skipping_partially_search_block(const char *input, const uint64_t inputLength, const uint32_t rid, int64_t *block_idx );
int decode_skipping_partially_decompress_block(
		const char *input, const uint64_t inputLength, const int64_t block_idx, uint32_t *output, uint32_t *outputCount);

/*
 * customized binary search
 * looking for a position that the key value locates the interval of [position , position + 1]
 */
_Bool binary_search_for_position(const raw_rid_t *set, int64_t low, int64_t high, raw_rid_t key,
		int64_t *key_pos);

/*
 * return pfordelta block size, usual case is 128
 */
uint32_t pfordelta_block_size();




/* Following function is particular for the case in which the data candidate check requires
 * any compressed form to been decoded into RIDs
 * */

//BitRun
int adios_runlength_decode_rid(const char *input, uint64_t inputLength, uint32_t *output, uint32_t *outputCount);

//BitExp
int adios_expand_decode_rid(const char *input, uint64_t inputLength, uint32_t *output, uint32_t *outputCount);

//BitRun-BitExp
int adios_expand_runlength_decode_rid(const char *input, uint64_t inputLength, uint32_t *output, uint32_t *outputCount);


#ifdef __cplusplus
}
#endif

#endif /* C_INTERFACE_H_ */
