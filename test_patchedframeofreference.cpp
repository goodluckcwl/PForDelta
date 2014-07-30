/*
 Test code
 Author : Saurabh V. Pendse
 */
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/poisson_distribution.hpp>
#include "patchedframeofreference.h"
#include <stdio.h>
#include "pfordelta-c-interface.h"

//extern "C" {
//#include "timer.h"
//}

//used for test1() and test2()
#define DATA_SIZE 135
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
//macro to get the size in megabytes
#define SIZE_IN_MB(x) (1.0 * x / (1024 * 1024))

using namespace boost;
using namespace std;
using namespace pfor;

int compfunc(const void * a, const void * b) {
	return (*(uint32_t*) a - *(uint32_t*) b);
}

void create_lookup(unsigned char set_bit_count[],
		unsigned char set_bit_position[][16]) {
	memset(set_bit_count, 0, 256);
	for (int i = 0; i < 65536; i++) {
		set_bit_count[i] = __builtin_popcount(i); // total bit 1 for value i
		unsigned short int temp = i;
		int counter = set_bit_count[i] - 1;
		for (int j = 15; j >= 0; j--) {
			unsigned int temp1 = temp >> j & 0x0001;
			if (temp1 == 1) {
				set_bit_position[i][counter--] = j;
			}
		}

	}
}

void test1() {
	cout << "Test1 start\n\n";
	vector<uint32_t> data, decoded_data;
	for (uint32_t index = 0; index < DATA_SIZE; ++index) {
		uint32_t temp = (uint32_t) (500.0 * rand() / RAND_MAX) + 1;
		data.push_back((uint32_t) temp);
		//		data.push_back(index);
	}
	sort(data.begin(), data.end());
	data.erase(unique(data.begin(), data.end()), data.end());
	cout << "Test data\n";
	for (uint32_t index = 0; index < data.size(); ++index) {
		cout << data[index] << " ";
	}
	cout << endl;

	string buffer;
	//PatchedFrameOfReference::encode(&data[0], data.size(), buffer);
	PatchedFrameOfReference::decode(buffer.c_str(), buffer.length(),
			decoded_data);

	/*for (uint32_t index = 0; index < data.size(); ++index) {
	 cout << index << " : " << data[index] << " : " << decoded_data[index] << endl;
	 }
	 cout << endl;*/

	assert(memcmp(&data[0], &decoded_data[0], data.size() * sizeof(uint32_t)) == 0);

	cout << "Actual data size = " << data.size() * sizeof(uint32_t) << endl;
	cout << "Compressed data size = " << buffer.length() << endl;
	cout << "Compression ratio = " << (data.size() * sizeof(uint32_t)) / (1.0
			* buffer.length()) << endl;
	cout << "\n\nTest1 end\n\n";
}

/*
 * eliminate the duplicated data
 */
/*void gen_random_sorted_data(vector<uin32_t> &data){
 boost::random::mt19937 gen;
 int delta_ub = 100;
 boost::random::uniform_int_distribution<> dist(1, 100);
 data[0]= (uint32_t) (1000.0 * rand() / RAND_MAX); // generate first element
 for(uint32_t i = 1; i < DATA_SIZE; i ++){
 int delta = dist(gen);
 data[i] = data[i-1] + (uint32_t)delta;
 }
 }*/
/*
void test2() {
	cout << "\n\nTest 2 start\n\n";
	timer_init();
	 char timerName [][256] = {"encode", "decode"};
	uint32_t *data = (uint32_t *)malloc(sizeof(uint32_t) * DATA_SIZE);
	 for (uint32_t index = 0; index < DATA_SIZE; ++index) {
	 data[index] = (uint32_t)(1000.0 * rand()/RAND_MAX);
	 }


	 vector<uint32_t> data;
	 for (uint32_t index = 0; index < DATA_SIZE; ++index) {
	 data.push_back((uint32_t) (1000.0 * rand() / RAND_MAX));
	 }
	 sort(data.begin(), data.end());

	vector<uint32_t> data;

	boost::random::mt19937 gen;
	int delta_ub = 100;
	boost::random::uniform_int_distribution<> dist(1, delta_ub);
	data.push_back((uint32_t) (1000.0 * rand() / RAND_MAX)); // generate first element
	for (uint32_t i = 1; i < DATA_SIZE; i++) {
		int delta = dist(gen);
		data.push_back(data.back() + (uint32_t) delta);
	}
	cout << "data size " << data.size() << endl;
	sort(data.begin(), data.end());

	for (int i = 0; i < DATA_SIZE; i++) {
		cout << data[i] << ",";
	}
	cout << endl;

	//	gen_random_sorted_data(data);

	vector<uint32_t> decoded_data;
	std::string buffer;
	cout << "Input data size = " << SIZE_IN_MB(DATA_SIZE * sizeof(uint32_t))
			<< " MB" << endl;

	//	timer_start(timerName[0]);
	PatchedFrameOfReference::encode(&data[0], DATA_SIZE, buffer);
	//	timer_stop(timerName[0]);

	cout << "Compressed data size = " << SIZE_IN_MB(buffer.length()) << " MB"
			<< endl;
	cout << "Compression ratio = " << (DATA_SIZE * sizeof(uint32_t)) / (1.0
			* buffer.length()) << endl;
	//	cout << "Compression time = " << timer_get_total_interval(timerName[0]) << endl;
	cout << "Compression throughput = "
	 << SIZE_IN_MB(DATA_SIZE * sizeof(uint32_t)) / timer_get_total_interval(timerName[0])
	 << endl << endl;

	//	timer_start(timerName[1]);
	//	uint32_t RID_size = *(const uint32_t *) current_buf; // first 32bit data is total # of RID
	uint32_t bitslot = BITNSLOTS(32768);
	bmap_t * bitmap = (bmap_t*) malloc(bitslot * sizeof(bmap_t));
	memset(bitmap, 0, bitslot * sizeof(bmap_t));
	PatchedFrameOfReference::decode_set_bmap(buffer.c_str(), buffer.length(),
			&bitmap);

	int k = 0;
	uint64_t tmp_t = 0;
	for (; k < bitslot; k++) {
		tmp_t += __builtin_popcountll(bitmap[k]);
	}
	cout << "total bit set " << tmp_t << endl;

	//recover bitmap
	unsigned char set_bit_count[65536];
	unsigned char set_bit_position[65536][16];
	create_lookup(set_bit_count, set_bit_position);

	uint32_t reconstct_rid;
	uint32_t total_recovered = 0;
	vector<uint32_t> recovered_rid;
	k = 0;
	cout << "recover bit map : ";
	for (; k < bitslot; k++) {
		uint64_t offset_long_int = k * 64;
		uint16_t * temp = (uint16_t *) &bitmap[k];
		uint64_t offset;
		for (int j = 0; j < 4; j++) {
			offset = offset_long_int + j * 16;
			for (int m = 0; m < set_bit_count[temp[j]]; m++) {
				total_recovered++;
				reconstct_rid = offset + set_bit_position[temp[j]][m];
				recovered_rid.push_back((uint32_t) reconstct_rid);
				//					cout << reconstct_rid << ",";
			}
		}
	}
	cout << endl;

	cout << "total recovered: " << total_recovered << " sizeof vec : "<< recovered_rid.size()<<endl;

	 cout << "original data: ";
	 for ( int i = 0; i < DATA_SIZE; i ++){
	 cout << data[i]<<",";
	 }
	 cout <<endl;

	 cout << "decodedd data: ";
	 for ( int i = 0; i < DATA_SIZE; i ++){
	 cout << decoded_data[i]<<",";
	 }
	 cout <<endl;

	 cout << "recoverb data: ";
	 for ( int i = 0; i < DATA_SIZE; i ++){
	 cout << recovered_rid[i]<<",";
	 }
	 cout <<endl;

	//	timer_stop(timerName[1]);
	cout << "Decompression time = " << decode_e - decode_s << endl;
	 cout << "Decompression throughput = " << SIZE_IN_MB(buffer.length())
	 / (decode_e - decode_s) << endl;
	//	free(bitmap);
	//	bitmap = NULL;
	//	assert(memcmp(&data[0], &decoded_data[0], DATA_SIZE) == 0);



	 cout << "decode data size " << decoded_data.size() << endl;
	 int notIndecoded = 0;
	 cout << "decoded not in data elem : ";
	 for ( int i = 0; i < DATA_SIZE; i ++){
	 uint32_t *item = (uint32_t*)bsearch(&data[i], &decoded_data[0], DATA_SIZE, sizeof(uint32_t), compfunc);
	 if (item == NULL) {
	 cout << data[i] << ",";
	 notIndecoded ++;
	 }
	 }
	 cout << endl;

	 int notIn = 0;
	 cout << "not in data elem : ";
	 for ( int i = 0; i < DATA_SIZE; i ++){
	 uint32_t *item =(uint32_t*)bsearch(&data[i], &recovered_rid[0],  DATA_SIZE, sizeof(uint32_t), compfunc);
	 if (item == NULL) {
	 cout << data[i] << ",";
	 notIn ++;
	 }

	 }
	 cout << " # of elm not in data " << notIn << endl;


	//	timer_stop(timerName[1]);
	cout << "Decompression time = " << timer_get_total_interval(timerName[1]) << endl;
	 cout << "Decompression throughput = "
	 << SIZE_IN_MB(buffer.length()) / timer_get_total_interval(timerName[1]) << endl;

	//	assert(memcmp(&data[0], &decoded_data[0], DATA_SIZE*sizeof(uint32_t)) == 0);
	assert(memcmp(&data[0], &recovered_rid[0], DATA_SIZE*sizeof(uint32_t)) == 0);
	//	timer_finalize();
	cout << "\n\nTest 2 end\n\n";
}
*/
/*
void test3(uint32_t poisson_mean, double small_index_fraction,
		uint32_t num_bins, uint32_t max_bin_size, uint32_t uniform_int_low,
		uint32_t uniform_int_high) {
	//cout << "\n\nTest 3 start\n\n";
	timer_init();
	 char timerName [][256] = {"encode", "decode"};
	boost::random::mt19937 gen;
	boost::poisson_distribution<> distpd(poisson_mean);
	gen.seed(time(NULL));
	uint32_t uncompressed_size = 0, compressed_size = 0;
	vector<vector<uint32_t> > inverted_indexes_vect;
	uint32_t *index_lengths = (uint32_t *) malloc(sizeof(uint32_t) * num_bins);
	for (uint32_t index = 0; index < (uint32_t) (small_index_fraction
			* num_bins); ++index) {
		index_lengths[index] = distpd(gen);
		uncompressed_size += index_lengths[index];
	}
	for (uint32_t index = (uint32_t) (small_index_fraction * num_bins); index
			< num_bins; ++index) {
		index_lengths[index] = max_bin_size;
		uncompressed_size += index_lengths[index];
	}
	//cout << "\nBin lengths initialized\n";
	uncompressed_size *= sizeof(uint32_t);
	boost::random::uniform_int_distribution<> dist(uniform_int_low,
			uniform_int_high);
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
	vector<string> encoded_outputs;
	double compress_times[num_bins], decompress_times[num_bins];

	double compress_max_th = 0, compress_avg_th = 0, compress_min_th = 1000;
	uint32_t compress_max_length, compress_min_length;

	double decompress_max_th = 0, decompress_avg_th = 0, decompress_min_th =
			1000;
	uint32_t decompress_max_length, decompress_min_length;

	for (uint32_t index = 0; index < num_bins; ++index) {
		string encoded_output;
		//		timer_start(timerName[0]);
		PatchedFrameOfReference::encode(&inverted_indexes_vect[index][0],
				inverted_indexes_vect[index].size(), encoded_output);
		//		timer_stop(timerName[0]);
		//		compress_times[index] = timer_get_current_interval(timerName[0]);
		compressed_size += encoded_output.length();
		encoded_outputs.push_back(encoded_output);
		double
				throughput =
						SIZE_IN_MB(inverted_indexes_vect[index].size() * sizeof(uint32_t))
								/ compress_times[index];

		if (throughput > compress_max_th) {
			compress_max_th = throughput;
			compress_max_length = inverted_indexes_vect[index].size();
		} else if (throughput < compress_min_th) {
			compress_min_th = throughput;
			compress_min_length = inverted_indexes_vect[index].size();
		}
		compress_avg_th += throughput;
		//		timer_reset(timerName[0]);
	}
	cout << "Compression" << endl;
	 cout << "Maximum throughput = " << compress_max_th << " MB/s, for bin size = " << compress_max_length << " elements" << endl;
	 cout << "Avg. throughput = " << (1.0 * compress_avg_th / num_bins) << " MB/s" << endl;
	 cout << "Minimum throughput = " << compress_min_th << " MB/s, for bin size = " << compress_min_length << " elements" << endl;
	 cout << "Compression ratio = " << 1.0 * uncompressed_size / compressed_size << endl;

	for (uint32_t index = 0; index < num_bins; ++index) {
		vector<uint32_t> decoded_data;
		const char *current = encoded_outputs[index].c_str();
		uint32_t length = encoded_outputs[index].length();

		//		timer_start(timerName[1]);
		PatchedFrameOfReference::decode(current, length, decoded_data);
		//		timer_stop(timerName[1]);

		//		decompress_times[index] = timer_get_current_interval(timerName[1]);
		double
				throughput =
						SIZE_IN_MB(inverted_indexes_vect[index].size() * sizeof(uint32_t))
								/ decompress_times[index];

		if (throughput > decompress_max_th) {
			decompress_max_th = throughput;
			decompress_max_length = inverted_indexes_vect[index].size();
		} else if (throughput < decompress_min_th) {
			decompress_min_th = throughput;
			decompress_min_length = inverted_indexes_vect[index].size();
		}
		decompress_avg_th += throughput;
		//		timer_reset(timerName[1]);
	}
	cout << endl << "Decompression" << endl;
	 cout << "Maximum throughput = " << decompress_max_th << " MB/s, for bin size = " << decompress_max_length << " elements" << endl;
	 cout << "Avg. throughput = " << 1.0 * decompress_avg_th / num_bins << endl;
	 cout << "Minimum throughput = " << decompress_min_th << " MB/s, for bin size = " << decompress_min_length << " elements" << endl;

	free(index_lengths);

	cout << poisson_mean << " " << compress_max_th << " " << 1.0
			* compress_avg_th / num_bins << " " << compress_min_th << " "
			<< 1.0 * uncompressed_size / compressed_size << " "
			<< decompress_max_th << " " << 1.0 * decompress_avg_th / num_bins
			<< " " << decompress_min_th << endl;
	//cout << "\n\nTest 3 end\n\n" << endl;
}

void test4() {
	cout << "\n\nTest 4 start\n\n";
	//	timer_init();
	//	char timerName [][256] = {"encode", "decode"};
	boost::random::mt19937 gen;
	boost::poisson_distribution<> distpd(POISSON_MEAN);
	gen.seed(time(NULL));

	uint32_t uncompressed_size = 0, compressed_size = 0;
	vector<vector<uint32_t> > inverted_indexes_vect;
	uint32_t index_lengths[NUM_BINS] = { 0 };
	for (uint32_t index = 0; index < (uint32_t) (SMALL_INDEX_FRACTION
			* NUM_BINS); ++index) {
		index_lengths[index] = distpd(gen);
		uncompressed_size += index_lengths[index];
	}
	for (uint32_t index = (uint32_t) (SMALL_INDEX_FRACTION * NUM_BINS); index
			< NUM_BINS; ++index) {
		index_lengths[index] = MAX_BIN_SIZE;
		uncompressed_size += index_lengths[index];
	}
	uncompressed_size *= sizeof(uint32_t);
	boost::random::uniform_int_distribution<> dist(UNIFORM_INT_LOW,
			UNIFORM_INT_HIGH);
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

	double decompress_max_th = 0, decompress_avg_th = 0, decompress_min_th =
			1000;
	uint32_t decompress_max_length, decompress_min_length;

	for (uint32_t index = 0; index < NUM_BINS; ++index) {
		string encoded_output;
		//		timer_start(timerName[0]);
		PatchedFrameOfReference::encode(&inverted_indexes_vect[index][0],
				inverted_indexes_vect[index].size(), encoded_output);
		//		timer_stop(timerName[0]);
		//		compress_times[index] = timer_get_current_interval(timerName[0]);
		compressed_size += encoded_output.length();
		encoded_outputs.push_back(encoded_output);
		double
				throughput =
						SIZE_IN_MB(inverted_indexes_vect[index].size() * sizeof(uint32_t))
								/ compress_times[index];

		if (throughput > compress_max_th) {
			compress_max_th = throughput;
			compress_max_length = inverted_indexes_vect[index].size();
		} else if (throughput < compress_min_th) {
			compress_min_th = throughput;
			compress_min_length = inverted_indexes_vect[index].size();
		}
		compress_avg_th += throughput;
		//		timer_reset(timerName[0]);
	}
	cout << "Compression" << endl;
	cout << "Maximum throughput = " << compress_max_th
			<< " MB/s, for bin size = " << compress_max_length << " elements"
			<< endl;
	cout << "Avg. throughput = " << (1.0 * compress_avg_th / NUM_BINS)
			<< " MB/s" << endl;
	cout << "Minimum throughput = " << compress_min_th
			<< " MB/s, for bin size = " << compress_min_length << " elements"
			<< endl;
	cout << "Compression ratio = " << 1.0 * uncompressed_size / compressed_size
			<< endl;

	for (uint32_t index = 0; index < NUM_BINS; ++index) {
		vector<uint32_t> decoded_data;
		const char *current = encoded_outputs[index].c_str();
		uint32_t length = encoded_outputs[index].length();

		//		timer_start(timerName[1]);
		PatchedFrameOfReference::decode(current, length, decoded_data);
		//		timer_stop(timerName[1]);

		//		decompress_times[index] = timer_get_current_interval(timerName[1]);
		double
				throughput =
						SIZE_IN_MB(inverted_indexes_vect[index].size() * sizeof(uint32_t))
								/ decompress_times[index];

		if (throughput > decompress_max_th) {
			decompress_max_th = throughput;
			decompress_max_length = inverted_indexes_vect[index].size();
		} else if (throughput < decompress_min_th) {
			decompress_min_th = throughput;
			decompress_min_length = inverted_indexes_vect[index].size();
		}
		decompress_avg_th += throughput;
		//		timer_reset(timerName[1]);
	}
	cout << endl << "Decompression" << endl;
	cout << "Maximum throughput = " << decompress_max_th
			<< " MB/s, for bin size = " << decompress_max_length << " elements"
			<< endl;
	cout << "Avg. throughput = " << 1.0 * decompress_avg_th / NUM_BINS << endl;
	cout << "Minimum throughput = " << decompress_min_th
			<< " MB/s, for bin size = " << decompress_min_length << " elements"
			<< endl;

	cout << "\n\nTest 4 end\n\n" << endl;
}*/
/*
void test_seq() {
	cout << "\n\nTest seq start\n\n";

	vector<uint32_t> data;
	data.push_back(388);
	int rof = 388;
	int size = 300;
	for (int i = 1; i < size; i++) {
		data.push_back(data.back() + 2);
	}

	for (int i = 0; i < size; i++) {
		cout << data[i] << ",";
	}
	cout << endl;

	std::string buffer;
	 PatchedFrameOfReference::encode(&data[0], size, buffer);
	 cout << "pass encode " << endl;

	 vector<uint32_t> decoded_data;
	 uint32_t bitslot = BITNSLOTS(32768);
	 bmap_t * bitmap = (bmap_t*) malloc(bitslot * sizeof(bmap_t));
	 memset(bitmap, 0, bitslot * sizeof(bmap_t));
	 PatchedFrameOfReference::decode_set_bmap(buffer.c_str(), buffer.length(),
	 decoded_data, &bitmap);

	 int k = 0;
	 uint64_t tmp_t = 0;
	 for (; k < bitslot; k++) {
	 tmp_t += __builtin_popcountll(bitmap[k]);
	 }
	 cout << "total bit set " << tmp_t << endl;

	 //recover bitmap
	 unsigned char set_bit_count[65536];
	 unsigned char set_bit_position[65536][16];
	 create_lookup(set_bit_count, set_bit_position);

	 uint32_t reconstct_rid;
	 uint32_t total_recovered = 0;
	 vector<uint32_t> recovered_rid;
	 k = 0;
	 cout << "recover bit map : ";
	 for (; k < bitslot; k++) {
	 uint64_t offset_long_int = k * 64;
	 uint16_t * temp = (uint16_t *) &bitmap[k];
	 uint64_t offset;
	 for (int j = 0; j < 4; j++) {
	 offset = offset_long_int + j * 16;
	 for (int m = 0; m < set_bit_count[temp[j]]; m++) {
	 total_recovered++;
	 reconstct_rid = offset + set_bit_position[temp[j]][m];
	 recovered_rid.push_back((uint32_t) reconstct_rid);
	 cout << reconstct_rid << ",";
	 }
	 }
	 }
	 cout << endl;

	 std::string buffer;
	 PatchedFrameOfReference::encode(&data[0], size, buffer);
	 cout << "pass encode " << endl;

	 vector<uint32_t> decoded_data;
	 uint32_t decoded_length;
	 uint64_t bstat[33] = {0}; uint64_t expstat[129] = {0};
	 print_encode_stat(buffer.c_str(), buffer.length(), &decoded_data[0],&decoded_length, bstat, expstat );
	 cout << "decoded length" << decoded_length << endl;
	 for (int j = 0; j < decoded_length; j ++){
	 cout << decoded_data[j]<< ",";
	 }

	std::string buffer;
	PatchedFrameOfReference::encode(&data[0], size, buffer);
	cout << "pass encode " << endl;

	uint32_t * decoded_data = (uint32_t *) malloc(size * sizeof(uint32_t));

	uint32_t decoded_length;
	uint64_t bstat[33] = { 0 };
	uint64_t expstat[129] = { 0 };
	print_encode_stat(buffer.c_str(), buffer.length(), decoded_data,
			&decoded_length, bstat, expstat);
	//		 decode_rids(buffer.c_str(), buffer.length(), decoded_data,&decoded_length);
	cout << "decoded length" << decoded_length << endl;
	 for (int j = 0; j < decoded_length; j++) {
	 cout << decoded_data[j] << ",";
	 }

	cout << "b distribution: ";
	 for (int jk = 0; jk <= 32; jk++) {
	 cout << bstat[jk] << endl;
	 }
	 cout << "exp distribution:";
	 for (int jk = 0; jk <= 128; jk++) {
	 cout << expstat[jk] << endl;
	 }

}*/

#define ELM_T uint32_t
void read_to_memory(char *fn, ELM_T *data, uint32_t num) {

	FILE *f = fopen(fn, "r");
	if (f == NULL) {
		printf("can't open file %s", fn);
		return;
	}
	fread(data, sizeof(ELM_T ), num, f);
	fclose(f);
}

void test_rle_encode() {
	/*vector<uint32_t> data;
	 int rof = 1;
	 data.push_back(rof);
	 int size =177;
	 for (int i = 1; i < size; i++) {
	 if (i == 10){
	 data.push_back(data.back() + 100);
	 }else if (i== 20){
	 data.push_back(data.back() + 1039);
	 }
	 if (i % 2 == 0){
	 data.push_back(data.back() + 100);
	 }
	 else {
	 data.push_back(data.back() + 1);

	 }

	 }

	 for (int i = 0; i < size; i++) {
	 cout << data[i] << ",";
	 }
	 cout << endl;*/

	uint32_t size = 64187;
	uint32_t *data = (uint32_t *) malloc(sizeof(uint32_t) * size);
	char fname[1024] = "/home/xczou/tmp/bin_34.bin";
	read_to_memory(fname, data, size);

	//	std::string buffer;
	char * buffer = (char *) malloc(sizeof(char) * sizeof(uint32_t) * size);
	uint64_t len;

	//	hybrid_encode_rids(&data[0], size, buffer, &len); // for vector<uint32_t> data
	hybrid_encode_rids(data, size, buffer, &len);

	uint32_t bitslot = BITNSLOTS(268435456) + 2;
	bmap_t * bitmap = (bmap_t*) malloc(bitslot * sizeof(bmap_t));
	memset(bitmap, 0, bitslot * sizeof(bmap_t));
	rle_decode_rids((const char *) buffer, len, (void **) &bitmap);

	uint64_t total = 0;
	int kk = 0;
	for (; kk < (bitslot - 2); kk++) {
		total += __builtin_popcountll(bitmap[kk]);
	}
	//	printf("expect total = %d, decoded_length = %u, actual total = %d \n", size, decoded_length, total);
	assert(total == size);

	uint32_t * recovered_rids = (uint32_t *) malloc(sizeof(uint32_t) * total);
	unsigned char set_bit_count[65536];
	unsigned char set_bit_position[65536][16];
	create_lookup(set_bit_count, set_bit_position);

	uint32_t reconstct_rid;
	uint32_t rcount = 0;

	int k = 0;
	for (; k < (bitslot - 2); k++) {
		uint64_t offset_long_int = k * 64; // original index offset
		// 2 bytes (unsigned short int)  = 16 bits
		// 4 bytes (unsigned long int )= 64 bit
		uint16_t * temp = (uint16_t *) &bitmap[k];
		uint64_t offset;
		for (int j = 0; j < 4; j++) {
			offset = offset_long_int + j * 16; // here, 16 is used because temp is 16bits (unsigned short int) pointer
			// set_bit_count for each 2 bytes, the number of 1
			/*
			 * *******|               64 bits                 | => final_result_bitmap []
			 * *******| 16 bits | 16 bits | 16 bits | 16 bits | => temp[]
			 */
			/*if(k == 2)
			 printf("%#.4x, ", temp[j]);*/
			for (int m = 0; m < set_bit_count[temp[j]]; m++) {
				reconstct_rid = offset + set_bit_position[temp[j]][m];
				//					cout <<  reconstct_rid << ",";

				recovered_rids[rcount++] = reconstct_rid;
				//								rcount ++;
				//				printf("%lu,", (unsigned long)reconstct_rid);
			}
		}
	}
	//		cout << endl;

	//		assert(memcmp(&data[0], recovered_rids, sizeof(uint32_t)*total) == 0); // for vector<uint32_t> data
	assert(memcmp(data, recovered_rids, sizeof(uint32_t)*total) == 0);
	free(data);

	free(bitmap);
	free(buffer);

}



void test_small_chunk_pfd(){
	uint32_t data[10] = {10,15,20,21,
			        23,30,33,35,
			        41,45};
	uint64_t size = 10, len ;
	char * buffer = (char *) malloc(
			sizeof(char) * sizeof(uint32_t) * size* 2);
	encode_rids(&data[0], size, buffer, &len);
	printf("%lu\n",len);

	uint32_t * output = (uint32_t *) malloc(sizeof(uint32_t) * size * 2);
		uint32_t outputCount;

		 decode_rids(buffer, len, output,
				&outputCount);

		assert(memcmp(data, output, sizeof(uint32_t)*size) == 0);
		free(buffer);
		free(output);
}


void test_sep_pfd() {
	vector<uint32_t> data;
	uint32_t SIZE_TO_GEN = 1059; //1928;
	boost::random::mt19937 gen;
	int delta_ub = 100;
	boost::random::uniform_int_distribution<> dist(1, delta_ub);
	data.push_back((uint32_t) (1000.0 * rand() / RAND_MAX)); // generate first element
	for (uint32_t i = 1; i < SIZE_TO_GEN; i++) {
		int delta = dist(gen);
		data.push_back(data.back() + (uint32_t) delta);
	}
	cout << "data size " << data.size() << endl;
	sort(data.begin(), data.end());

	//todo: maximum buffer size calculation
	printf("************encode********\n");
	char * buffer = (char *) malloc(
			sizeof(char) * sizeof(uint32_t) * SIZE_TO_GEN * 2);
	uint64_t len;
	encode_rids(&data[0], SIZE_TO_GEN, buffer, &len);

	printf("************decode********\n");
	uint32_t bitslot = BITNSLOTS(268435456) + 2;
	bmap_t * bitmap = (bmap_t*) malloc(bitslot * sizeof(bmap_t));
	memset(bitmap, 0, bitslot * sizeof(bmap_t));

	decode_rids_set_bmap(buffer, len, (void **) &bitmap);

	uint32_t * recovered_rids = (uint32_t *) malloc(
			sizeof(uint32_t) * SIZE_TO_GEN);
	unsigned char set_bit_count[65536];
	unsigned char set_bit_position[65536][16];
	create_lookup(set_bit_count, set_bit_position);

	uint32_t reconstct_rid;
	uint32_t rcount = 0;

	int k = 0;
	for (; k < (bitslot - 2); k++) {
		uint64_t offset_long_int = k * 64;
		uint16_t * temp = (uint16_t *) &bitmap[k];
		uint64_t offset;
		for (int j = 0; j < 4; j++) {
			offset = offset_long_int + j * 16; // here, 16 is used because temp is 16bits (unsigned short int) pointer

			for (int m = 0; m < set_bit_count[temp[j]]; m++) {
				reconstct_rid = offset + set_bit_position[temp[j]][m];

				recovered_rids[rcount++] = reconstct_rid;
			}
		}
	}

	assert(memcmp(&data[0], recovered_rids, sizeof(uint32_t)*SIZE_TO_GEN) == 0);

	free(bitmap);
	free(buffer);

	printf("successful \n");
}

void test_skipping() {
	vector<uint32_t> data;
	uint32_t SIZE_TO_GEN = 1059; //1928;
	boost::random::mt19937 gen;
	int delta_ub = 100;
	boost::random::uniform_int_distribution<> dist(1, delta_ub);
	data.push_back((uint32_t) (1000.0 * rand() / RAND_MAX)); // generate first element
	for (uint32_t i = 1; i < SIZE_TO_GEN; i++) {
		int delta = dist(gen);
		data.push_back(data.back() + (uint32_t) delta);
	}
	cout << "data size " << data.size() << endl;
	sort(data.begin(), data.end());

	//todo: maximum buffer size calculation
	printf("************encode********\n");
	char * buffer = (char *) malloc(
			sizeof(char) * sizeof(uint32_t) * SIZE_TO_GEN * 2);
	uint64_t len;
	encode_skipping(&data[0], SIZE_TO_GEN, buffer, &len);

	printf("************decode********\n");
	uint32_t * output = (uint32_t *) malloc(sizeof(uint32_t) * SIZE_TO_GEN * 2);
	uint32_t outputCount;
	decode_skipping_fully(buffer, len, output, &outputCount);

	assert(outputCount == SIZE_TO_GEN);
	assert(memcmp(&data[0], output, outputCount* sizeof(uint32_t)) == 0);

	free(buffer);
	free(output);
}

void test_skipping_partially_decoing() {
	vector<uint32_t> data;
	uint32_t SIZE_TO_GEN = 1059; //1928;
	boost::random::mt19937 gen;
	int delta_ub = 100;
	boost::random::uniform_int_distribution<> dist(1, delta_ub);
	data.push_back((uint32_t) (1000.0 * rand() / RAND_MAX)); // generate first element
	for (uint32_t i = 1; i < SIZE_TO_GEN; i++) {
		int delta = dist(gen);
		data.push_back(data.back() + (uint32_t) delta);
	}
	cout << "data size " << data.size() << endl;
	sort(data.begin(), data.end());

	//todo: maximum buffer size calculation
	printf("************encode********\n");
	char * buffer = (char *) malloc(
			sizeof(char) * sizeof(uint32_t) * SIZE_TO_GEN * 2);
	uint64_t len;
	encode_skipping(&data[0], SIZE_TO_GEN, buffer, &len);

	printf("************decode********\n");
	uint32_t block_size = pfordelta_block_size();
	uint32_t * output = (uint32_t *) malloc(sizeof(uint32_t) * block_size);
	uint32_t outputCount;
	uint32_t rid = 8200;
	int rt = decode_skipping_partially(buffer, len, rid, output, &outputCount);
	if (rt == 2) {
		printf(" rid is not found \n");
	} else if (rt == 1) {

		printf("decoded size %u \n", outputCount);
		for (uint32_t i = 0; i < outputCount; ++i) {
			printf("%u,", output[i]);
		}
	} else {
		printf("decoding error \n");
	}

}

void test_separate_skipping_partially() {
	vector<uint32_t> data;
	uint32_t SIZE_TO_GEN = 1059; //1928;
	boost::random::mt19937 gen;
	int delta_ub = 100;
	boost::random::uniform_int_distribution<> dist(1, delta_ub);
	data.push_back((uint32_t) (1000.0 * rand() / RAND_MAX)); // generate first element
	for (uint32_t i = 1; i < SIZE_TO_GEN; i++) {
		int delta = dist(gen);
		data.push_back(data.back() + (uint32_t) delta);
	}
	cout << "data size " << data.size() << endl;
	sort(data.begin(), data.end());

	//todo: maximum buffer size calculation
	printf("************encode********\n");
	char * buffer = (char *) malloc(
			sizeof(char) * sizeof(uint32_t) * SIZE_TO_GEN * 2);
	uint64_t len;
	encode_skipping(&data[0], SIZE_TO_GEN, buffer, &len);

	printf("************decode********\n");
	uint32_t block_size = pfordelta_block_size();
	uint32_t * output = (uint32_t *) malloc(sizeof(uint32_t) * block_size);
	uint32_t outputCount;
	uint32_t rid = 100;
	int rt = decode_skipping_partially(buffer, len, rid, output, &outputCount);
	int64_t block_idx;
	int rt2 = decode_skipping_partially_search_block(buffer, len, rid,
			&block_idx);

	if (rt == 2) {
		assert(rt2 == 0);
		printf(" rid is not found \n");
	} else if (rt == 1) {
		assert(rt2 == 1);
		uint32_t * output2 = (uint32_t *) malloc(sizeof(uint32_t) * block_size);
		uint32_t outputCount2;
		decode_skipping_partially_decompress_block(buffer, len, block_idx,
				output2, &outputCount2);

		assert(outputCount == outputCount2);
		assert(memcmp(output, output2, outputCount* sizeof(uint32_t)) == 0);
		printf("decoded size %u \n", outputCount);
		for (uint32_t i = 0; i < outputCount; ++i) {
			printf("%u,", output[i]);
		}

	} else {
		printf("decoding error \n");
	}

}

void test_expansion_encode_decode() {
	uint32_t delta[128] =
			{ 0, 9, 393, 402, 6, 395, 803, 3, 398, 2, 399, 400, 2, 3, 395, 3,
					1, 396, 400, 400, 409, 790, 7, 2, 397, 1, 392, 7, 399, 1,
					399, 4, 789, 417, 395, 787, 809, 387, 1, 1, 15, 400, 373,
					27, 2, 13, 364, 18, 3, 1, 6, 1, 1, 1, 386, 4, 10, 364, 26,
					374, 19, 4, 3, 5, 1, 2, 3, 364, 20, 2, 4, 10, 386, 15, 365,
					35, 367, 1, 1, 2, 1, 1, 1, 1, 1, 1, 3, 3, 18, 2001, 400,
					400, 400, 799, 399, 400, 399, 1, 398, 2, 797, 399, 802,
					396, 394, 1, 1, 3, 4, 394, 1194, 12, 1, 1, 1201, 1, 376, 5,
					20, 401, 400, 374, 26, 775, 9, 415, 382, 398 };

	uint32_t *data = (uint32_t *) malloc(sizeof(uint32_t) * 128);
	data[0] = 1888;
	printf("************encode********\n");
	for (int var = 1; var < 128; ++var) {
		data[var] = data[var - 1] + delta[var];
	}
	//	printf("dad = %u\n", data[127]);


	int SIZE_TO_GEN = 128;
	char * exp_buf = (char *) malloc(
			sizeof(char) * sizeof(uint32_t) * SIZE_TO_GEN * 2);
	uint64_t len;
	expand_encode_rids(data, SIZE_TO_GEN, exp_buf, &len);

	uint32_t total = 128;
	uint32_t bitslot = BITNSLOTS(268435456) + 2;
	bmap_t * bitmap = (bmap_t*) malloc(bitslot * sizeof(bmap_t));
	memset(bitmap, 0, bitslot * sizeof(bmap_t));

	printf("************decode********\n");
	expand_decode_rids(exp_buf, len, (void **) &bitmap);

	uint32_t * recovered_rids = (uint32_t *) malloc(sizeof(uint32_t) * total);
	unsigned char set_bit_count[65536];
	unsigned char set_bit_position[65536][16];
	create_lookup(set_bit_count, set_bit_position);

	uint32_t reconstct_rid;
	uint32_t rcount = 0;

	int k = 0;
	for (; k < (bitslot - 2); k++) {
		uint64_t offset_long_int = k * 64;
		uint16_t * temp = (uint16_t *) &bitmap[k];
		uint64_t offset;
		for (int j = 0; j < 4; j++) {
			offset = offset_long_int + j * 16; // here, 16 is used because temp is 16bits (unsigned short int) pointer

			for (int m = 0; m < set_bit_count[temp[j]]; m++) {
				reconstct_rid = offset + set_bit_position[temp[j]][m];

				recovered_rids[rcount++] = reconstct_rid;
			}
		}
	}

	assert(memcmp(data, recovered_rids, sizeof(uint32_t)*total) == 0);
	free(data);

	free(bitmap);
	free(exp_buf);
}

int main(int argc, char **argv) {
	test_small_chunk_pfd();
//	test_sep_pfd();
	//	test_expansion_encode_decode();
	//	test_separate_skipping_partially();
	//	test_skipping();
	//	test_rle_encode();
	//	test_seq();
	//	test2();
	/*test1();
	 test2();*/
	/*if (argc < 7)  {
	 cout << "Usage : ./unittest <poisson mean> <small index fraction> <num bins> <max bin size> <uniform int low> <uniform int high> " << endl;
	 } else {
	 test3(atoi(argv[1]), atof(argv[2]), atoi(argv[3]), atoi(argv[4]),
	 atoi(argv[5]), atoi(argv[6]));
	 }*/
}
