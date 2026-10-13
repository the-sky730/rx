#ifndef __LLNS_H_
#define __LLNS_H_

typedef struct {
    float gainfloor;
    float suppress_level;
} llns_param_t;

int JLSP_llns_get_heap_size(int *share_head_size, int *private_head_size, int samplerate);

void *JLSP_llns_init(char *private_buffer, int private_size, char *share_buffer, int share_size, int samplerate, float gainfloor, float suppress_level);

int JLSP_llns_reset(void *m);

int JLSP_llns_process(void *m, short *pcm, short *outbuf, int *outsize);

int JLSP_llns_free(void *m);

int JLSP_llns_set_parameters(void *m, llns_param_t *p);

int JLSP_llns_set_winsize(void *m, int winsize);

#endif
