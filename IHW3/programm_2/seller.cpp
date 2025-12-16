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

int main(int argc, char **argv) {
    signal(SIGINT, handle_sigint);

    if (argc < 2) { printf("Usage: %s <seller_id>\n", argv[0]); return 1; }
    const int id = atoi(argv[1]);

    const int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    shm = static_cast<shared_t *>(mmap(nullptr, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

    char sem_name[32];
    sprintf(sem_name, "/sem_seller%d", id);
    sem_t *sem_seller = sem_open(sem_name, 0);
    sem_t *sem_print = sem_open(SEM_PRINT, 0);

    sem_wait(sem_print);
    printf("[Seller %d] started\n", id); fflush(stdout);
    sem_post(sem_print);

    srand(time(nullptr) + id);
    while (!shm->stop) {
        if (sem_trywait(sem_seller) == 0) {
            sem_wait(sem_print);
            printf("[Seller %d] serving customer...\n", id); fflush(stdout);
            sem_post(sem_print);

            usleep(200000 + rand() % 400000);

            sem_wait(sem_print);
            printf("[Seller %d] finished\n", id); fflush(stdout);
            sem_post(sem_print);
        } else {
            usleep(50000);
        }
    }

    sem_wait(sem_print);
    printf("[Seller %d] exiting\n", id); fflush(stdout);
    sem_post(sem_print);

    return 0;
}
