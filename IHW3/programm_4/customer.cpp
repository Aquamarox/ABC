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
    printf("\n[Customer] Caught SIGINT, shutting down...\n");
    fflush(stdout);
}

int main(const int argc, char **argv) {
    signal(SIGINT, handle_sigint);

    if (argc < 2) { printf("Usage: %s <customer_id>\n", argv[0]); return 1; }
    const int cid = atoi(argv[1]);

    const int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    shm = static_cast<shared_t *>(mmap(nullptr, sizeof(shared_t),
                                       PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

    sem_t *sem_print = sem_open(SEM_PRINT, 0);

    int fifo_fds[NUM_OBSERVERS];
    char fifo_name[64];
    for (int i = 0; i < NUM_OBSERVERS; i++) {
        sprintf(fifo_name, FIFO_TEMPLATE, i);
        fifo_fds[i] = open(fifo_name, O_WRONLY | O_NONBLOCK);
    }

    srand(time(nullptr) + cid);
    char buf[256];

    sem_wait(sem_print);
    sprintf(buf, "   (Customer %d) entered shop\n", cid);
    printf("%s", buf); fflush(stdout);
    sem_post(sem_print);
    for (int i = 0; i < NUM_OBSERVERS; i++) {
        if (fifo_fds[i] != -1) write(fifo_fds[i], buf, strlen(buf));
    }
    const int visits = 1 + rand() % DEPTS;
    for (int i = 0; i < visits; i++) {
        const int dept = rand() % DEPTS;
        char sem_name[32];
        sprintf(sem_name, "/sem_seller%d", dept);
        sem_t *sem_seller = sem_open(sem_name, 0);

        sem_wait(sem_print);
        sprintf(buf, "   (Customer %d) waiting for dept %d\n", cid, dept);
        printf("%s", buf); fflush(stdout);
        sem_post(sem_print);
        for (int j = 0; j < NUM_OBSERVERS; j++) {
            if (fifo_fds[j] != -1) write(fifo_fds[j], buf, strlen(buf));
        }
        sem_post(sem_seller);
        usleep(200000 + rand() % 300000);

        sem_wait(sem_print);
        sprintf(buf, "   (Customer %d) done in dept %d\n", cid, dept);
        printf("%s", buf); fflush(stdout);
        sem_post(sem_print);
        for (int j = 0; j < NUM_OBSERVERS; j++) {
            if (fifo_fds[j] != -1) write(fifo_fds[j], buf, strlen(buf));
        }
    }

    sem_wait(sem_print);
    sprintf(buf, "   (Customer %d) leaving shop\n", cid);
    printf("%s", buf); fflush(stdout);
    sem_post(sem_print);
    for (int i = 0; i < NUM_OBSERVERS; i++) {
        if (fifo_fds[i] != -1) write(fifo_fds[i], buf, strlen(buf));
    }
    for (int i = 0; i < NUM_OBSERVERS; i++) {
        if (fifo_fds[i] != -1) close(fifo_fds[i]);
    }
    return 0;
}
