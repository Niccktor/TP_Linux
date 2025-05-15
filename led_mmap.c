#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define PHYS_ADDR 0xFF203000
#define MAP_SIZE 4096

int main() {
    int fd;
    void *mapped_base;

    // Open /dev/mem
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("Error opening /dev/mem");
        return EXIT_FAILURE;
    }

    // Map the physical address
    mapped_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PHYS_ADDR);
    if (mapped_base == MAP_FAILED) {
        perror("Error mapping memory");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Memory mapped at virtual address %p\n", mapped_base);

    // Access the mapped memory
    volatile unsigned int *mapped_ptr = (volatile unsigned int *)mapped_base;
    while(1){
    // Chenillard loop
        for (int i = 0; i < 10; i++) {
            *mapped_ptr = (1 << i); // Set one LED at a time
            usleep(500000); // 500ms delay
        }
    }

    // Unmap and close
    if (munmap(mapped_base, MAP_SIZE) == -1) {
        perror("Error unmapping memory");
    }
    close(fd);

    return EXIT_SUCCESS;
}