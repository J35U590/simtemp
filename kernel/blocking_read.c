#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

struct simtemp_sample {
    int64_t timestamp_ns;
    int32_t temperature;
};

int main() {
    int fd;
    struct simtemp_sample sample;

    fd = open("/dev/simtemp", O_RDONLY);
    if (fd < 0) {
        perror("Error opening /dev/simtemp");
        return 1;
    }

    printf("Reading temperature (block mode)...\n");
    while (1) {
        int ret = read(fd, &sample, sizeof(sample));
        if (ret < 0) {
            perror("Read error");
            break;
        }
        printf("Current temperature: %.2f °C  (timestamp=%lld ns)\n",
               sample.temperature / 1000.0, (long long)sample.timestamp_ns);
        sleep(1);
    }

    close(fd);
    return 0;
}


