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
        perror("Error al abrir /dev/simtemp");
        return 1;
    }

    printf("Leyendo temperatura (modo bloqueante)...\n");
    while (1) {
        int ret = read(fd, &sample, sizeof(sample));
        if (ret < 0) {
            perror("Error en read");
            break;
        }
        printf("Temperatura actual: %.2f °C  (timestamp=%lld ns)\n",
               sample.temperature / 1000.0, (long long)sample.timestamp_ns);
        sleep(1);
    }

    close(fd);
    return 0;
}


