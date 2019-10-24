#include "tpm_fpga.h"

#include <unistd.h>

clock_t total_clocks_fpga_tpm = 0;

void generate_w(uint32_t* base, int lfsr_seed) {
  IOWR(base, LOAD_SEED_LFSR_W, lfsr_seed);
  IOWR(base, GENERATE_W, 0);
}

void load_lfsr_x(uint32_t* base, uint32_t* seeds, int n) {
  for(int i = 0; i < n; i++) {
    IOWR(base, LOAD_SEED_LFSR_X, seeds[i]);
  }
}

int get_output(uint32_t* base) {
  clock_t begin = clock();
  IOWR(base, GENERATE_X, 0);
  IOWR(base, GENERATE_OUTPUT, 0);
  int y = (int) IORD(base, READ_OUTPUT);
  total_clocks_fpga_tpm += clock() - begin;
  return y;
}

void update_w(uint32_t* base, int y) {
  clock_t begin = clock();
  IOWR(base, LOAD_Y_BOB, y);
  IOWR(base, UPDATE_W, 0);
  total_clocks_fpga_tpm += clock() - begin;
}

void read_w(uint32_t* base, int* buf, int n) {
  for(int i = 0; i < n; i++) {
    buf[i] = IORD(base, READ_W);
  }
}

void read_x(uint32_t* base, int* buf, int n) {
  for(int i = 0; i < n; i++) {
    buf[i] = IORD(base, READ_X);
  }
}

void read_o(uint32_t* base, int* buf, int n) {
  for(int i = 0; i < n; i++) {
    buf[i] = IORD(base, READ_O);
  }
}
