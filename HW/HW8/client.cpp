#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>

struct Shared
{
    pid_t serverPid;
    pid_t clientPid;
    int number;
    int terminate;
};

static const char * const ShmName = "/posix_shm_example";
static const char * const SemName = "/posix_shm_example_sem";
static Shared * sharedPtr = nullptr;
static sem_t * semPtr = SEM_FAILED;
static int shmFd = -1;
static volatile sig_atomic_t signalReceived = 0;

static void SignalHandler(int)
{
    signalReceived = 1;

    if (sharedPtr != nullptr)
    {
        sem_wait(semPtr);
        sharedPtr->terminate = 1;
        pid_t otherPid = sharedPtr->serverPid;
        sem_post(semPtr);

        if (otherPid > 0)
        {
            kill(otherPid, SIGUSR1);
        }
    }
}

static void ConnectSharedMemory()
{
    const size_t size = sizeof(Shared);

    for (int i = 0; i < 30; ++i)
    {
        shmFd = shm_open(ShmName, O_RDWR, 0);
        if (shmFd != -1)
        {
            break;
        }
        sleep(1);
    }

    if (shmFd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    void * mapped = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (mapped == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sharedPtr = static_cast<Shared *>(mapped);

    semPtr = sem_open(SemName, 0);
    if (semPtr == SEM_FAILED)
    {
        perror("sem_open");
        munmap(mapped, size);
        exit(EXIT_FAILURE);
    }

    sem_wait(semPtr);
    sharedPtr->clientPid = getpid();
    sem_post(semPtr);
}

static void Cleanup()
{
    const size_t size = sizeof(Shared);

    if (semPtr != SEM_FAILED)
    {
        sem_close(semPtr);
    }

    if (sharedPtr != nullptr)
    {
        munmap(sharedPtr, size);
    }

    if (shmFd != -1)
    {
        close(shmFd);
    }

    sem_unlink(SemName);
    shm_unlink(ShmName);
}

int main()
{
    struct sigaction act {};
    act.sa_handler = SignalHandler;
    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);
    sigaction(SIGUSR1, &act, nullptr);

    ConnectSharedMemory();

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (;;)
    {
        if (signalReceived)
        {
            break;
        }

        const int number = std::rand() % 100;

        sem_wait(semPtr);

        if (sharedPtr->terminate != 0)
        {
            sem_post(semPtr);
            break;
        }

        sharedPtr->number = number;

        sem_post(semPtr);

        sleep(1);
    }

    Cleanup();
    return 0;
}
