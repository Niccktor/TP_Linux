#include <stdio.h>
#include <unistd.h> // For sleep()

int main(void)
{
    const int num_leds = 9;
    char command[100];

    while (1) {
        for (int i = 1; i <= num_leds; i++) {
            // Turn on the current LED
            snprintf(command, sizeof(command), "echo \"1\" > /sys/class/leds/fpga_led%d/brightness", i);
            system(command);

            // Turn off the previous LED (if not the first one)
            if (i > 1) {
                snprintf(command, sizeof(command), "echo \"0\" > /sys/class/leds/fpga_led%d/brightness", i - 1);
                system(command);
            }

            // Add a delay
            sleep(1);
        }

        // Turn off the last LED to complete the cycle
        snprintf(command, sizeof(command), "echo \"0\" > /sys/class/leds/fpga_led%d/brightness", num_leds);
        system(command);
    }

    return 0;
}