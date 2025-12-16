#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include "shared.h"
#include <signal.h>

shared_t *shm = nullptr;

void handle_sigint(int sig) {
    if (shm) shm->stop = 1;
    printf("\n[Seller] Caught SIGINT, shutting down...\n");
    fflush(stdout);
}

int main(const int argc, char **argv) {
    signal(SIGINT, handle_sigint);

    if (argc < 2) { printf("Usage: %s <seller_id>\n", argv[0]); return 1; }
    const int id = atoi(argv[1]);

    const int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    shm = static_cast<shared_t *>(mmap(nullptr, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

    char sem_name[32];
    sprintf(sem_name, "/sem_seller%d", id);
    sem_t *sem_seller = sem_open(sem_name, 0);
    sem_t *sem_print = sem_open(SEM_PRINT, 0);

    const int fifo_fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK);

    char buf[256];
    sem_wait(sem_print);
    sprintf(buf, "[Seller %d] started\n", id);
    printf("%s", buf); fflush(stdout);
    sem_post(sem_print);
    if (fifo_fd != -1) write(fifo_fd, buf, strlen(buf));

    srand(time(nullptr) + id);
    while (!shm->stop) {
        if (sem_trywait(sem_seller) == 0) {
            sem_wait(sem_print);
            sprintf(buf, "[Seller %d] serving customer...\n", id);
            printf("%s", buf); fflush(stdout);
            sem_post(sem_print);
            if (fifo_fd != -1) write(fifo_fd, buf, strlen(buf));

            usleep(200000 + rand() % 400000);

            sem_wait(sem_print);
            sprintf(buf, "[Seller %d] finished\n", id);
            printf("%s", buf); fflush(stdout);
            sem_post(sem_print);
            if (fifo_fd != -1) write(fifo_fd, buf, strlen(buf));
        } else {
            usleep(50000);
        }
    }

    sem_wait(sem_print);
    sprintf(buf, "[Seller %d] exiting\n", id);
    printf("%s", buf); fflush(stdout);
    sem_post(sem_print);
    if (fifo_fd != -1) write(fifo_fd, buf, strlen(buf));

    if (fifo_fd != -1) close(fifo_fd);
    return 0;
}
