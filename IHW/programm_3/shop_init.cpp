#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include "shared.h"
#include <sys/stat.h>

int main() {
    const int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shared_t));
    shared_t *shm = static_cast<shared_t *>(mmap(nullptr, sizeof(shared_t),
                                                 PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    shm->stop = 0;

    // создаем семафоры для продавцов
    char sem_name[32];
    for (int i = 0; i < DEPTS; i++) {
        sprintf(sem_name, "/sem_seller%d", i);
        sem_unlink(sem_name);
        sem_open(sem_name, O_CREAT, 0666, 0);
    }

    // семафор для синхронного вывода
    sem_unlink(SEM_PRINT);
    sem_open(SEM_PRINT, O_CREAT, 0666, 1);

    // создаем FIFO для наблюдателя
    mkfifo(FIFO_NAME, 0666);

    printf("Shop initialized.\n");
    return 0;
}
