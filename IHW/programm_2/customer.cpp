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
    printf("\n[Customer] Caught SIGINT, leaving shop...\n");
    fflush(stdout);
}

int main(int argc, char **argv) {
    signal(SIGINT, handle_sigint);

    if (argc < 2) { printf("Usage: %s <customer_id>\n", argv[0]); return 1; }
    const int cid = atoi(argv[1]);

    const int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    shm = static_cast<shared_t *>(mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

    sem_t *sem_print = sem_open(SEM_PRINT, 0);
    srand(time(NULL) + cid);

    sem_wait(sem_print);
    printf("   (Customer %d) entered shop\n", cid); fflush(stdout);
    sem_post(sem_print);

    const int visits = 1 + rand() % DEPTS;
    for (int i = 0; i < visits && !shm->stop; i++) {
        const int dept = rand() % DEPTS;
        char sem_name[32];
        sprintf(sem_name, "/sem_seller%d", dept);
        sem_t *sem_seller = sem_open(sem_name, 0);

        sem_wait(sem_print);
        printf("   (Customer %d) waiting for dept %d\n", cid, dept); fflush(stdout);
        sem_post(sem_print);

        sem_post(sem_seller); // сигнал продавцу
        usleep(200000 + rand() % 300000);

        sem_wait(sem_print);
        printf("   (Customer %d) done in dept %d\n", cid, dept); fflush(stdout);
        sem_post(sem_print);
    }

    sem_wait(sem_print);
    printf("   (Customer %d) leaving shop\n", cid); fflush(stdout);
    sem_post(sem_print);

    return 0;
}
