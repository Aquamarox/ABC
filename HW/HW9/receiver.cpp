#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>

volatile sig_atomic_t bit_count = 0;
volatile sig_atomic_t finished = 0;

int32_t received_number = 0;
int received_bits[32];
pid_t sender_pid = 0;

void bit0_handler(int sig, siginfo_t *info, void *context)
{
    sender_pid = info->si_pid;
    received_bits[bit_count] = 0;

    printf("Received bit 0 (bit #%d)\n", bit_count);

    bit_count = bit_count + 1;
    kill(sender_pid, SIGUSR1);
}

void bit1_handler(int sig, siginfo_t *info, void *context)
{
    sender_pid = info->si_pid;
    received_bits[bit_count] = 1;

    printf("Received bit 1 (bit #%d)\n", bit_count);

    received_number |= (1 << bit_count);
    bit_count = bit_count + 1;
    kill(sender_pid, SIGUSR1);
}

void finish_handler(int sig)
{
    finished = 1;
}

int main(void)
{
    int i;

    printf("Receiver PID: %d\n", getpid());
    printf("Enter sender PID: ");
    scanf("%d", &sender_pid);

    {
        struct sigaction sa0;
        struct sigaction sa1;
        struct sigaction sa_end;

        sa0.sa_sigaction = bit0_handler;
        sa0.sa_flags = SA_SIGINFO;
        sigemptyset(&sa0.sa_mask);

        sa1.sa_sigaction = bit1_handler;
        sa1.sa_flags = SA_SIGINFO;
        sigemptyset(&sa1.sa_mask);

        sa_end.sa_handler = finish_handler;
        sa_end.sa_flags = 0;
        sigemptyset(&sa_end.sa_mask);

        sigaction(SIGUSR1, &sa0, NULL);
        sigaction(SIGUSR2, &sa1, NULL);
        sigaction(SIGINT, &sa_end, NULL);
    }

    while (!finished)
        pause();

    printf("\nReceived bits (LSB -> MSB): ");
    for (i = 0; i < bit_count; i = i + 1)
        printf("%d", received_bits[i]);
    printf("\n");

    printf("Received number: %d\n", received_number);
    return 0;
}
