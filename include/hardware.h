#ifndef VOLTSHIFT_HARDWARE_H
#define VOLTSHIFT_HARDWARE_H

#define CPU_NAME_LENGTH 128

typedef struct {
    char name[CPU_NAME_LENGTH];
    unsigned int logical_processors;
} CPUInfo;

int hardware_get_cpu(CPUInfo *info);

#endif
