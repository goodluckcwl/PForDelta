/*
Test code
Author : Saurabh V. Pendse
*/
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
//#include <boost/random/mersenne_twister.hpp>
//#include <boost/random/uniform_int_distribution.hpp>
//#include <boost/random/poisson_distribution.hpp>
#include "patchedframeofreference.h"
#include "pfordelta.h"
extern "C" {
#include "timer.h"
}

//used for test1() and test2()
#define DATA_SIZE 23

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

#define MEM_ALLOC_PER_INT 1000
//macro to get the size in megabytes
#define SIZE_IN_MB(x) (1.0 * x / (1024 * 1024))

//using namespace boost;
using namespace std;
using namespace pfor;


void test0() {
  uint32_t *input = (uint32_t *)malloc(sizeof(uint32_t)  * DATA_SIZE);
  /*for (uint32_t index = 0; index < DATA_SIZE; ++index) {
      input[index] = (index * index);
  }*/
  input[0] = 1;
  input[1] = 3;
  input[2] = 6;
  input[3] = 10;
  input[4] = 15;
  
  string output;
  PatchedFrameOfReference::encode(input, DATA_SIZE, output);
  cout << "output size = " << output.length() << endl;
  vector<uint32_t> decoded_data;
  PatchedFrameOfReference::decode(output.c_str(), output.length(), decoded_data);
  /*for (uint32_t index = 0 ; index < 5; ++index) {
	cout << decoded_data[index] << " ";
  }*/
  cout << endl;
  assert(memcmp(input, &decoded_data[0], DATA_SIZE * sizeof(uint32_t)) == 0);
}

/*void test1() {
	cout << "Test1 start\n\n";
	vector<uint32_t> data, decoded_data;
	for (uint32_t index = 0; index < DATA_SIZE; ++index) {
                uint32_t temp = (uint32_t)(500.0 * rand() / RAND_MAX);
		data.push_back((uint32_t)temp);
	}
        sort(data.begin(), data.end());
	data.erase(unique(data.begin(), data.end()), data.end());

	uint32_t actual_data_size = data.size();
	string buffer;
	PatchedFrameOfReference::encode(&data[0], actual_data_size, buffer);
	PatchedFrameOfReference::decode(buffer.c_str(), buffer.length(), decoded_data);
	assert(memcmp(&data[0], &decoded_data[0], actual_data_size * sizeof(uint32_t)) == 0);
	cout << "Method A" << endl << endl;
	cout << "Actual data size = " << actual_data_size * sizeof(uint32_t) << endl;
	cout << "Compressed data size = " << buffer.length() << endl;
	cout << "Compression ratio = " << (actual_data_size * sizeof(uint32_t)) / (1.0 * buffer.length()) << endl;

	struct compressed_data cd;
	struct decompressed_data dcd;
	pfordelta_compress(&data[0], actual_data_size, &cd);
	pfordelta_decompress(&cd, &dcd);

	cout << "\n\nMethod B" << endl << endl;
	cout << "Actual data size = " << actual_data_size * sizeof(uint32_t) << endl;
	cout << "Compressed data size = " << get_compressed_size(cd) << endl;
	cout << "Compression ratio = " << (actual_data_size * sizeof(uint32_t)) / (1.0 * get_compressed_size(cd))<< endl;

	cout << "\n\nTest1 end\n\n";
}*/

void test_saurabh(char * bin_file) {

	cout << "\n\ntest_saurabh start\n\n";
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
	string encoded_output;
	vector<string> encoded_outputs;

	// Dont use !foef(f) to determine the test5// This function not work properly, it will read last value twice
	while ((read_size = fread(&curval, sizeof(uint32_t), 1, f)) != 0) {
		if (curval < lastval) { // bin value boundary
//			printf("\n bin %u: \n", num_per_bin);
//			print_data(data_per_bin, num_per_bin);
				// put compressed bits into file
			cout << "Compressing bin no. " << num_bins << " : " << num_per_bin;

			//cout << num_per_bin << endl;
			uncompressed_size += (num_per_bin * sizeof(uint32_t));

			//compress
			timer_start(timerName[0]);
			PatchedFrameOfReference::encode(data_per_bin,
					   num_per_bin,
				    	   encoded_output);
			timer_stop(timerName[0]);

			cout << "encoded output size = " << encoded_output.length() << endl;
			encoded_outputs.push_back(encoded_output);

			vector<uint32_t> decoded_data;
			PatchedFrameOfReference::decode(encoded_output.c_str(),
							encoded_output.length(),
							decoded_data);
			cout << ", verified compression" << endl;

			compressed_size += encoded_output.length();

			bin_lengths[num_bins] = num_per_bin;
			
			compress_times[num_bins] = timer_get_total_interval(timerName[0]);
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
	PatchedFrameOfReference::encode(data_per_bin,
			   num_per_bin,
		    	   encoded_output);
	timer_stop(timerName[0]);
	
	vector<uint32_t> decoded_data;
	/*PatchedFrameOfReference::decode(encoded_output.c_str(),
					encoded_output.length(),
					decoded_data);*/

	//assert(memcmp(&data_per_bin[0], &decoded_data[0], num_per_bin * sizeof(uint32_t)) == 0);

	encoded_outputs.push_back(encoded_output);

	compressed_size += encoded_output.length();

	//cout << "total compressed size = " << compressed_size << endl;
	//cout << "total uncompressed size = " << uncompressed_size << endl;
	bin_lengths[num_bins] = num_per_bin;
	timer_reset(timerName[0]);
	compress_times[num_bins] = timer_get_total_interval(timerName[0]);

	double decompress_times[NUM_BINS] = {0};
	double compress_max_th = 0, compress_min_th = 1000, compress_avg_th = 0;
	double decompress_max_th = 0, decompress_min_th = 1000, decompress_avg_th = 0;
	uint32_t compress_max_length, compress_min_length;
	uint32_t decompress_max_length, decompress_min_length;
	double throughput;

	for (uint32_t index = 0; index < num_bins; ++index) {
		throughput = (1.0 * SIZE_IN_MB(bin_lengths[index] * sizeof(uint32_t)) / compress_times[index]);
		if (compress_max_th < throughput) {
			compress_max_th = throughput;
			compress_max_length = bin_lengths[index];
		}
		if (compress_min_th > throughput) {
			compress_min_th = throughput;
			compress_min_length = bin_lengths[index];
		}
		compress_avg_th += throughput;
	}
	
	cout << "\nCompression" << endl;
	cout << "Maximum throughput = " << compress_max_th << " MB/s, for bin size = " << compress_max_length << " elements" << endl;
	cout << "Avg. throughput = " << (1.0 * compress_avg_th / num_bins) << " MB/s" << endl;
	cout << "Minimum throughput = " << compress_min_th << " MB/s, for bin size = " << compress_min_length << " elements" << endl;
	cout << "Compression ratio = " << 1.0 * uncompressed_size / compressed_size << endl << endl;

	for (uint32_t index = 0; index < num_bins; ++index) {
		cout << "Decompressing bin : " << index 
		     << ", bin length : " << bin_lengths[index] << endl;
		vector<uint32_t> decoded_data;
		timer_start(timerName[1]);
		PatchedFrameOfReference::decode(encoded_outputs[index].c_str(), encoded_outputs[index].length(), decoded_data);
		timer_stop(timerName[1]);
		decompress_times[index] = timer_get_total_interval(timerName[1]);
		timer_reset(timerName[1]);

		throughput = (1.0 * SIZE_IN_MB(bin_lengths[index] * sizeof(uint32_t)) / decompress_times[index]);
		cout << throughput << endl;
		if (decompress_max_th < throughput) {
			decompress_max_th = throughput;
		}
		if (decompress_min_th > throughput) {
			decompress_min_th = throughput;
		}
		decompress_avg_th += throughput;
	}

	cout << "\n\nDecompression" << endl;
	cout << "Maximum throughput = " << decompress_max_th << " MB/s" << endl;
	cout << "Avg. throughput = " << (1.0 * decompress_avg_th / num_bins) << " MB/s" << endl;
	cout << "Minimum throughput = " << decompress_min_th << " MB/s" << endl;

	fclose(f);
	free_memory(data_per_bin);
}

void test_chris(char * bin_file) {

	cout << "\n\ntest_chris start\n\n";
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
			pfordelta_compress(data_per_bin, num_per_bin, &cd);
			timer_stop(timerName[0]);
			encoded_outputs.push_back(cd);

			//verify compression
			struct decompressed_data dcd;
			pfordelta_decompress(&cd, &dcd);

			assert(num_per_bin == dcd.unpacked_nums);

			//assert(memcmp(data_per_bin, dcd.unpacked, num_per_bin * sizeof(uint32_t)) == 0);
			assert(memcmp(&data_per_bin[0], &dcd.unpacked[0], num_per_bin * sizeof(uint32_t)) == 0);
			cout << ", verified compression" << endl;

			compressed_size += get_compressed_size(cd);

			bin_lengths[num_bins] = num_per_bin;
			
			compress_times[num_bins] = timer_get_total_interval(timerName[0]);
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
	pfordelta_compress(data_per_bin, num_per_bin, &cd);
	timer_stop(timerName[0]);
	
	struct decompressed_data dcd;
	pfordelta_decompress(&cd, &dcd);
	
	assert(memcmp(&data_per_bin[0], dcd.unpacked, num_per_bin * sizeof(uint32_t)) == 0);

	encoded_outputs.push_back(cd);

	compressed_size += get_compressed_size(cd);

	//cout << "total compressed size = " << compressed_size << endl;
	//cout << "total uncompressed size = " << uncompressed_size << endl;
	bin_lengths[num_bins] = num_per_bin;
	timer_reset(timerName[0]);
	compress_times[num_bins] = timer_get_total_interval(timerName[0]);

	double decompress_times[NUM_BINS] = {0};
	double compress_max_th = 0, compress_min_th = 1000, compress_avg_th = 0;
	double decompress_max_th = 0, decompress_min_th = 1000, decompress_avg_th = 0;
	uint32_t compress_max_length, compress_min_length;
	uint32_t decompress_max_length, decompress_min_length;
	double throughput;

	for (uint32_t index = 0; index < num_bins; ++index) {
		throughput = (1.0 * SIZE_IN_MB(bin_lengths[index] * sizeof(uint32_t)) / compress_times[index]);
		if (compress_max_th < throughput) {
			compress_max_th = throughput;
			compress_max_length = bin_lengths[index];
		}
		if (compress_min_th > throughput) {
			compress_min_th = throughput;
			compress_min_length = bin_lengths[index];
		}
		compress_avg_th += throughput;
	}
	
	cout << "\nCompression" << endl;
	cout << "Maximum throughput = " << compress_max_th << " MB/s, for bin size = " << compress_max_length << " elements" << endl;
	cout << "Avg. throughput = " << (1.0 * compress_avg_th / num_bins) << " MB/s" << endl;
	cout << "Minimum throughput = " << compress_min_th << " MB/s, for bin size = " << compress_min_length << " elements" << endl;
	cout << "Compression ratio = " << 1.0 * uncompressed_size / compressed_size << endl << endl;

	for (uint32_t index = 0; index < num_bins; ++index) {
		cout << "Decompressing bin : " << index 
		     << ", bin length : " << bin_lengths[index] << endl;
		struct decompressed_data dcd;
		timer_start(timerName[1]);
		pfordelta_decompress(&encoded_outputs[index], &dcd); 
		timer_stop(timerName[1]);
		decompress_times[index] = timer_get_total_interval(timerName[1]);
		timer_reset(timerName[1]);

		throughput = (1.0 * SIZE_IN_MB(bin_lengths[index] * sizeof(uint32_t)) / decompress_times[index]);
		if (decompress_max_th < throughput) {
			decompress_max_th = throughput;
		}
		if (decompress_min_th > throughput) {
			decompress_min_th = throughput;
		}
		decompress_avg_th += throughput;
	}

	cout << "\n\nDecompression" << endl;
	cout << "Maximum throughput = " << decompress_max_th << " MB/s" << endl;
	cout << "Avg. throughput = " << (1.0 * decompress_avg_th / num_bins) << " MB/s" << endl;
	cout << "Minimum throughput = " << decompress_min_th << " MB/s" << endl;

	fclose(f);
	free_memory(data_per_bin);
}

void profile_query_index(vector< vector<uint32_t> > query_index) {
	cout << "\nBin Lengths\n";	
	for (uint32_t index = 0; index < query_index.size(); ++index) {
		cout << query_index[index].size() << " ";
	}
	cout << endl;
	timer_init();
	char timerName [][256] = {"encode", "decode"};
	uint32_t uncompressed_size = 0, compressed_size = 0;
	double compress_times[NUM_BINS] = {0};
	string encoded_output;
	vector<string> encoded_outputs;
	for (uint32_t index = 0; index < query_index.size(); ++index) {
		uncompressed_size += (query_index[index].size() * sizeof(uint32_t));
		cout << "Compressing bin " << index;
		timer_start(timerName[0]);
		PatchedFrameOfReference::encode(&query_index[index][0],
						query_index[index].size(),
					    	encoded_output);
		timer_stop(timerName[0]);
		encoded_outputs.push_back(encoded_output);

		vector<uint32_t> decoded_data;
		PatchedFrameOfReference::decode(encoded_output.c_str(),
						encoded_output.length(),
						decoded_data);

    uint32_t wrong;
		wrong = memcmp(&query_index[index][0], &decoded_data[0], query_index[index].size() * sizeof(uint32_t));
    if (wrong) {
        cout << endl;
        for (uint32_t idx = 0; idx < query_index[index].size(); ++idx) {
            cout << query_index[index][idx] << " : " << decoded_data[idx] << endl;
        }
        return;
    }
    assert(wrong == 0);
		cout << ", verified compression" << endl;

		compressed_size += encoded_output.length();		
		compress_times[index] = timer_get_total_interval(timerName[0]);
		timer_reset(timerName[0]);
	}

	double decompress_times[NUM_BINS] = {0};
	double compress_max_th = 0, compress_min_th = 1000, compress_avg_th = 0;
	double decompress_max_th = 0, decompress_min_th = 1000, decompress_avg_th = 0;
	uint32_t compress_max_length, compress_min_length;
	uint32_t decompress_max_length, decompress_min_length;
	double throughput;

	uint32_t total_bin_length = 0;
	for (uint32_t index = 0; index < query_index.size(); ++index) {
		total_bin_length += query_index[index].size();
	}
	for (uint32_t index = 0; index < query_index.size(); ++index) {
		throughput = (1.0 * SIZE_IN_MB(query_index[index].size() * sizeof(uint32_t)) / compress_times[index]);
		if (compress_max_th < throughput) {
			compress_max_th = throughput;
			compress_max_length = query_index[index].size();
		}
		if (compress_min_th > throughput) {
			compress_min_th = throughput;
			compress_min_length = query_index[index].size();
		}
		compress_avg_th += (1.0 * query_index[index].size() / total_bin_length) * throughput;
	}
	
	cout << "\nCompression" << endl;
	cout << "Maximum compression throughput = " << compress_max_th << " MB/s, for bin size = " << compress_max_length << " elements" << endl;
	cout << "Avg. compression throughput = " << compress_avg_th << " MB/s" << endl;
	cout << "Minimum compression throughput = " << compress_min_th << " MB/s, for bin size = " << compress_min_length << " elements" << endl;
	cout << "Compression ratio = " << 1.0 * uncompressed_size / compressed_size << endl << endl;

	for (uint32_t index = 0; index < query_index.size(); ++index) {
		cout << "Decompressing bin : " << index << endl;
		vector<uint32_t> decoded_data;
		timer_start(timerName[1]);
		PatchedFrameOfReference::decode(encoded_outputs[index].c_str(), encoded_outputs[index].length(), decoded_data);
		timer_stop(timerName[1]);
		decompress_times[index] = timer_get_total_interval(timerName[1]);
		timer_reset(timerName[1]);

		throughput = (1.0 * SIZE_IN_MB(query_index[index].size() * sizeof(uint32_t)) / decompress_times[index]);
		if (decompress_max_th < throughput) {
			decompress_max_th = throughput;
		} 
		if (decompress_min_th > throughput) {
			decompress_min_th = throughput;
		}
		decompress_avg_th += (1.0 * query_index[index].size() / total_bin_length) * throughput;
	}

	cout << "\n\nDecompression" << endl;
	cout << "Maximum decompression throughput = " << decompress_max_th << " MB/s" << endl;
	cout << "Avg. decompression throughput = " << decompress_avg_th << " MB/s" << endl;
	cout << "Minimum decompression throughput = " << decompress_min_th << " MB/s" << endl;

}




void profile_query_index_chris(vector< vector<uint32_t> > query_index) {
	cout << "\nBin Lengths\n";
	for (uint32_t index = 0; index < query_index.size(); ++index) {
		cout << query_index[index].size() << " ";
	}
	cout << endl;
	timer_init();
	char timerName [][256] = {"encode", "decode"};
	uint32_t uncompressed_size = 0, compressed_size = 0;
	double compress_times[NUM_BINS] = {0};
	string encoded_output;
	vector<compressed_data> encoded_outputs;
	struct compressed_data cd;
	for (uint32_t index = 0; index < query_index.size(); ++index) {
		uncompressed_size += (query_index[index].size() * sizeof(uint32_t));
		cout << "Compressing bin " << index;
		timer_start(timerName[0]);

		/*PatchedFrameOfReference::encode(&query_index[index][0],
						query_index[index].size(),
					    	encoded_output);*/
		pfordelta_compress(&query_index[index][0], query_index[index].size(), &cd);
		timer_stop(timerName[0]);

		encoded_outputs.push_back(cd);

		//verify compression
		struct decompressed_data dcd;
		pfordelta_decompress(&cd, &dcd);

//		encoded_outputs.push_back(encoded_output);

		/*vector<uint32_t> decoded_data;
		PatchedFrameOfReference::decode(encoded_output.c_str(),
						encoded_output.length(),
						decoded_data);*/

		assert(memcmp(&query_index[index][0], &dcd.unpacked[0], query_index[index].size() * sizeof(uint32_t)) == 0);
		cout << ", verified compression" << endl;

		compressed_size += get_compressed_size(cd);
		compress_times[index] = timer_get_total_interval(timerName[0]);
		timer_reset(timerName[0]);
	}

	double decompress_times[NUM_BINS] = {0};
	double compress_max_th = 0, compress_min_th = 1000, compress_avg_th = 0;
	double decompress_max_th = 0, decompress_min_th = 1000, decompress_avg_th = 0;
	uint32_t compress_max_length, compress_min_length;
	uint32_t decompress_max_length, decompress_min_length;
	double throughput;

	uint32_t total_bin_length = 0;
	for (uint32_t index = 0; index < query_index.size(); ++index) {
		total_bin_length += query_index[index].size();
	}	

	for (uint32_t index = 0; index < query_index.size(); ++index) {
		throughput = (1.0 * SIZE_IN_MB(query_index[index].size() * sizeof(uint32_t)) / compress_times[index]);
		if (compress_max_th < throughput) {
			compress_max_th = throughput;
			compress_max_length = query_index[index].size();
		}
		if (compress_min_th > throughput) {
			compress_min_th = throughput;
			compress_min_length = query_index[index].size();
		}
		compress_avg_th += (1.0 * query_index[index].size() / total_bin_length) * throughput;
	}

	cout << "\nCompression" << endl;
	cout << "Maximum compression throughput = " << compress_max_th << " MB/s, for bin size = " << compress_max_length << " elements" << endl;
	cout << "Avg. compression throughput = " << compress_avg_th << " MB/s" << endl;
	cout << "Minimum compression throughput = " << compress_min_th << " MB/s, for bin size = " << compress_min_length << " elements" << endl;
	cout << "Compression ratio = " << 1.0 * uncompressed_size / compressed_size << endl << endl;

	for (uint32_t index = 0; index < query_index.size(); ++index) {
		cout << "Decompressing bin : " << index << endl;
		struct decompressed_data dcd;
		timer_start(timerName[1]);
		pfordelta_decompress(&encoded_outputs[index], &dcd);
		timer_stop(timerName[1]);
		decompress_times[index] = timer_get_total_interval(timerName[1]);
		timer_reset(timerName[1]);

		throughput = (1.0 * SIZE_IN_MB(query_index[index].size() * sizeof(uint32_t)) / decompress_times[index]);
		if (decompress_max_th < throughput) {
			decompress_max_th = throughput;
		}
		if (decompress_min_th > throughput) {
			decompress_min_th = throughput;
		}
		decompress_avg_th += (1.0 * query_index[index].size() / total_bin_length) * throughput;
	}

	cout << "\n\nDecompression" << endl;
	cout << "Maximum decompression throughput = " << decompress_max_th << " MB/s" << endl;
	cout << "Avg. decompression throughput = " << decompress_avg_th << " MB/s" << endl;
	cout << "Minimum decompression throughput = " << decompress_min_th << " MB/s" << endl;

}



void test_local_blocks_chris(char *root_path, uint32_t block_index) {
	char current_file[256], offset_file[256];
	vector<uint32_t> current_file_offsets;

	sprintf(offset_file, "%s/block.%d-offset.dat", root_path, block_index);
	sprintf(current_file, "%s/block.%d-query_index.dat", root_path, block_index);

	FILE *offset_fp = fopen(offset_file, "r");
	uint32_t value;
	while (! feof(offset_fp)) {
		fscanf(offset_fp, " %d", &value);
		current_file_offsets.push_back(value);
	}
	fclose(offset_fp);

	current_file_offsets.erase(current_file_offsets.begin());
	FILE *fp = fopen(current_file, "rb");
	uint32_t curval;
	uint32_t counter = 0;

	vector< vector<uint32_t> > query_index;
	vector<uint32_t> current_bin;

	uint32_t total_elements = 0;
	while (fread(&curval, sizeof(uint32_t), 1, fp)) {
//	while (counter < 200) {
//		fread(&curval, sizeof(uint32_t), 1, fp);
		if (counter < current_file_offsets[0]) {
			current_bin.push_back(curval);
		} else {
			current_file_offsets.erase(current_file_offsets.begin());
			total_elements += current_bin.size();
			query_index.push_back(current_bin);
			current_bin.clear();

			//compress current bin
			current_bin.push_back(curval);
		}
		counter++;
	}
	query_index.push_back(current_bin);

	profile_query_index_chris(query_index);
	/*	if (counter > current_file_offsets[0]) {
			query_index.push_back(current_bin);
			current_bin.clear();
			//cout << "\nstarting new bin\n";
			current_file_offsets.erase(current_file_offsets.begin());
			current_bin.push_back(curval);
			//cout << curval < " ";
		} else {
			current_bin.push_back(curval);
			//cout << curval << " ";
		}
		counter++;
	}*/
	/*cout << "Number of bins = " << query_index.size() << endl;
	for (uint32_t index = 0; index < query_index.size(); ++index) {
		vector<uint32_t> temp = query_index[index];
		cout << "Bin " << index << endl;
		for (uint32_t index2 = 0; index2 < temp.size(); ++index2) {
			cout << temp[index2] << " ";
		}
		cout << endl;
	}*/
	fclose(fp);
}




void test_local_blocks(char *root_path, uint32_t block_index) {
	char current_file[256], offset_file[256];
	vector<uint32_t> current_file_offsets;

	sprintf(offset_file, "%s/block.%d-offset.dat", root_path, block_index);
	sprintf(current_file, "%s/block.%d-query_index.dat", root_path, block_index);

	FILE *offset_fp = fopen(offset_file, "r");
	uint32_t value;
	while (! feof(offset_fp)) {
		fscanf(offset_fp, " %d", &value);
		current_file_offsets.push_back(value);
	}
	fclose(offset_fp);
	
	current_file_offsets.erase(current_file_offsets.begin());
	FILE *fp = fopen(current_file, "rb");
	uint32_t curval;
	uint32_t counter = 0;

	vector< vector<uint32_t> > query_index;
	vector<uint32_t> current_bin;

	uint32_t total_elements = 0;
	while (fread(&curval, sizeof(uint32_t), 1, fp)) {
//	while (counter < 200) {
//		fread(&curval, sizeof(uint32_t), 1, fp);
		if (counter < current_file_offsets[0]) {
			current_bin.push_back(curval);
		} else {
			current_file_offsets.erase(current_file_offsets.begin());
			total_elements += current_bin.size();
			query_index.push_back(current_bin);
			current_bin.clear();
			
			//compress current bin
			current_bin.push_back(curval);
		}
		counter++;
	}
	query_index.push_back(current_bin);

	profile_query_index(query_index);
	/*	if (counter > current_file_offsets[0]) {
			query_index.push_back(current_bin);
			current_bin.clear();
			//cout << "\nstarting new bin\n";
			current_file_offsets.erase(current_file_offsets.begin());
			current_bin.push_back(curval);
			//cout << curval < " ";
		} else {
			current_bin.push_back(curval);
			//cout << curval << " ";
		}
		counter++;
	}*/
	/*cout << "Number of bins = " << query_index.size() << endl;
	for (uint32_t index = 0; index < query_index.size(); ++index) {
		vector<uint32_t> temp = query_index[index];
		cout << "Bin " << index << endl;
		for (uint32_t index2 = 0; index2 < temp.size(); ++index2) {
			cout << temp[index2] << " ";
		}
		cout << endl;
	}*/
	fclose(fp);
}
int main(int argc, char **argv) {
	//test0();
	/*if ( argc == 4){
		if (strcmp(argv[1],"chris") == 0){
			test_local_blocks_chris(argv[2], atoi(argv[3]));
		}else if (strcmp(argv[1], "saurabh") == 0){
			test_local_blocks(argv[2], atoi(argv[3]));
		}
	}else{
		cout << "input : chirs/saurabh [path to index file dir] [# of block]" << endl;
	}*/
//	test_local_blocks_chris(argv[1], atoi(argv[2]));
	test_saurabh(argv[1]);
	//test_chris(argv[1]);
	//test1();
	/*test1();
	test2();*/
	/*if (argc < 7)  {
            cout << "Usage : ./unittest <poisson mean> <small index fraction> <num bins> <max bin size> <uniform int low> <uniform int high> " << endl;
	} else {
            test3(atoi(argv[1]), atof(argv[2]), atoi(argv[3]), atoi(argv[4]),
        	      atoi(argv[5]), atoi(argv[6]));
	}*/
}
