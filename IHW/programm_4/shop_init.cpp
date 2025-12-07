#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include "shared.h"
#include <sys/stat.h>
#include <signal.h>

shared_t *shm = nullptr;

void handle_sigint(int sig) {
    if (shm) shm->stop = 1;
    printf("\n[Shop Init] Caught SIGINT, shutting down...\n");
    fflush(stdout);
}

int main() {
    signal(SIGINT, handle_sigint);

    const int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shared_t));
    shm = static_cast<shared_t *>(mmap(NULL, sizeof(shared_t),
                                       PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    shm->stop = 0;

    sem_unlink(SEM_PRINT);
    sem_open(SEM_PRINT, O_CREAT, 0666, 1);

    char fifo_name[64];
    for (int i = 0; i < NUM_OBSERVERS; i++) {
        sprintf(fifo_name, FIFO_TEMPLATE, i);
        mkfifo(fifo_name, 0666);
    }

    printf("Shop initialized.\n");
    return 0;
}
