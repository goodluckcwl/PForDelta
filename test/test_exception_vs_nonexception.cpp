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

	if (argc < 4) {
		printf(" ./expperf [b] [exception_num] [reptition] \n");
		return;
	}
	uint32_t kBatchSize = 128;
	int bmax = 31;

	int argi = 1;
	uint32_t exp_total = kBatchSize / 2 - 1;
	uint32_t rep_max = 1;
	sscanf(argv[argi++], "%d", &bmax);
	sscanf(argv[argi++], "%u", &exp_total);
	sscanf(argv[argi++], "%u", &rep_max);

	printf("experimenting b[%d], exception_num[%u], and reptition[%u] \n",
			bmax, exp_total, rep_max);

	PatchedFrameOfReference::Header header;
	uint32_t b;
	uint32_t exception_num;
	uint32_t exception_offset; // > kBatchSize => no exception at all
	header.frame_of_reference_ = 100;
	header.significant_data_size_ = kBatchSize;

	header.exception_type_ = PatchedFrameOfReference::EXCEPTION_UNSIGNED_INT;

	uint32_t deltas[kBatchSize];
	uint32_t recovered[kBatchSize];

	char * buffer;
	uint32_t buffer_size = sizeof(uint64_t) + 4 * (kBatchSize + kBatchSize / 2);
	uint32_t data_size, sig_data_size, buf_size;

	for (b = 1; b <= bmax; b++) {
		uint32_t max_val = (1 << b) - 1;
		//		cout << "b=" << b;
		double t_s, last = 0;

		for (exception_num = 0; exception_num < exp_total; exception_num++) {

			buffer = (char *) malloc(rep_max * buffer_size);

			for (uint32_t r = 0; r < rep_max; r++) {

				if (b == 1) {
					for (int i = 0; i < kBatchSize; i++) {
						deltas[i] = 1; // dummy value
					}
				} else {
					for (int i = 0; i < kBatchSize; i++) {
						srand(time(NULL));
						deltas[i] = rand() % max_val + 1; // dummy value
						assert(deltas[i] <= max_val);
						assert(deltas[i] > 0);
					}
				}

				if (exception_num == 0) {
					exception_offset = kBatchSize;
				} else if (exception_num == 1) {
					// a random position
//					srand(time(NULL) + r + exception_num + b);
					void *p;
					double ctime=dclock();
					p=  &ctime;
						uint32_t * t = (uint32_t *) p;
					srand(t[0]);
					uint32_t rpos = (uint32_t) (rand() % kBatchSize);
					if (rpos == 0) {
						rpos = 1; // first elment does not count as exception
					}
					deltas[rpos] = 0;
					exception_offset = rpos; // make the while for loop from beginning
//					printf("pos:[%u],", exception_offset);
				} else {
					// distribute exception evenly among the delta value
					exception_offset = kBatchSize - 1;

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
								deltas[pre] = 0;
								next = pre;
								if (next < exception_offset) {
									exception_offset = next;
								}
							} else {
								printf("duplicated exception ! BUG \n ");
								exception_offset = 1;
							}
						} else {
							deltas[next] = 0;
//							printf("%u,", next);
							if (next < exception_offset) {
								exception_offset = next;
							}
						}

					}
//					printf("offset %u, ", exception_offset);

				}
				header.first_exception_ = exception_offset;
				header.fixed_length_ = b;

//				printf("exception_num=%u,", exception_num);
				header.encoded_size_
						= PatchedFrameOfReference::required_buffer_size(b,
								exception_num, header.exception_type_);

				char *tmp_buf;
				tmp_buf = buffer + (r * buffer_size);
				PatchedFrameOfReference::fixed_length_encode(deltas,
						kBatchSize, header.fixed_length_,
						(char *) tmp_buf + sizeof(uint64_t),
						PFOR_FIXED_LENGTH_BUFFER_SIZE(b));
				header.write(tmp_buf);
				t_s = dclock();
				PatchedFrameOfReference::profile_exp((const void *) tmp_buf,
						buffer_size, recovered, kBatchSize, data_size,
						sig_data_size, buf_size);
				last += (dclock() - t_s);
//				sleep(1);
			}

			printf("b=%u,exception_num=%u,time=%lf\n", b, exception_num, last);
			free(buffer);
			//			printf(" time=%lf\n\n",last / chunk_rep);
		}

	}

}

int main(int argc, char **argv) {
	test(argc, argv);
}
