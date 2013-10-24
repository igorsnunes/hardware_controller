#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>
#define BRAM_SIZE  0x4000
#include "/usr/include/pciDriver/lib/pciDriver.h"//TODO: smaller include
#include "/usr/include/pciDriver/driver/pciDriver.h"

void DMAKernelClearBuffer(uint32_t *bar0, uint32_t *bar1, const unsigned long test_len);
void DMAKernelMemoryWrite(uint32_t *bar0, uint32_t *bar1, uint64_t *bar2, pd_kmem_t *km, const unsigned long test_len, void *kernel_memory,int block,int offset);
void DMAKernelMemoryRead(uint32_t *bar0, uint32_t *bar1, uint64_t *bar2, pd_kmem_t *km, const unsigned long test_len, void *kernel_memory,int block, int offset);
void writeDMA(uint32_t *bar0, unsigned long ha, unsigned long pa, unsigned long next, unsigned long size, unsigned int bar_no, int block);
void readDMA(uint32_t *bar0, unsigned long ha, unsigned long pa, unsigned long next, unsigned long size, unsigned int bar_no, int block);
