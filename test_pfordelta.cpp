/*
Test code
Author : Saurabh V. Pendse
*/
#include <iostream>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/poisson_distribution.hpp>
#include "patchedframeofreference.h"
#include "pfordelta.h"
#include "common.h"
extern "C" {
#include "timer.h"
}

//used for test1() and test2()
#define DATA_SIZE 1000000

//test3()
//number of bins
#define NUM_BINS 1000
//maximum bin size
#define MAX_BIN_SIZE 10000
//mean of the poisson distribution from which the bin lengths are sampled
#define POISSON_MEAN 100
//fraction of the bins whose lengths are sampled from the poisson distribution.
//The remaining bins will have length = MAX_MIN_SIZE
#define SMALL_INDEX_FRACTION 0.90
//The low and high bounds of the uniform integer distribution used to generate the deltas in each bin
#define UNIFORM_INT_LOW 1000
#define UNIFORM_INT_HIGH 20000

#define MEM_ALLOC_PER_INT 100
//macro to get the size in megabytes
#define SIZE_IN_MB(x) (1.0 * x / (1024 * 1024))

using namespace boost;
using namespace std;
using namespace pfor;

void test1() {
	cout << "Test1 start\n\n";
	uint32_t data[DATA_SIZE] = {0};
	for (uint32_t index = 0; index < DATA_SIZE; ++index) {
		data[index] = (uint32_t)(500.0 * rand()/RAND_MAX);
	}
	string buffer;
	cout << "Actual data size = " << DATA_SIZE * sizeof(uint32_t) << endl;
	PatchedFrameOfReference::encode(data, DATA_SIZE, buffer);
	cout << "Compressed data size = " << buffer.length() << endl;
	cout << "Compression ratio = " << (DATA_SIZE * sizeof(uint32_t)) / (1.0 * buffer.length())<< endl;
	cout << "\n\nTest1 end\n\n";
}

void test2() {
	cout << "\n\nTest 2 start\n\n";
	timer_init();
	char timerName [][256] = {"encode", "decode"};
	uint32_t *data = (uint32_t *)malloc(sizeof(uint32_t) * DATA_SIZE);
	vector<uint32_t> decoded_data;	
	for (uint32_t index = 0; index < DATA_SIZE; ++index) {
		data[index] = (uint32_t)(1000.0 * rand()/RAND_MAX);
	}
	std::string buffer;
	cout << "Input data size = " << SIZE_IN_MB(DATA_SIZE * sizeof(uint32_t)) << " MB" << endl;

	timer_start(timerName[0]);
	PatchedFrameOfReference::encode(data, DATA_SIZE, buffer);
	timer_stop(timerName[0]);

	cout << "Compressed data size = " << SIZE_IN_MB(buffer.length()) << " MB" << endl;
	cout << "Compression ratio = " << (DATA_SIZE * sizeof(uint32_t)) / (1.0 * buffer.length()) << endl;
	cout << "Compression time = " << timer_get_total_interval(timerName[0]) << endl;
	cout << "Compression throughput = " 
	     << SIZE_IN_MB(DATA_SIZE * sizeof(uint32_t)) / timer_get_total_interval(timerName[0]) 
             << endl << endl;

	timer_start(timerName[1]);
	PatchedFrameOfReference::decode(buffer.c_str(), buffer.length(), decoded_data);
	timer_stop(timerName[1]);
	cout << "Decompression time = " << timer_get_total_interval(timerName[1]) << endl;
	cout << "Decompression throughput = " 
	     << SIZE_IN_MB(buffer.length()) / timer_get_total_interval(timerName[1]) << endl;

	assert(memcmp(data, &decoded_data[0], DATA_SIZE) == 0);
	timer_finalize();
	cout << "\n\nTest 2 end\n\n";
}



void test3(uint32_t poisson_mean, double small_index_fraction, uint32_t num_bins,
           uint32_t max_bin_size, uint32_t uniform_int_low, uint32_t uniform_int_high) {
	//cout << "\n\nTest 3 start\n\n";
	timer_init();
	char timerName [][256] = {"encode", "decode"};
	boost::random::mt19937 gen;
	boost::poisson_distribution<> distpd(poisson_mean);
	gen.seed(time(NULL));
	uint32_t uncompressed_size = 0, compressed_size = 0;
	vector < vector<uint32_t> > inverted_indexes_vect;
	uint32_t *index_lengths = (uint32_t *)malloc(sizeof(uint32_t) * num_bins);
	for (uint32_t index = 0; index < (uint32_t)(small_index_fraction * num_bins); ++index) {
		index_lengths[index] = distpd(gen);
		uncompressed_size += index_lengths[index];
	}
	for (uint32_t index = (uint32_t) (small_index_fraction * num_bins); index < num_bins; ++index) {
		index_lengths[index] = max_bin_size;
		uncompressed_size += index_lengths[index];
	}
	//cout << "\nBin lengths initialized\n";
	uncompressed_size *= sizeof(uint32_t);
	boost::random::uniform_int_distribution<> dist(uniform_int_low, uniform_int_high);
	gen.seed(time(NULL));
	dist.reset();

	for (uint32_t index = 0; index < num_bins; ++index) {
		vector<uint32_t> bin_data;
		for (uint32_t element = 0; element < index_lengths[index]; ++element) {
			uint32_t rowid = dist(gen);
			bin_data.push_back(rowid);
		}
		inverted_indexes_vect.push_back(bin_data);
	}

	string compressed_index;
	//vector<string> encoded_outputs;
	vector<compressed_data> encoded_outputs;

	double compress_times[num_bins], decompress_times[num_bins];

	double compress_max_th = 0, compress_avg_th = 0, compress_min_th = 1000;
	uint32_t compress_max_length, compress_min_length;

	double decompress_max_th = 0, decompress_avg_th = 0, decompress_min_th = 1000;
	uint32_t decompress_max_length, decompress_min_length;

	//cout << "Before compress" << endl;
	for (uint32_t index = 0; index < num_bins; ++index) {
		string encoded_output;
		timer_start(timerName[0]);

	
		struct compressed_data cd;
		pfordelta_compress(&inverted_indexes_vect[index][0],
				   inverted_indexes_vect[index].size(),
			    	   &cd);

		encoded_outputs.push_back(cd);

	/*	PatchedFrameOfReference::encode(&inverted_indexes_vect[index][0],
						inverted_indexes_vect[index].size(),
						encoded_output);*/
		timer_stop(timerName[0]);
		compress_times[index] = timer_get_current_interval(timerName[0]);
		uint32_t temp =  ceil(1.0 * cd.min_bits_per_item * cd.unpacked_nums / 8) + (cd.exception_nums * sizeof(uint32_t));
		compressed_size += temp;

		double throughput = SIZE_IN_MB(inverted_indexes_vect[index].size() * sizeof(uint32_t)) / compress_times[index];

		if (throughput > compress_max_th) {
			compress_max_th = throughput;
			compress_max_length = inverted_indexes_vect[index].size();
		} else if (throughput < compress_min_th) {
			compress_min_th = throughput;
			compress_min_length = inverted_indexes_vect[index].size();
		}
		compress_avg_th += throughput;
		timer_reset(timerName[0]);
	}
	/*cout << "Compression" << endl;
	cout << "Maximum throughput = " << compress_max_th << " MB/s, for bin size = " << compress_max_length << " elements" << endl;
	cout << "Avg. throughput = " << (1.0 * compress_avg_th / num_bins) << " MB/s" << endl;
	cout << "Minimum throughput = " << compress_min_th << " MB/s, for bin size = " << compress_min_length << " elements" << endl;
	cout << "Compression ratio = " << 1.0 * uncompressed_size / compressed_size << endl;
	*/
	for (uint32_t index = 0; index < num_bins; ++index) {
		vector<uint32_t> decoded_data;

		//const char *current = encoded_outputs[index].c_str();

		struct compressed_data cd = encoded_outputs[index];

		uint32_t length = ceil(1.0 * cd.min_bits_per_item * cd.unpacked_nums / 8) + (cd.exception_nums * sizeof(uint32_t));
		//uint32_t length = encoded_outputs[index].length();

		struct decompressed_data dcd;
		timer_start(timerName[1]);
		//PatchedFrameOfReference::decode(current, length, decoded_data);
		pfordelta_decompress(&cd, &dcd);
		timer_stop(timerName[1]);

		decompress_times[index] = timer_get_current_interval(timerName[1]);
		double throughput = SIZE_IN_MB(inverted_indexes_vect[index].size() * sizeof(uint32_t)) / decompress_times[index];

		if (throughput > decompress_max_th) {
			decompress_max_th = throughput;
			decompress_max_length = inverted_indexes_vect[index].size();
		} else if (throughput < decompress_min_th) {
			decompress_min_th = throughput;
			decompress_min_length = inverted_indexes_vect[index].size();
		}
		decompress_avg_th += throughput;
		timer_reset(timerName[1]);
	}
	/*cout << endl << "Decompression" << endl;
	cout << "Maximum throughput = " << decompress_max_th << " MB/s, for bin size = " << decompress_max_length << " elements" << endl;
	cout << "Avg. throughput = " << 1.0 * decompress_avg_th / num_bins << endl;
	cout << "Minimum throughput = " << decompress_min_th << " MB/s, for bin size = " << decompress_min_length << " elements" << endl;
	*/
	free(index_lengths);
	cout << poisson_mean << " " << compress_max_th << " " << 1.0 * compress_avg_th / num_bins << " " << compress_min_th << " "
             << 1.0 * uncompressed_size / compressed_size << " "
             << decompress_max_th << " " << 1.0 * decompress_avg_th / num_bins << " " << decompress_min_th << endl;
	//cout << "\n\nTest 3 end\n\n" << endl;
}

void test4() {
	cout << "\n\nTest 4 start\n\n";
	timer_init();
	char timerName [][256] = {"encode", "decode"};
	boost::random::mt19937 gen;
	boost::poisson_distribution<> distpd(POISSON_MEAN);
	gen.seed(time(NULL));

	uint32_t uncompressed_size = 0, compressed_size = 0;
	vector < vector<uint32_t> > inverted_indexes_vect;
	uint32_t index_lengths[NUM_BINS] = {0};
	for (uint32_t index = 0; index < (uint32_t)(SMALL_INDEX_FRACTION * NUM_BINS); ++index) {
		index_lengths[index] = distpd(gen);
		uncompressed_size += index_lengths[index];
	}
	for (uint32_t index = (uint32_t) (SMALL_INDEX_FRACTION * NUM_BINS); index < NUM_BINS; ++index) {
		index_lengths[index] = MAX_BIN_SIZE;
		uncompressed_size += index_lengths[index];
	}
	uncompressed_size *= sizeof(uint32_t);
	boost::random::uniform_int_distribution<> dist(UNIFORM_INT_LOW, UNIFORM_INT_HIGH);
	gen.seed(time(NULL));
	dist.reset();

	for (uint32_t index = 0; index < NUM_BINS; ++index) {
		vector<uint32_t> bin_data;
		for (uint32_t element = 0; element < index_lengths[index]; ++element) {
			uint32_t rowid = dist(gen);
			bin_data.push_back(rowid);
		}
		inverted_indexes_vect.push_back(bin_data);
	}

	string compressed_index;
	vector<string> encoded_outputs;
	double compress_times[NUM_BINS], decompress_times[NUM_BINS];

	double compress_max_th = 0, compress_avg_th = 0, compress_min_th = 1000;
	uint32_t compress_max_length, compress_min_length;

	double decompress_max_th = 0, decompress_avg_th = 0, decompress_min_th = 1000;
	uint32_t decompress_max_length, decompress_min_length;

	for (uint32_t index = 0; index < NUM_BINS; ++index) {
		string encoded_output;
		timer_start(timerName[0]);
		PatchedFrameOfReference::encode(&inverted_indexes_vect[index][0],
						inverted_indexes_vect[index].size(),
						encoded_output);
		timer_stop(timerName[0]);
		compress_times[index] = timer_get_current_interval(timerName[0]);
		compressed_size += encoded_output.length();
		encoded_outputs.push_back(encoded_output);
		double throughput = SIZE_IN_MB(inverted_indexes_vect[index].size() * sizeof(uint32_t)) / compress_times[index];

		if (throughput > compress_max_th) {
			compress_max_th = throughput;
			compress_max_length = inverted_indexes_vect[index].size();
		} else if (throughput < compress_min_th) {
			compress_min_th = throughput;
			compress_min_length = inverted_indexes_vect[index].size();
		}
		compress_avg_th += throughput;
		timer_reset(timerName[0]);
	}
	cout << "Compression" << endl;
	cout << "Maximum throughput = " << compress_max_th << " MB/s, for bin size = " << compress_max_length << " elements" << endl;
	cout << "Avg. throughput = " << (1.0 * compress_avg_th / NUM_BINS) << " MB/s" << endl;
	cout << "Minimum throughput = " << compress_min_th << " MB/s, for bin size = " << compress_min_length << " elements" << endl;
	cout << "Compression ratio = " << 1.0 * uncompressed_size / compressed_size << endl;

	for (uint32_t index = 0; index < NUM_BINS; ++index) {
		vector<uint32_t> decoded_data;
		const char *current = encoded_outputs[index].c_str();
		uint32_t length = encoded_outputs[index].length();

		timer_start(timerName[1]);
		PatchedFrameOfReference::decode(current, length, decoded_data);
		timer_stop(timerName[1]);

		decompress_times[index] = timer_get_current_interval(timerName[1]);
		double throughput = SIZE_IN_MB(inverted_indexes_vect[index].size() * sizeof(uint32_t)) / decompress_times[index];

		if (throughput > decompress_max_th) {
			decompress_max_th = throughput;
			decompress_max_length = inverted_indexes_vect[index].size();
		} else if (throughput < decompress_min_th) {
			decompress_min_th = throughput;
			decompress_min_length = inverted_indexes_vect[index].size();
		}
		decompress_avg_th += throughput;
		timer_reset(timerName[1]);
	}
	cout << endl << "Decompression" << endl;
	cout << "Maximum throughput = " << decompress_max_th << " MB/s, for bin size = " << decompress_max_length << " elements" << endl;
	cout << "Avg. throughput = " << 1.0 * decompress_avg_th / NUM_BINS << endl;
	cout << "Minimum throughput = " << decompress_min_th << " MB/s, for bin size = " << decompress_min_length << " elements" << endl;

	cout << "\n\nTest 4 end\n\n" << endl;
}

void test5(char * bin_file) {

	cout << "\n\nTest 5 start\n\n";
	timer_init();
	char timerName [][256] = {"encode", "decode"};
	double compress_times[NUM_BINS] = {0};
	
	FILE *f = fopen(bin_file, "r");
	if (!f) {
		printf("file %s open error! \n", bin_file);
		return;
	}
	uint32_t lastval = 0;
	uint32_t *data_per_bin = (uint32_t *) malloc(
			sizeof(uint32_t) * MEM_ALLOC_PER_INT);
	uint32_t num_per_bin = 0;
	uint32_t bin_lengths[NUM_BINS] = {0};
	uint32_t curval;
	uint32_t read_size = 0;
	uint32_t num_bins = 0;
	// used for compute compressed data size in order to avoid
	// cost of writing compressed data file
	uint32_t compressed_size = 0, uncompressed_size = 0;

	// current allocated memory size, we initially allocate 32 size of memory
	// but that space can not held all the values in one bin, we have to
	// dynamically increase memory allocated size
	uint32_t cur_mem_size = MEM_ALLOC_PER_INT;
	struct compressed_data cd;
	vector<compressed_data> encoded_outputs;

	// Dont use !foef(f) to determine the test5// This function not work properly, it will read last value twice
	FILE *out = fopen("bin_data.dat", "w");
	while ((read_size = fread(&curval, sizeof(uint32_t), 1, f)) != 0) {
		if (curval < lastval) { // bin value boundary
//			printf("\n bin %u: \n", num_per_bin);
//			print_data(data_per_bin, num_per_bin);
				// put compressed bits into file
			cout << "Compressing bin no. " << num_bins << " : " << num_per_bin << endl;
			//cout << num_per_bin << endl;
			uncompressed_size += (num_per_bin * sizeof(uint32_t));

			//compress
			timer_start(timerName[0]);
			pfordelta_compress(data_per_bin,
					   num_per_bin,
				    	   &cd);
			timer_stop(timerName[0]);
			encoded_outputs.push_back(cd);

			decompressed_data dcd;
			pfordelta_decompress(&cd, &dcd);
	
			uint32_t temp =  ceil(1.0 * cd.min_bits_per_item * cd.unpacked_nums / 8) + (cd.exception_nums * sizeof(uint32_t));
			compressed_size += temp;

			bin_lengths[num_bins] = num_per_bin;
			
			compress_times[num_bins] = timer_get_current_interval(timerName[0]);
			timer_reset(timerName[0]);

			// start a new memory allocation
			uint32_t * newp = (uint32_t *)realloc(data_per_bin,
		       					      sizeof(uint32_t) * MEM_ALLOC_PER_INT);
			if (newp != NULL) {
				data_per_bin = newp;
				cur_mem_size = MEM_ALLOC_PER_INT;
				num_per_bin = 0;
			} else {
				printf("Can not re-allocate more memory ! ");
				return;
			}
			num_bins++;
		}
		fprintf(out, "%d ",curval);
		data_per_bin[num_per_bin++] = curval;
		if (num_per_bin >= cur_mem_size) { // value in bins exceed the current allocated memory
			cur_mem_size += MEM_ALLOC_PER_INT;
			uint32_t *newp = (uint32_t *)realloc(data_per_bin,
					sizeof(uint32_t) * cur_mem_size);
			if (newp != NULL) {
				data_per_bin = newp;
			} else {
				printf("Can not re-allocate more memory ! ");
				return;
			}
		}
		lastval = curval; // update lastval
	}
//	printf("\n bin %u: \n", num_per_bin);
//	print_data(data_per_bin, num_per_bin);

	//compress
	uncompressed_size += (num_per_bin * sizeof(uint32_t));
	timer_start(timerName[0]);

	pfordelta_compress(data_per_bin,
			   num_per_bin,
		    	   &cd);
	timer_stop(timerName[0]);
	decompressed_data dcd;
	pfordelta_decompress(&cd, &dcd);
	encoded_outputs.push_back(cd);

	uint32_t temp =  ceil(1.0 * cd.min_bits_per_item * cd.unpacked_nums / 8) + (cd.exception_nums * sizeof(uint32_t));
	compressed_size += temp;

	//cout << "total compressed size = " << compressed_size << endl;
	//cout << "total uncompressed size = " << uncompressed_size << endl;
	bin_lengths[num_bins] = num_per_bin;
	timer_reset(timerName[0]);
	compress_times[num_bins] = timer_get_current_interval(timerName[0]);

	double decompress_times[NUM_BINS] = {0};
	double compress_max_th = 0, compress_min_th = 1000, compress_avg_th = 0;
	double decompress_max_th = 0, decompress_min_th = 1000, decompress_avg_th = 0;
	uint32_t compress_max_length, compress_min_length;
	uint32_t decompress_max_length, decompress_min_length;
	double throughput;

	for (uint32_t index = 0; index < num_bins; ++index) {
		cout << compress_times[index] << " ";
		throughput = (1.0 * SIZE_IN_MB(bin_lengths[index] * sizeof(uint32_t)) / compress_times[index]);
		if (compress_max_th < throughput) {
			compress_max_th = throughput;
			compress_max_length = bin_lengths[index];
		} else if (compress_min_th > throughput) {
			compress_min_th = throughput;
			compress_min_length = bin_lengths[index];
		}
		compress_avg_th += throughput;
	}
	cout << endl;
	
	cout << "Compression" << endl;
	cout << "Maximum throughput = " << compress_max_th << " MB/s, for bin size = " << compress_max_length << " elements" << endl;
	cout << "Avg. throughput = " << (1.0 * compress_avg_th / num_bins) << " MB/s" << endl;
	cout << "Minimum throughput = " << compress_min_th << " MB/s, for bin size = " << compress_min_length << " elements" << endl;
	cout << "Compression ratio = " << 1.0 * uncompressed_size / compressed_size << endl;

	/*for (uint32_t index = 0; index < num_bins; ++index) {
		struct decompressed_data dcd;
		struct compressed_data cd = encoded_outputs[index];
		timer_start(timerName[1]);
		cout << "Before decompress" << endl;
		pfordelta_decompress(&cd, &dcd);
		cout << "After decompress" << endl;
		timer_stop(timerName[1]);
		decompress_times[index] = timer_get_current_interval(timerName[1]);
		timer_reset(timerName[1]);

		throughput = (1.0 * SIZE_IN_MB(bin_lengths[index] * sizeof(uint32_t)) / decompress_times[index]);
		if (decompress_max_th < throughput) {
			decompress_max_th = throughput;
		} else if (decompress_min_th > throughput) {
			decompress_min_th = throughput;
		}
		decompress_avg_th += throughput;
	}

	cout << "\n\nDecompression" << endl;
	cout << "Maximum throughput = " << decompress_max_th << " MB/s" << endl;
	cout << "Avg. throughput = " << (1.0 * decompress_avg_th / num_bins) << " MB/s" << endl;
	cout << "Minimum throughput = " << decompress_min_th << " MB/s" << endl;*/

	fclose(out);
	fclose(f);
	free_memory(data_per_bin);
}


int main(int argc, char **argv) {
	test5(argv[1]);
	/*test1();
	test2();*/
	/*if (argc < 7)  {
            cout << "Usage : ./unittest <poisson mean> <small index fraction> <num bins> <max bin size> <uniform int low> <uniform int high> " << endl;
	} else {
            test3(atoi(argv[1]), atof(argv[2]), atoi(argv[3]), atoi(argv[4]),
        	      atoi(argv[5]), atoi(argv[6]));
	}*/
}
