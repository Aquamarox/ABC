#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>

volatile sig_atomic_t ack_received = 0;

void ack_handler(int sig)
{
    ack_received = 1;
}

int main(void)
{
    pid_t receiver_pid;
    int32_t number;
    int i;
    int bit;

    printf("Sender PID: %d\n", getpid());
    printf("Enter receiver PID: ");
    scanf("%d", &receiver_pid);

    printf("Enter integer number: ");
    scanf("%d", &number);

    {
        struct sigaction sa;
        sa.sa_handler = ack_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGUSR1, &sa, NULL);
        sigaction(SIGUSR2, &sa, NULL);
    }

    printf("\nSending bits (LSB -> MSB):\n");

    for (i = 0; i < 32; i = i + 1)
    {
        bit = (number >> i) & 1;
        ack_received = 0;

        printf("Sent bit %d (bit #%d)\n", bit, i);

        if (bit == 0)
            kill(receiver_pid, SIGUSR1);
        else
            kill(receiver_pid, SIGUSR2);

        while (!ack_received)
            pause();
    }

    kill(receiver_pid, SIGINT);

    printf("Transmission finished\n");
    return 0;
}
