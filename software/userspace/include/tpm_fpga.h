#ifndef __TPM_FPGA__
#define __TPM_FPGA__

#include <inttypes.h>
#include <time.h>

extern clock_t total_clocks_fpga_tpm;

#define IORD(base,reg) *(base+reg)
#define IOWR(base,reg,data) *(base+reg) = data

#define LOAD_SEED_LFSR_W 0x01
#define GENERATE_W 0x02
#define LOAD_SEED_LFSR_X 0x80
#define GENERATE_X 0x03
#define GENERATE_OUTPUT 0x04
#define READ_OUTPUT 0x11
#define READ_W 0x12
#define READ_X 0x13
#define READ_O 0x14
#define LOAD_Y_BOB 0x05
#define UPDATE_W 0x06

void generate_w(uint32_t* base, int lfsr);
void load_lfsr_x(uint32_t* base, uint32_t* seeds, int n);
int get_output(uint32_t* base);
void update_w(uint32_t* base, int y);
void read_w(uint32_t* base, int* buf, int n);
void read_x(uint32_t* base, int* buf, int n);
void read_o(uint32_t* base, int* buf, int n);

#endif