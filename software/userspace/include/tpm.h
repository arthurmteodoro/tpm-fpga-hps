#ifndef __TPM__
#define __TPM__

#include <stdint.h>
#include <time.h>

typedef struct tpm *TPM;

#define HEBBIAN 0
#define ANTI_HEBBIAN 1
#define RANDOM_WALK 2

extern clock_t total_clocks_software_tpm;

TPM init_tpm(int k, int n, int l, int rule, uint32_t seed);
void destroy_tpm(TPM tpm);
void set_lfsr32_seeds(TPM tpm, const uint32_t* seeds, int size);
void generate_x(TPM tpm);
int calc_output(TPM tpm);
void update(TPM tpm, int y);
int compare_w(TPM tpm1, TPM tpm2);
void exit_w(TPM tpm, int* buf);
void exit_x(TPM tpm, int* buf);
void exit_o(TPM tpm, int* buf);
void exit_lfsr(TPM tpm, uint32_t* buf);

#endif