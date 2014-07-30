#include "inverted_index_process.h"

double dclock1(void) {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (double) tv.tv_sec + (double) tv.tv_usec * 1e-6;
}

/*
 * write compressed_data structure into file
 */
void write_compression_per_bin(struct compressed_data *cd, FILE *fc) {
	// meta data
	fwrite(&(cd->min_bits_per_item), sizeof(uint32_t), 1, fc);
	fwrite(&(cd->unpacked_nums), sizeof(uint32_t), 1, fc);
	fwrite(&(cd->exception_nums), sizeof(uint32_t), 1, fc);
	fwrite(&(cd->exception_offset), sizeof(uint32_t), 1, fc);
	//actual data
	uint32_t packedNums = (cd->min_bits_per_item * cd->unpacked_nums)
			/ BITS_PER_INT;
	uint32_t divisible = (cd->min_bits_per_item * cd->unpacked_nums)
			% BITS_PER_INT;
	packedNums = packedNums + (divisible > 0 ? 1 : 0);
	fwrite(cd->packed, sizeof(uint32_t), packedNums, fc);
	fwrite(cd->exceptions, sizeof(uint32_t), cd->exception_nums, fc);
}

// return number of bytes used in compressed structure
uint32_t compressed_size (struct compressed_data *cd ){
	// total bits / 4 bytes(32)
	uint32_t packedNums = (cd->min_bits_per_item * cd->unpacked_nums)
			/ BITS_PER_INT;
	uint32_t divisible = (cd->min_bits_per_item * cd->unpacked_nums)
			% BITS_PER_INT;
	packedNums = packedNums + (divisible > 0 ? 1 : 0);
	// each number has 4 bytes
	// extra 4 is for the first 4 fields in compressed_data structure
	return 4 * (4 + packedNums + cd->exception_nums  );
}

/*
 * compress values in each bin into file by using pfordelta algorithm
 */
double compress_per_bin(uint32_t * data_per_bin, uint32_t num_per_bin, FILE *fc, uint32_t *memory_compressed_size ) {

	struct compressed_data cd;

	double s_time, e_time;
	s_time = dclock1(); //start timer

	pfordelta_compress(data_per_bin, num_per_bin, &cd);

	e_time = dclock1();

	*memory_compressed_size += compressed_size(&cd);
//	printf("compressed size : %u \n", compressed_size(&cd));
//	print_compressed_data(&cd);
	write_compression_per_bin(&cd, fc);
	free_memory(cd.exceptions);
	free_memory(cd.packed);

	return e_time - s_time;
}

/*
 * read values from each bin and applies pfordelta algorithm on them
 * memory_compress : if it is true, compress the data in memory without writing to file
 */
void compress_bins(char * bin_file, char * output) {

	double compress_time = 0;

	FILE *f = fopen(bin_file, "r");
	if (!f) {
		printf("file %s open error! \n", bin_file);
		return;
	}
	uint32_t lastval = 0;
	uint32_t *data_per_bin = (uint32_t *) malloc(
			sizeof(uint32_t) * MEM_ALLOC_PER_INT);
	uint32_t num_per_bin = 0;
	uint32_t curval;
	uint32_t read_size = 0;
	uint32_t num_bins = 0;
	// used for compute compressed data size in order to avoid
	// cost of writing compressed data file
	uint32_t memory_compressed_size = 0;

	//FILE *fc = fopen(output, "w");

	//fwrite(&num_bins, sizeof(uint32_t), 1, fc); // write the number of bin into file

	// current allocated memory size, we initially allocate 32 size of memory
	// but that space can not held all the values in one bin, we have to
	// dynamically increase memory allocated size
	uint32_t cur_mem_size = MEM_ALLOC_PER_INT;
	// Dont use !foef(f) to determine the end of file
	// This function not work properly, it will read last value twice
	while ((read_size = fread(&curval, sizeof(uint32_t), 1, f)) != 0) {
		if (curval < lastval) { // bin value boundary
//			printf("\n bin %u: \n", num_per_bin);
//			print_data(data_per_bin, num_per_bin);
				// put compressed bits into file

			double time_per_bin = compress_per_bin(data_per_bin, num_per_bin,
					fc, &memory_compressed_size);
			compress_time += time_per_bin;
			num_bins++;
			// start a new memory allocation
			uint32_t * newp = realloc(data_per_bin,
					sizeof(uint32_t) * MEM_ALLOC_PER_INT);
			if (newp != NULL) {
				data_per_bin = newp;
				cur_mem_size = MEM_ALLOC_PER_INT;
				num_per_bin = 0;
			} else {
				printf("Can not re-allocate more memory ! ");
				return;
			}
		}
		data_per_bin[num_per_bin++] = curval;
		if (num_per_bin >= cur_mem_size) { // value in bins exceed the current allocated memory
			cur_mem_size += MEM_ALLOC_PER_INT;
			uint32_t *newp = realloc(data_per_bin,
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
	num_bins++;

	compress_per_bin(data_per_bin, num_per_bin, fc, &memory_compressed_size);
	// Go back at head of file to update the total number of bins
	rewind(fc);
	fwrite(&num_bins, sizeof(uint32_t), 1, fc);
	fclose(f);
	fclose(fc);
	free_memory(data_per_bin);

	printf("Compressed time elapsed: %lf \n", compress_time);
	// extra 4 bytes stands for num_bins field in compressed file
	printf("Compressed memory usage : %u \n", memory_compressed_size + 4);
	printf("Detected number of bins : %u \n", num_bins);
}

/*
 * load compressed data per bin on file into memory
 */
void read_decompression_per_bin(FILE *fc, struct compressed_data *cd) {
	fread(&(cd->min_bits_per_item), sizeof(uint32_t), 1, fc);
	fread(&(cd->unpacked_nums), sizeof(uint32_t), 1, fc);
	fread(&(cd->exception_nums), sizeof(uint32_t), 1, fc);
	fread(&(cd->exception_offset), sizeof(uint32_t), 1, fc);
	uint32_t packedNums = (cd->min_bits_per_item * cd->unpacked_nums)
			/ BITS_PER_INT;
	uint32_t divisible = (cd->min_bits_per_item * cd->unpacked_nums)
			% BITS_PER_INT;
	packedNums = packedNums + (divisible > 0 ? 1 : 0);
	cd->packed = (uint32_t *) malloc(packedNums * sizeof(uint32_t));

	fread(cd->packed, sizeof(uint32_t), packedNums, fc);

	if (cd->exception_nums > 0) {
		cd->exceptions = (uint32_t *) malloc(cd->exception_nums * sizeof(uint32_t));
		if (!cd->exceptions) {
			printf("Can not allocate memory for exceptions list \n");
			return;
		}
		fread(cd->exceptions, sizeof(uint32_t), cd->exception_nums, fc);
	}

}

/*
 * decompressed bins value from a compressed file
 */
void recover_compressed_bins(char *f, char *output) {
	double recover_time = 0;

	FILE *fc = fopen(f, "r");
	if (!fc) {
		printf("file %s open error! \n", f);
		return;
	}
	FILE *fo = fopen(output, "w");
	if (!fo) {
		printf("file %s open error! \n", f);
		return;
	}
	uint32_t num_bins = 0;
	fread(&num_bins, sizeof(uint32_t), 1, fc);
	uint32_t i;
	for (i = 0; i < num_bins; i++) {
		struct compressed_data cd;
		// TO solve exceptions pointer will not been initialized
		// if there is no exceptions in compressed list so that
		// we can not free the exceptions
//		cd.exceptions = NULL;
		read_decompression_per_bin(fc, &cd);
		struct decompressed_data dcd;

		double s_time, e_time;
		s_time = dclock1(); //start timer

		pfordelta_decompress(&cd, &dcd);

		e_time = dclock1();
		recover_time += (e_time - s_time);

		// Currently, write the values into file
		fwrite(dcd.unpacked, sizeof(uint32_t), dcd.unpacked_nums, fo);
//		print_data(dcd.unpacked, dcd.unpackedNums);
		if (cd.exception_nums > 0) {
			free_memory(cd.exceptions);
		}free_memory(cd.packed);
		free_memory(dcd.unpacked);
	}
	fclose(fc);
	fclose(fo);

	printf("Recovered bin elapsed: %lf\n", recover_time);

}

/******************Following functions are auxiliary function****************************************/
/*
 * print out content of file which has a binary value(4 bytes) each line
 * num = 0: print out entire file, otherwise print 'num' number of lines
 */

void print_file(char *fn, int num) {
	FILE *f = fopen(fn, "r");
	if (!f) {
		printf("file open error! \n");
		return;
	}
	uint32_t val = 0;
	int i = 0;
	uint32_t s = 0;
	printf("data: ");
	if (!num) {
		while ((s = fread(&val, sizeof(uint32_t), 1, f)) != 0) {
			i++;
			printf("%u  \n ", val);
		}
	} else {
		while ((s = fread(&val, sizeof(uint32_t), 1, f)) != 0 && i < num) {
			i++;
			printf("%u  \n ", val);
		}

	}
	fclose(f);
	printf("\n total num: %d \n", i);

}

/*
 * print out a array of data
 */
void print_data(uint32_t * data, uint32_t num) {
	printf("data: ");
	uint32_t i;
	for (i = 0; i < num; i++) {
		printf("%u, ", data[i]);
	}
	printf("\n");
}

/*
 * print out number of values in each bin
 */
void print_bin_stat(char *fn) {
	FILE *f = fopen(fn, "r");
	if (!f) {
		printf("file %s open error! \n", fn);
		return;
	}
	uint32_t lastval = 0;
	uint32_t curval;
	uint32_t binsize = 0;
	uint32_t read_size = 0;
	uint32_t num_bins = 0;
	uint32_t total_size = 0;
	while ((read_size = fread(&curval, sizeof(uint32_t), 1, f)) != 0) {
		total_size++;
		if (curval > lastval)
			binsize++;
		else if (curval == lastval) {
			printf("There is two adjacent values are equals in the list! \n ");
			printf(
					"These two equal value[%u] in the [%u] bin and at [%u] position of the list! \n ",
					lastval, num_bins, total_size);
		} else {
			num_bins++;
			printf("Bin[%u]: %u\n", num_bins, binsize);
			binsize = 1;
		}
		lastval = curval;
	}
	num_bins++;
	printf("Bin[%u]: %u\n", num_bins, binsize);
	fclose(f);
}

/*
 * read number of binary values from file into data
 * total number read is returned by num
 */
void read_to_memory(char *fn, uint32_t *data, uint32_t start, uint32_t num) {

	FILE *f = fopen(fn, "r");
	if (!f) {
		printf("file %s open error! \n", fn);
		return;
	}
	fseek(f, start, SEEK_SET);
	fread(data, sizeof(uint32_t), num, f);
	fclose(f);
}

/*
 * write number of value in data to file
 */
void write_to_file(char *fn, uint32_t *data, uint32_t num) {
	FILE *f = fopen(fn, "w");
	if (!f) {
		printf("file %s open error! \n", fn);
		return;
	}
	fwrite(data, sizeof(uint32_t), num, f);
	fclose(f);
}

/*
 * cut start at starting point from original file
 */
void partial_file(char *original, char *output, uint32_t start, uint32_t nums) {
	uint32_t *data = (uint32_t *) malloc(sizeof(uint32_t) * nums);
	read_to_memory(original, data, start, nums);
	write_to_file(output, data, nums);
	free_memory(data);
}
/*
 * take a small part of value from big file , and write these amount of value into small file
 * just for the test purpose
 */
void cut_to_small_file(char *big_file, char *small_file, uint32_t num) {
	uint32_t *data = (uint32_t *) malloc(sizeof(uint32_t) * num);
	read_to_memory(big_file, data, 0, num);
//	print_data(data, num);
	write_to_file(small_file, data, num);
	free_memory(data);
}

void test_compress_bins() {
	char * f = "./small.dat";
	char * out = "./decompress.dat";
	printf("1st Test \n ");
	uint32_t one_bin[5] = { 1, 2, 3, 4, 5 };
	write_to_file(f, one_bin, sizeof(one_bin) / sizeof(uint32_t));
	compress_bins(f, out);

	printf("2nd Test \n ");
	uint32_t one_val_bin[10] = { 1, 2, 3, 4, 5, 4, 1, 2, 3, 4 };
	write_to_file(f, one_val_bin, sizeof(one_val_bin) / sizeof(uint32_t));
	compress_bins(f, out);

	printf("3th Test \n ");
	uint32_t one_val_bin_last[10] = { 1, 2, 3, 4, 5, 1, 2, 3, 4, 1 };
	write_to_file(f, one_val_bin_last,
			sizeof(one_val_bin_last) / sizeof(uint32_t));
	compress_bins(f, out);

	printf("4th Test \n ");
	// test multiple memory allocation
	// MEM_ALLOC_PER_BIN = 5
	uint32_t mem_insufficient[23] = { 1, 2, 3, 4, 1, 2, 3, 4, 5, 6, 7, 1, 2, 3,
			4, 5, 6, 7, 8, 9, 10, 11, 1 };
	write_to_file(f, mem_insufficient,
			sizeof(mem_insufficient) / sizeof(uint32_t));
	compress_bins(f, out);



}

/*
 * return file size in the byte units
 */
long get_file_size(char *fn) {
	FILE *fp = fopen(fn, "r");
	if (!fp) {
		printf("file %s open error! \n", fn);
		return 0;
	}
	fseek(fp, 0L, SEEK_END);
	long sz = ftell(fp);
	fclose(fp);
	return sz;
}


void test_recover_bins(char *original, char *compressed, char *decompressed) {
	compress_bins(original, compressed);
	recover_compressed_bins(compressed, decompressed);
	// we simply say recover is successful only by looking at
	// file size
	if (get_file_size(original) == get_file_size(decompressed)) {
		printf("compression and decompression successfully! \n");
	} else {
		printf("Failed to recover compressed file !\n ");
	}
}

void test_suite_compress_recover(){
	char * f = "small.dat";
	char * out = "small_compress.dat";
	char *r = "small_recover.dat";
	int i ;
	int len = 100;
	uint32_t one_increased[len];
	for (i = 0 ; i< len; i++) {
		one_increased[i] = i+1;
	}
	write_to_file(f, one_increased, len);
	test_recover_bins(f,out,r);
}

int main(int argc, char **argv) {

//	test_suite_compress_recover();
//	print_file("small_compress.dat", 10);
	if (argc <= 1) {
		printf("Need more parameter, choose following options \n");
		int i = 1;
		printf("%d : test_compress_recover bin.dat compressed.dat recovered.dat \n", i++);
		printf("%d : test_compress bin.dat compressed.dat \n", i++);
		printf("%d : test_recover compressed.dat  recovered.dat \n", i++ );
		printf("%d : print_bin_stat bin.dat \n", i++ );
		printf("%d : print_file file [lines #] \n", i++);
		printf("%d : partial_file big_file small_file start[byte] num[4 bytes] \n", i++);
		return 0;
	}

	int op = atoi(argv[1]);
	int s , n;
	switch (op){
	case 1:
		test_recover_bins(argv[2], argv[3],argv[4]);
		break;
	case 2:
		compress_bins(argv[2],argv[3]);
		break;
	case 3:
		recover_compressed_bins(argv[2],argv[3]);
		break;
	case 4:
		print_bin_stat(argv[2]);
		break;
	case 5:
		if (argc ==  4){
			n = atoi(argv[3]);
			print_file(argv[2],n);
		}else if (argc == 3){
			print_file(argv[2], 0);
		}
		break;
	case 6:
	    s = atoi(argv[4]);
		n = atoi(argv[5]);
		partial_file(argv[2], argv[3], s, n);
		break;
	default:
		printf("please input number 1 from 6 \n");
		break;
	}


}
