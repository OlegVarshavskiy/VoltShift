#ifndef VOLTSHIFT_GPU_H
#define VOLTSHIFT_GPU_H
#include <stddef.h>
typedef struct { char name[1024]; char instance_id[1024]; } GPUInfo;
typedef struct {
    GPUInfo *items;
    size_t count;
    unsigned long error;
    int complete;
} GPUList;
int gpu_detect(GPUList *list);
void gpu_free(GPUList *list);
#endif
