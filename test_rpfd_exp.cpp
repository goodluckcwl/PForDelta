/*
 * test_exception_vs_nonexception.cpp
 *
 *  Created on: Jul 16, 2013
 *      Author: xczou
 */

#include <iostream>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include "patchedframeofreference.h"
#include <stdio.h>
#include "pfordelta-c-interface.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <cstdlib>
#include <cstdio>
#include <time.h>
#include <assert.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/poisson_distribution.hpp>

double dclock(void) {
	struct timeval tv;
	gettimeofday(&tv, 0);

	return (double) tv.tv_sec + (double) tv.tv_usec * 1e-6;
}

//#include "gen_fixedlengthcode.cpp"

using namespace pfor;
using namespace std;

void test(int argc, char **argv) {

	if (argc < 3) {
		printf(" ./rpfd_exp [exception_num] [reptition] \n");
		return;
	}
	uint32_t kBatchSize = 128;
	int argi = 1;
	uint32_t exp_total = kBatchSize / 2 - 1;
	uint32_t rep_max = 1;
	sscanf(argv[argi++], "%u", &exp_total);
	sscanf(argv[argi++], "%u", &rep_max);

	printf("exception_num[%u], and reptition[%u] \n",  exp_total, rep_max);

	uint32_t b =1 ;
	uint32_t exception_num;
	uint32_t frame_of_reference_ = 100;

	PatchedFrameOfReference::ExceptionType exception_type_ = PatchedFrameOfReference::EXCEPTION_UNSIGNED_INT;

	uint32_t deltas[kBatchSize];
	uint32_t recovered[kBatchSize];

	char * buffer;
	uint32_t buffer_size = sizeof(uint64_t) + 4 * (kBatchSize + kBatchSize / 2);
	uint32_t data_size, sig_data_size, buf_size;
	uint32_t dummy_exp_val = 10;

	bmap_t *bmap = (bmap_t * ) malloc(sizeof(bmap_t) * 10);

		uint32_t max_val = 1; // b = 1 , max_val =1
		double t_s, last = 0;

		for (exception_num = 0; exception_num < exp_total; exception_num++) {

			buffer = (char *) malloc(rep_max * buffer_size);

			for (uint32_t r = 0; r < rep_max; r++) {

				for (int i = 0; i < kBatchSize; i++) {
						deltas[i] = 1; // dummy value , all value are ones
				}

				 if (exception_num == 1) {
					// a random position
					void *p;
					double ctime=dclock();
					p =  &ctime;
					uint32_t * t = (uint32_t *) p;
					srand(t[0]);
					uint32_t rpos = (uint32_t) (rand() % kBatchSize);
					if (rpos == 0) {
						rpos = 1; // first elment does not count as exception
					}
					deltas[rpos] = dummy_exp_val;
//					printf("pos:[%u],", rpos);
				} else {
					// distribute exception evenly among the delta value
					vector<uint32_t> data;
					for (uint32_t k = 0; k < exception_num; k++) {
						boost::random::uniform_int_distribution<> dist(1,
								kBatchSize - 1);
						boost::random::mt19937 gen;
//						gen.seed(time(NULL) + r + k + exception_num + b);
						// time() has second as resolution
						void *p;
						double ctime=dclock();
						p=  &ctime;
						uint32_t * t = (uint32_t *) p;
						gen.seed(t[0]);
						int next = dist(gen);
						// if duplicated
						while (next < kBatchSize && deltas[next] == 0) {
							next++;
						}
						if (next == kBatchSize) {
							int pre = 1;
							while (pre < kBatchSize && deltas[pre] == 0) {
								pre++;
							}
							if (pre < kBatchSize) {
								deltas[pre] = dummy_exp_val;
//								printf("%u,", pre);
							} else {
								printf("duplicated exception ! BUG \n ");
							}
						} else {
							deltas[next] = dummy_exp_val;
//							printf("%u,", next);

						}

					}

				}

//				printf("exception_num=%u\n", exception_num);

				char *tmp_buf;
				tmp_buf = buffer + (r * buffer_size);

				uint32_t encoded_size;
				PatchedFrameOfReference::encode_on_b_equal_one(deltas, kBatchSize, kBatchSize,
								b, frame_of_reference_, exception_type_, (char *) tmp_buf,
								buffer_size, encoded_size);
				t_s = dclock();
				PatchedFrameOfReference::rle_decode_every_batch((const void *) tmp_buf,
						buffer_size, data_size,
						sig_data_size, buf_size, &bmap);
				last += (dclock() - t_s);
			}

			printf("exception_num=%u,time=%lf\n", exception_num, last);
			free(buffer);
			//			printf(" time=%lf\n\n",last / chunk_rep);
		}

}

int main(int argc, char **argv) {
	test(argc, argv);
}
