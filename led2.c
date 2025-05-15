#include <stdio.h>
#include <unistd.h> // For sleep()
#include <fcntl.h>  // For open()
#include <string.h> // For strlen()

int main(void)
{
    const int num_leds = 9;
    char filepath[100];
    int fd;

    while (1) {
        for (int i = 1; i <= num_leds; i++) {
            // Turn on the current LED
            snprintf(filepath, sizeof(filepath), "/sys/class/leds/fpga_led%d/brightness", i);
            fd = open(filepath, O_WRONLY);
            if (fd >= 0) {
                write(fd, "1", 1);
                close(fd);
            }

            // Turn off the previous LED (if not the first one)
            if (i > 1) {
                snprintf(filepath, sizeof(filepath), "/sys/class/leds/fpga_led%d/brightness", i - 1);
                fd = open(filepath, O_WRONLY);
                if (fd >= 0) {
                    write(fd, "0", 1);
                    close(fd);
                }
            }

            // Add a delay
            sleep(1);
        }

        // Turn off the last LED to complete the cycle
        snprintf(filepath, sizeof(filepath), "/sys/class/leds/fpga_led%d/brightness", num_leds);
        fd = open(filepath, O_WRONLY);
        if (fd >= 0) {
            write(fd, "0", 1);
            close(fd);
        }
    }

    return 0;
}