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
        pid_t otherPid = sharedPtr->clientPid;
        sem_post(semPtr);
        if (otherPid > 0)
        {
            kill(otherPid, SIGUSR1);
        }
    }
}

static void SetupSharedMemory()
{
    shmFd = shm_open(ShmName, O_CREAT | O_RDWR, 0600);
    if (shmFd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    const size_t size = sizeof(Shared);

    if (ftruncate(shmFd, static_cast<off_t>(size)) == -1)
    {
        perror("ftruncate");
        shm_unlink(ShmName);
        exit(EXIT_FAILURE);
    }

    void * mapped = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (mapped == MAP_FAILED)
    {
        perror("mmap");
        shm_unlink(ShmName);
        exit(EXIT_FAILURE);
    }

    sharedPtr = static_cast<Shared *>(mapped);

    semPtr = sem_open(SemName, O_CREAT, 0600, 1);
    if (semPtr == SEM_FAILED)
    {
        perror("sem_open");
        munmap(mapped, size);
        shm_unlink(ShmName);
        exit(EXIT_FAILURE);
    }

    sem_wait(semPtr);
    sharedPtr->serverPid = getpid();
    sharedPtr->clientPid = 0;
    sharedPtr->number = -1;
    sharedPtr->terminate = 0;
    sem_post(semPtr);
}

static void Cleanup()
{
    const size_t size = sizeof(Shared);

    if (semPtr != SEM_FAILED)
    {
        sem_close(semPtr);
    }
    sem_unlink(SemName);

    if (sharedPtr != nullptr)
    {
        munmap(sharedPtr, size);
    }
    if (shmFd != -1)
    {
        close(shmFd);
    }
    shm_unlink(ShmName);
}

int main()
{
    struct sigaction act {};
    act.sa_handler = SignalHandler;
    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);
    sigaction(SIGUSR1, &act, nullptr);

    SetupSharedMemory();

    for (;;)
    {
        sem_wait(semPtr);

        if (sharedPtr->terminate != 0)
        {
            sem_post(semPtr);
            break;
        }

        pid_t clientPid = sharedPtr->clientPid;
        int number = sharedPtr->number;

        sem_post(semPtr);

        if (clientPid != 0 && number != -1)
        {
            printf("Server received number: %d from client pid %d\n", number, clientPid);
            fflush(stdout);
        }

        if (signalReceived)
        {
            break;
        }

        sleep(1);
    }

    Cleanup();
    return 0;
}
