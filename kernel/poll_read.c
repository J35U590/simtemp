#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stdint.h>
#include <string.h>

struct simtemp_sample {
    int64_t ts_ns;   // timestamp en nanosegundos
    int32_t temp_mC; // temperatura en milicelsius
} __attribute__((packed));

int main() {
    int fd;
    struct pollfd fds;
    struct simtemp_sample sample;

    fd = open("/dev/simtemp", O_RDONLY);
    if (fd < 0) {
        perror("Error al abrir /dev/simtemp");
        return 1;
    }

    fds.fd = fd;
    fds.events = POLLIN;

    printf("Esperando eventos con poll()...\n");

    while (1) {
        int ret = poll(&fds, 1, 3000); // timeout 3s
        if (ret == 0) {
            printf("Timeout sin nuevos datos...\n");
        } else if (ret > 0) {
            if (fds.revents & POLLIN) {
                ssize_t bytes = read(fd, &sample, sizeof(sample));
                if (bytes == sizeof(sample)) {
                    printf("Temperatura: %.2f °C  (timestamp=%lld ns)\n",
                           sample.temp_mC / 1000.0,
                           (long long) sample.ts_ns);
                } else {
                    printf("Tamaño inesperado: %zd bytes (esperaba %zu)\n",
                           bytes, sizeof(sample));
                }
            }
        } else {
            perror("Error en poll");
            break;
        }
    }

    close(fd);
    return 0;
}


