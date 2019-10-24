#include "tpm.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

clock_t total_clocks_software_tpm = 0;

void lfsr(uint32_t* lfsr) {
    uint32_t bit = ((*lfsr >> 0) ^ (*lfsr >> 10) ^ (*lfsr >> 30) ^ (*lfsr >> 31));
    *lfsr = (*lfsr >> 1) | (bit << 31);
}

int sign(int a) {
    if(a >= 0)
        return 1;
    else
        return -1;
}

int step(int y, int o) {
    if((y * o) <= 0)
        return 0;
    else
        return 1;
}

int hebbian(int w, int x, int y, int o) {
    if (y != o) return w;
    return w + (x * y);
}

int anti_hebbian(int w, int x, int y, int o) {
    return w - (x*o) * step(y, o);
}

int random_walk(int w, int x, int y, int o) {
    return w + x * step(y, o);
}

int clip(int x, int l) {
    if(x > l)
        return l;
    else if(x < -l)
        return -l;
    else
        return x;
}


struct tpm {
    int k;
    int n;
    int l;
    int rule;

    int* w;
    int* x;
    int* o;
    int y;
    uint32_t* lfsr32;
};

TPM init_tpm(int k, int n, int l, int rule, uint32_t seed) {
    TPM tpm = (TPM) malloc(sizeof(struct tpm));
    tpm->k = k;
    tpm->n = n;
    tpm->l = l;
    tpm->rule = rule;

    tpm->w = (int*) malloc(sizeof(int)*k*n);
    tpm->x = (int*) malloc(sizeof(int)*k*n);
    tpm->o = (int*) malloc(sizeof(int)*k);
    tpm->lfsr32 = (uint32_t*) malloc(sizeof(uint32_t)*k);

    uint32_t seed_w = seed;
    int qtd = (int)ceil(log2(tpm->l));

    for(int i = 0; i < k*n; i++) {
        uint32_t value = seed_w;

        uint32_t need_signal = value & (unsigned int) pow(2, qtd);

        if (need_signal != 0) {
            uint32_t mask_signal = UINT32_MAX;
            for(int j = 0; j < qtd+1; j++)
                mask_signal -= (int) pow(2, j);
            value = value | mask_signal;
        } else {
            uint32_t mask = 0;
            for(int j = 0; j < qtd; j++)
                mask += (int) pow(2, j);
            value = value & mask;
        }
        tpm->w[i] = clip((int) value, tpm->l);
        lfsr(&seed_w);
    }

    return tpm;
}

void set_lfsr32_seeds(TPM tpm, const uint32_t* seeds, int size) {
    for(int i = 0; i < size; i++) {
        tpm->lfsr32[i] = seeds[i];
    }
}

void generate_x(TPM tpm) {
    int qtd = (int)ceil(log2(tpm->l));

    for(int i = 0; i < tpm->k; i++) {
        int index = 0;
        lfsr(&tpm->lfsr32[i]);

        for(int j = 0; j < tpm->n; j++) {
            uint32_t value = tpm->lfsr32[i];
            uint32_t mask = (uint32_t) pow(2, index);
            uint32_t result = (value & mask) >> index;

            result == 1 ? tpm->x[i*tpm->n+j] = 1 : (tpm->x[i*tpm->n+j] = -1);
            index++;
        }
    }
}

int calc_output(TPM tpm) {
    clock_t begin = clock();
    generate_x(tpm);

    for(int i = 0; i < tpm->k; i++) {
        int h = 0;
        for(int j = 0; j < tpm->n; j++) {
            h = h + (tpm->w[i*tpm->n+j] * tpm->x[i*tpm->n+j]);
        }
        tpm->o[i] = sign(h);
    }

    tpm->y = 1;
    for(int i = 0; i < tpm->k; i++) {
        tpm->y = tpm->y * tpm->o[i];
    }
    total_clocks_software_tpm += clock() - begin;
    return tpm->y;
}

void update(TPM tpm, int y) {
    clock_t begin = clock();
    if (y == tpm->y) {
        for(int i = 0; i < tpm->k; i++) {
            for(int j = 0; j < tpm->n; j++) {
                switch (tpm->rule) {
                    case HEBBIAN:
                        tpm->w[i*tpm->n+j] = hebbian(tpm->w[i*tpm->n+j], tpm->x[i*tpm->n+j], tpm->y, tpm->o[i]);
                        break;
                    case ANTI_HEBBIAN:
                        tpm->w[i*tpm->n+j] = anti_hebbian(tpm->w[i*tpm->n+j], tpm->x[i*tpm->n+j], tpm->y, tpm->o[i]);
                        break;
                    case RANDOM_WALK:
                        tpm->w[i*tpm->n+j] = random_walk(tpm->w[i*tpm->n+j], tpm->x[i*tpm->n+j], tpm->y, tpm->o[i]);
                        break;
                    default:
                        tpm->w[i*tpm->n+j] = hebbian(tpm->w[i*tpm->n+j], tpm->x[i*tpm->n+j], tpm->y, tpm->o[i]);
                        break;
                }
                tpm->w[i*tpm->n+j] = clip(tpm->w[i*tpm->n+j], tpm->l);
            }
        }
    }
    total_clocks_software_tpm += clock() - begin;
}

int compare_w(TPM tpm1, TPM tpm2) {
    for(int i = 0; i < tpm1->k*tpm1->n; i++) {
        if (tpm1->w[i] != tpm2->w[i]) return 0;
    }
    return 1;
}

void exit_w(TPM tpm, int* buf) {
    for(int i = 0; i < tpm->k*tpm->n; i++)
        buf[i] = tpm->w[i];
}

void exit_x(TPM tpm, int* buf) {
    for(int i = 0; i < tpm->k*tpm->n; i++)
        buf[i] = tpm->x[i];
}

void exit_o(TPM tpm, int* buf) {
    for(int i = 0; i < tpm->k; i++)
        buf[i] = tpm->o[i];
}

void exit_lfsr(TPM tpm, uint32_t* buf) {
    for(int i = 0; i < tpm->k; i++)
        buf[i] = tpm->lfsr32[i];
}

void destroy_tpm(TPM tpm) {
    free(tpm->w);
    free(tpm->o);
    free(tpm->x);
    free(tpm->lfsr32);
}
