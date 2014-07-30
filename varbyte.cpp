#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "varbyte.h"
#include "timer.h"
#define DATASIZE 10000
#define SIZE_IN_MB(x) (1.0 * (x) / (1024 * 1024))
using namespace std;
using namespace pfor;

bool VarByte::encode(const uint32_t *data, uint32_t data_size, string &buffer) {
    uint32_t x;
    char value; 
    for (uint32_t index = 0; index < data_size; ++index) {
        x = data[index];
        while (x > 127) {
            value = ((x & 127) | 128);
            buffer.append(&value, 1);
            x >>= 7;
        }
        value = x;
        buffer.append(&value, 1);
    }
    return true;
}

bool VarByte::decode(const void *buffer, uint32_t buffer_size, vector<uint32_t> &data) {
    const char *current_buffer = (const char *)buffer;
    uint32_t decoded_value, b;
    int shift;
    uint32_t remaining_buffer = buffer_size;
    while (remaining_buffer > 0) {
        decoded_value = 0, shift = 0;
        while ((b = *current_buffer) > 127) {
            decoded_value |= (b & 127) << shift;
            shift += 7;
            current_buffer++;
            remaining_buffer--;
        }
        current_buffer++;
        remaining_buffer--;
        data.push_back(decoded_value |= (b << shift));
    }
    return true; 
}

/*int main(int argc, char **argv) {
    string str;
    uint32_t x[DATASIZE];
    for (uint32_t index = 0; index < DATASIZE; ++index) {
        x[index] = index;
    }
    char value;
    vector<uint32_t> decoded_data;
	timer_init();
	char timerName [][256] = {"encode", "decode"};

	timer_start(timerName[0]);
    VarByte::encode(x, DATASIZE, str);
	timer_stop(timerName[0]);

	cout << "Compression throughput = " 
		 << SIZE_IN_MB(1.0 * DATASIZE * sizeof(uint32_t)) / timer_get_total_interval(timerName[0]) << endl;
	timer_start(timerName[1]);
    VarByte::decode(str.c_str(), str.length(), decoded_data);
	timer_stop(timerName[1]);
	cout << "Compression throughput = " 
		 << SIZE_IN_MB(1.0 * DATASIZE * sizeof(uint32_t)) / timer_get_total_interval(timerName[1]) << endl;
 
    cout << "Compression ratio = " << DATASIZE * sizeof(uint32_t) / str.length()
         << endl;   
    for (uint32_t index = 0; index < decoded_data.size(); ++index) {
        cout << decoded_data[index] << " ";
    }
    cout << endl;
	timer_finalize();
}*/
