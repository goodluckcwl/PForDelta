/*
Variable Byte Encoding Header
Author : Saurabh V. Pendse
*/

#ifndef VARBYTE_H
#define VARBYTE_H
#include <vector>

namespace pfor {
    class VarByte {
	public:
        static bool encode(const uint32_t *data, uint32_t data_size, 
                    std::string &buffer);

        static bool decode(const void *buffer, uint32_t buffer_size, 
                    std::vector<uint32_t> &data);
    };
}

#endif
