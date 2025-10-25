#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define DEVICE_PATH "/dev/simtemp"
#pragma pack(push, 1)
struct temp_sample {
    int64_t timestamp_ns;  // 8 bytes
    int32_t temperature;   // 4 bytes
};
#pragma pack(pop)

int main(void)
{
    int fd;
    struct temp_sample sample;

    fd = open(DEVICE_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Could not open /dev/simtemp");
        return 1;
    }

    printf("Reading temperature (non-locking mode)...\n");

    while (1) {
        ssize_t ret = read(fd, &sample, sizeof(sample));

        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(200000); // 200 ms
                continue;
            } else {
                perror("Read error");
                break;
            }
        } else if (ret == 0) {
            printf("Reading end.\n");
            break;
        } else if (ret != sizeof(sample)) {
            fprintf(stderr, "Unexpected size: read %zd bytes (expected %zu)\n",
                    ret, sizeof(sample));
            break;
        }

        printf("Temperature: %.2f °C  (timestamp=%" PRId64 " ns)\n",
            sample.temperature / 1000.0, sample.timestamp_ns);

        usleep(1000000); // 1 s
    }

    close(fd);
    return 0;
}

