#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define SOURCES 100
#define BUFFER_SIZE 1000

static int buffer[BUFFER_SIZE];
static int head = 0;
static int tail = 0;
static int count = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static int active_sources = SOURCES;
static int active_summers = 0;
static int done = 0;

static void buffer_push(int value)
{
    buffer[tail] = value;
    tail = (tail + 1) % BUFFER_SIZE;
    count++;
}

static int buffer_pop()
{
    int value = buffer[head];
    head = (head + 1) % BUFFER_SIZE;
    count--;
    return value;
}

static void* source_thread(void* arg)
{
    int id = *(int*)arg;

    int delay = rand() % 7 + 1;
    sleep(delay);

    int value = rand() % 100 + 1;

    pthread_mutex_lock(&mutex);
    buffer_push(value);
    printf("[SOURCE %d] produced %d | buffer=%d\n", id, value, count);
    active_sources--;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

struct sum_args {
    int a;
    int b;
};

static void* summer_thread(void* arg)
{
    struct sum_args* s = (struct sum_args*)arg;
    int a = s->a;
    int b = s->b;
    free(s);

    int delay = rand() % 4 + 3;
    sleep(delay);

    int result = a + b;

    pthread_mutex_lock(&mutex);
    buffer_push(result);
    active_summers--;
    printf("[SUM] %d + %d = %d | buffer=%d\n", a, b, result, count);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

static void* monitor_thread(void* arg)
{
    (void)arg;

    while (1)
    {
        pthread_mutex_lock(&mutex);

        while (count < 2 && !done)
        {
            if (active_sources == 0 && count == 1 && active_summers == 0)
            {
                done = 1;
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            pthread_cond_wait(&cond, &mutex);
        }

        if (done)
        {
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        int a = buffer_pop();
        int b = buffer_pop();

        printf("[MONITOR] take %d and %d | buffer=%d\n", a, b, count);

        struct sum_args* args =
            (struct sum_args*)malloc(sizeof(struct sum_args));
        args->a = a;
        args->b = b;

        pthread_t tid;
        active_summers++;
        pthread_create(&tid, NULL, summer_thread, args);
        pthread_detach(tid);

        pthread_mutex_unlock(&mutex);
    }
}

int main()
{
    srand(time(NULL));

    pthread_t sources[SOURCES];
    pthread_t monitor;
    int ids[SOURCES];

    pthread_create(&monitor, NULL, monitor_thread, NULL);

    for (int i = 0; i < SOURCES; i++)
    {
        ids[i] = i + 1;
        pthread_create(&sources[i], NULL, source_thread, &ids[i]);
    }

    for (int i = 0; i < SOURCES; i++)
        pthread_join(sources[i], NULL);

    pthread_join(monitor, NULL);

    pthread_mutex_lock(&mutex);
    if (count == 1)
        printf("\nFINAL RESULT = %d\n", buffer[head]);
    pthread_mutex_unlock(&mutex);

    return 0;
}
