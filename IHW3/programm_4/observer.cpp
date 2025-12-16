#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "shared.h"
#include <signal.h>

shared_t *shm = nullptr;

void handle_sigint(int sig) {
    if (shm) shm->stop = 1;
    printf("\n[Observer] Caught SIGINT, shutting down...\n");
    fflush(stdout);
}

int main(const int argc, char **argv) {
    signal(SIGINT, handle_sigint);

    if (argc < 2) { printf("Usage: %s <observer_id>\n", argv[0]); return 1; }
    const int obs_id = atoi(argv[1]);

    // открываем shared memory
    const int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    shm = static_cast<shared_t *>(mmap(nullptr, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

    char fifo_name[64];
    sprintf(fifo_name, FIFO_TEMPLATE, obs_id);

    const int fd = open(fifo_name, O_RDONLY | O_NONBLOCK);
    if (fd == -1) { perror("open fifo"); return 1; }

    char buf[256];
    ssize_t n = 0;

    while (!shm->stop) {
        n = read(fd, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            printf("%s", buf); fflush(stdout);
        } else {
            usleep(50000);
        }
    }

    close(fd);
    return 0;
}
