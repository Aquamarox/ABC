#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

#define DEPTS 3   // три отдела

typedef struct {
    sem_t seller_free[DEPTS]; // семафор каждого продавца
    sem_t print_lock; // семафор для синхронного вывода
    int stop; // признак завершения
} shared_t;

shared_t *shm = nullptr;

// безопасный вывод
void safe_print(const char *msg) {
    sem_wait(&shm->print_lock);
    printf("%s", msg);
    fflush(stdout);
    sem_post(&shm->print_lock);
}

// процесс продавца
void seller_proc(const int id) {
    char buf[128];
    sprintf(buf, "[Seller %d] started\n", id);
    safe_print(buf);

    while (!shm->stop) {
        if (sem_trywait(&shm->seller_free[id]) == 0) {
            sprintf(buf, "[Seller %d] serving customer...\n", id);
            safe_print(buf);

            usleep(200000 + rand() % 400000);

            sprintf(buf, "[Seller %d] finished\n", id);
            safe_print(buf);
        } else {
            usleep(50000);
        }
    }

    sprintf(buf, "[Seller %d] exiting\n", id);
    safe_print(buf);
    exit(0);
}

// процесс покупателя
void customer_proc(const int cid) {
    char buf[128];
    sprintf(buf, "   (Customer %d) entered shop\n", cid);
    safe_print(buf);

    const int visits = 1 + rand() % DEPTS;

    for (int i = 0; i < visits && !shm->stop; i++) {
        const int dept = rand() % DEPTS;
        sprintf(buf, "   (Customer %d) waiting for dept %d\n", cid, dept);
        safe_print(buf);

        // сигнал продавцу
        sem_post(&shm->seller_free[dept]);

        usleep(200000 + rand() % 300000);

        sprintf(buf, "   (Customer %d) done in dept %d\n", cid, dept);
        safe_print(buf);
    }

    sprintf(buf, "   (Customer %d) leaving shop\n", cid);
    safe_print(buf);
    exit(0);
}

// обработчик Ctrl+C
void sig_handler(int s) {
    (void) s;
    if (shm) {
        shm->stop = 1;

        // чтобы продавцы не зависли
        for (int i = 0; i < DEPTS; i++) {
            sem_post(&shm->seller_free[i]);
        }
    }
}

int main(const int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <num_customers>\n", argv[0]);
        return 1;
    }

    const int customers = atoi(argv[1]);
    srand(time(nullptr));

    // -------- shared memory --------
    shm = static_cast<shared_t *>(mmap(nullptr, sizeof(shared_t),
                                       PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

    shm->stop = 0;
    for (int i = 0; i < DEPTS; i++)
        sem_init(&shm->seller_free[i], 1, 0);

    sem_init(&shm->print_lock, 1, 1);

    signal(SIGINT, sig_handler);

    // запускаем продавцов
    for (int i = 0; i < DEPTS; i++) {
        if (fork() == 0) seller_proc(i);
        usleep(50000);
    }

    // запускаем покупателей
    for (int i = 0; i < customers; i++) {
        if (fork() == 0) customer_proc(i);
        usleep(100000);
    }

    // ждём всех покупателей
    for (int i = 0; i < customers; i++) {
        wait(nullptr);
    }

    // сигнализируем продавцам завершить работу
    shm->stop = 1;
    for (int i = 0; i < DEPTS; i++) {
        sem_post(&shm->seller_free[i]);
    }

    // ждём всех продавцов
    for (int i = 0; i < DEPTS; i++) {
        wait(nullptr);
    }

    // удаляем семафоры и shared memory
    for (int i = 0; i < DEPTS; i++) {
        sem_destroy(&shm->seller_free[i]);
    }
    sem_destroy(&shm->print_lock);
    munmap(shm, sizeof(shared_t));

    printf("Main: finished\n");
    return 0;
}
