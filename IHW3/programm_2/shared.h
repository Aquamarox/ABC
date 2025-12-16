#ifndef SHARED
#define SHARED

#define DEPTS 3
#define SHM_NAME "/shop_shm"
#define SEM_PRINT "/sem_print"

typedef struct {
    int stop;  // используется для завершения по Ctrl+C
} shared_t;

#endif
