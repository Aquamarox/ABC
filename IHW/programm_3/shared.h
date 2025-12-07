#ifndef SHARED
#define SHARED

#define DEPTS 3
#define SHM_NAME "/shop_shm"
#define SEM_PRINT "/sem_print"
#define FIFO_NAME "/tmp/shop_fifo"   // один общий FIFO для наблюдателя

typedef struct {
    int stop;
} shared_t;

#endif
