#include "headers.h"
#include <string.h>
#include <unistd.h> // for usleep
/* Modify this file as needed*/
int remainingtime;

struct Time
{
    int remining;
    int start_time;
};
void *shr;
// Time time;
int start = 0;
int running = 0;
void ClearCock(int signum) // if it receive interrupt signal
{
    shmdt(shr);
    destroyClk(false);

    exit(0);
}
void memory()
{

    // memcpy(( int*)shr, &time, sizeof(int));
  ((struct Time *)shr)->start_time = start;//first time it runs
   // ((struct Time *)shr)->runnunig_time = running;

    kill(getppid(), SIGUSR1); // to save information and notify scheduler
}
void StopHandler(int signum)
{
    memory();
    raise(SIGSTOP);
}
int main(int agrc, char *argv[])
{
    // printf("helloFromProcess\n");
    signal(SIGINT, ClearCock);
    signal(SIGUSR2, StopHandler);
    int shm_id = atoi(argv[1]);
    remainingtime = atoi(argv[2]); // first time
                                   //  int slice = atoi(argv[3]);
    int actual_runnuing_time = 0;
    // TODO it needs to get the remaining time from somewhere(sheduler)
    // remainingtime = ??;
    shr = shmat(shm_id, NULL, 0);
    kill(getppid(),SIGCONT);
    kill(getpid(), SIGUSR2); // to stop it initially when create

    if (shr == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    initClk();

    int prev = getClk();
    //printf("time=%d\n", prev);
    start = prev;
    // time.start_time = prev; // firts time it runs
    // time.runnunig_time = 0;
    while ( ((struct Time *)shr)->remining > 0)
    {
    }

    /////if it normally finished
    printf("finish\n");
    destroyClk(false);
    memory();
    shmdt(shr);

    exit(0);
}
