#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
// #include "hwlib.h"
/* #include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h" */
#include "hps_0.h"
#include <inttypes.h>

#include "tpm.h"
#include "tpm_fpga.h"

#define K 3
#define N 4
#define L 5

#define HPS_TO_FPGA_LW_BASE 0xFF200000
#define HPS_TO_FPGA_LW_SPAN 0x0020000

int compare_buf(int* buf1, int* buf2, int size) {
  for(int i = 0; i < size; i++) {
    if (buf1[i] != buf2[i]) return 0;
  }
  return 1;
}

int main(int argc, char** argv) {
  setbuf(stdout, NULL);
  srand(time(NULL));

  void* virtual_base = 0;
  int devmem_fd = 0;

  devmem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (devmem_fd < 0) {
    perror("devmem open");
    exit(EXIT_FAILURE);
  }

  virtual_base = (uint32_t*) mmap(NULL, HPS_TO_FPGA_LW_SPAN, PROT_READ|PROT_WRITE, MAP_SHARED, devmem_fd, HPS_TO_FPGA_LW_BASE);
  if (virtual_base == MAP_FAILED) {
    perror("devmem mmap");
    close(devmem_fd);
    exit(EXIT_FAILURE);
  }

  uint32_t* tpm_map = virtual_base + ((uint32_t) (TPM_0_BASE));

  int random_bob = rand();
  int random_alice = rand();
  TPM Alice = init_tpm(K, N, L, HEBBIAN, random_alice);
  generate_w(tpm_map, random_bob);

  uint32_t seeds[K];
  for(int i = 0; i < K; i++)
    seeds[i] = (uint32_t) rand();
  
  set_lfsr32_seeds(Alice, seeds, K);
  load_lfsr_x(tpm_map, seeds, K);

  int sync = 0;
  int ages = 0;

  int bufAlice[K*N];
  int bufBob[K*N];

  int bufXAlice[K*N];
  int bufXBob[K*N];

  int bufOAlice[K];
  int bufOBob[K];

  uint32_t bufLFSR32[K];

  FILE* file = fopen("output_8_16_8.txt", "w");

  fprintf(file, "K: %d N: %d L: %d\n", K, N, L);
  fprintf(file, "Random CPU W seed: %x\n", random_alice);
  fprintf(file, "Random FPGA W seed: %x\n", random_bob);
  fprintf(file, "Seeds X: \n");
  for(int i = 0; i < K; i++)
    fprintf(file, "%x ", seeds[i]);
  fprintf(file, "\n\n");

  int counter = 0;
  clock_t begin = clock();
  while(!sync) {
    fprintf(file, "---------------------------------------------\n");
    fprintf(file, "Age %d\n", ages);
    if((ages % 100) == 0) fprintf(stderr, "%d age\n", ages);

    exit_w(Alice, bufAlice);
    read_w(tpm_map, bufBob, K*N);

    fprintf(file, "Alice W: \n");
    for(int i = 0; i < K; i++) {
      for(int j = 0; j < N; j++)
        fprintf(file, "%d ", bufAlice[i*N+j]);
      fprintf(file, "\n");
    }

    fprintf(file, "\nBob W: \n");
    for(int i = 0; i < K; i++) {
      for(int j = 0; j < N; j++)
        fprintf(file, "%d ", bufBob[i*N+j]);
      fprintf(file, "\n");
    }

    int yAlice = calc_output(Alice);
    int yBob = get_output(tpm_map);

    exit_o(Alice, bufOAlice);
    read_o(tpm_map, bufOBob, K);

    fprintf(file, "Alice O: \n");
    for(int i = 0; i < K; i++) {
      fprintf(file, "%d ", bufOAlice[i]);
    }
    fprintf(file, "\nBob O: \n");
    for(int i = 0; i < K; i++) {
      fprintf(file, "%d ", bufOBob[i]);
    }

    exit_x(Alice, bufXAlice);
    read_x(tpm_map, bufXBob, K*N);
    exit_lfsr(Alice, bufLFSR32);

    fprintf(file, "\nAlice LFSR32: \n");
    for(int i = 0; i < K; i++)
      fprintf(file, "%x ", bufLFSR32[i]);

    fprintf(file, "\nAlice X: \n");
    for(int i = 0; i < K; i++) {
      for(int j = 0; j < N; j++)
        fprintf(file, "%d ", bufXAlice[i*N+j]);
      fprintf(file, "\n");
    }

    fprintf(file, "\nBob X: \n");
    for(int i = 0; i < K; i++) {
      for(int j = 0; j < N; j++)
        fprintf(file, "%d ", bufXBob[i*N+j]);
      fprintf(file, "\n");
    }

    update(Alice, yBob);
    update_w(tpm_map, yAlice);

    fprintf(file, "Alice Y: %d\n", yAlice);
    fprintf(file, "Bob Y: %d\n", yBob);

    exit_w(Alice, bufAlice);
    read_w(tpm_map, bufBob, K*N); 

    if (compare_buf(bufAlice, bufBob, K*N)) sync = 1;
    ages++;
  }
  clock_t end = clock();

  double time_elapsed =  (double)(end - begin) / CLOCKS_PER_SEC;
  fprintf(stdout, "\n\n\nTotal time: %lf secs, %lf milisecs\n", time_elapsed, time_elapsed*1000);
  fprintf(stdout, "Total ages: %d\n", ages);
  printf("Total time in TPM software: %lf\n", ((double)total_clocks_software_tpm)/CLOCKS_PER_SEC);
  printf("Total time in TPM in FPGA: %lf\n", ((double)total_clocks_fpga_tpm)/CLOCKS_PER_SEC);

  // fclose(file);
  /*double time_elapsed =  (double)(end - begin) / CLOCKS_PER_SEC;
  total_time += time_elapsed;
  printf("\nTime elapsed %lf secs, %lf mili ages: %d\n", time_elapsed, time_elapsed*1000, ages);
  printf("Total time in TPM software: %lf\n", ((double)total_clocks_software_tpm)/CLOCKS_PER_SEC);
  printf("Total time in TPM in FPGA: %lf\n", ((double)total_clocks_fpga_tpm)/CLOCKS_PER_SEC);*/

  // printf("mean time: %lf secs, %lf mili\n", total_time/1.00, (total_time/1.00)*1000);

  int result = munmap(virtual_base, HPS_TO_FPGA_LW_SPAN); 
  if( result < 0) {
    perror("devmem munmap");
    close(devmem_fd);
    exit(EXIT_FAILURE);
  }

  close(devmem_fd);
  exit(EXIT_SUCCESS);
}