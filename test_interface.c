#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pfordelta-c-interface.h"

#define DATA_SIZE 1000

void HPM_Init(void);
void HPM_Start(char *);
void HPM_Stop(char *);
void HPM_Print(void);
void HPM_Print_Flops(void);
void HPM_Print_Flops_Agg(void);
void HPM_Flops( char *, double *, double * );
int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  printf("hello\n");
 
  uint32_t index;
  uint32_t *decoded = (uint32_t *)malloc(sizeof(uint32_t) * 3 * DATA_SIZE);
  uint32_t decoded_count = 0;
  uint32_t *input = (uint32_t *)malloc(sizeof(uint32_t)  * DATA_SIZE);
  for (index = 0; index < DATA_SIZE; ++index) {
      input[index] = (index * index);
  }
  /*input[0] = 1;
  input[1] = 3;
  input[2] = 6;
  input[3] = 10;
  input[4] = 15;*/
  
  uint32_t inputCount = DATA_SIZE;
  uint64_t maxOutputSize = get_sufficient_buffer_capacity(inputCount);

  char *output = (char *)malloc(sizeof(char) * maxOutputSize);
  char *concat_output = (char *)malloc(sizeof(char) * 3 * maxOutputSize);

  uint64_t outputLength; 
  HPM_Init();
  HPM_Start("Cache");
  printf("\n\n\ninput data\n");
  for (index = 0; index < DATA_SIZE; ++index) {
      printf("%d ", input[index]);
  }
  printf("\n");
  int r = encode_rids(input, inputCount, output, &outputLength);
  HPM_Stop("Cache");
  printf("encode returns = %d\n", r);
  printf("outputLength = %d\n", outputLength);

  r = decode_rids(output, outputLength, decoded, &decoded_count);
  printf("decode returns = %d\n", r);
  printf("outputLength = %d\n", outputLength);

  printf("\n\n\nDecoded before update\n");
  for (index = 0; index < DATA_SIZE; ++index) {
      printf("%d ", decoded[index]);
  }
  printf("\n");
  r = update_rids(output, outputLength, 50);
  r = decode_rids(output, outputLength, decoded, &decoded_count);

  printf("\n\n\nDecoded after update\n");
  for (index = 0; index < DATA_SIZE; ++index) {
      printf("%d ", decoded[index]);
  }
  printf("\n");

  memcpy(concat_output, output, outputLength);
  memcpy(concat_output + outputLength, output, outputLength);
  memcpy(concat_output + 2 * outputLength, output, outputLength);
  r = decode_rids(concat_output, 3 * outputLength, decoded, &decoded_count);

  printf("\n\n\nDecoded after concatenation : %d\n", decoded_count);
  for (index = 0; index < 3 * DATA_SIZE; ++index) {
	printf("%d ", decoded[index]);
  }
  printf("\n");
  return 0;
}
