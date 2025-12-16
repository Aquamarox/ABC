#ifndef SHARED
#define SHARED

#define DEPTS 3
#define SHM_NAME "/shop_shm"
#define SEM_PRINT "/sem_print"

// Количество наблюдателей
#define NUM_OBSERVERS 2

// Шаблон имен FIFO для наблюдателей
#define FIFO_TEMPLATE "/tmp/shop_fifo_%d"

typedef struct {
    int stop;
} shared_t;

#endif